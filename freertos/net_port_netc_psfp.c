/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FreeRTOS specific Per Stream Filtering and Policing service implementation
 @details
*/

#include "config.h"
#include "genavb/qos.h"

#if CFG_NUM_NETC_SW

#include "net_port_netc_psfp.h"
#include "net_port_netc_sw_drv.h"
#include "net_port_netc_stream_identification.h"

int netc_sw_psfp_get_eid(void *sw_drv, uint32_t handle, uint32_t *rp_eid, uint32_t *sg_eid, uint32_t *isc_eid, uint32_t *sf_eid, uint16_t *msdu)
{
	struct netc_sw_drv *drv = (struct netc_sw_drv *)sw_drv;
	struct netc_sw_stream_filter *sf = NULL;
	int index;
	int rc = 0;

	for (index = 0; index < drv->max_stream_filters_instances; index++) {
		if ((drv->stream_filters[index].used) && (drv->stream_filters[index].stream_handle == handle)) {

			if ((sf == NULL) || (drv->stream_filters[index].priority_spec == GENAVB_PRIORITY_SPEC_WILDCARD))
				sf = &drv->stream_filters[index];

			if (sf->priority_spec == GENAVB_PRIORITY_SPEC_WILDCARD)
				break;
		}
	}

	if (sf != NULL) {
		*rp_eid = sf->flow_meter_ref;
		*sg_eid = sf->stream_gate_ref;
		*isc_eid = sf->isc_eid;
		*sf_eid = sf->sf_eid;
		*msdu = sf->max_sdu_size;
		rc = 1;
	}

	return rc;
}

static void netc_sw_psfp_get_isc_eid(struct netc_sw_stream_filter *sf, u32 index)
{
	sf->isc_eid = index;
}

static int netc_sw_psfp_delete_notify(struct netc_sw_drv *drv, struct netc_sw_stream_filter *sf)
{
	return netc_si_update_sf_ref(drv, sf->stream_handle);
}

static int netc_sw_psfp_update_notify(struct netc_sw_drv *drv, struct netc_sw_stream_filter *sf)
{
	int rc = 0;

	if (sf->stream_handle != GENAVB_STREAM_HANDLE_WILDCARD)
		rc = netc_si_update_sf_ref(drv, sf->stream_handle);

	return rc;
}

static int netc_sw_stream_filter_find_entry(struct netc_sw_drv *drv, uint32_t handle, uint8_t pcp, uint32_t *entry_id, netc_tb_isf_config_t *config)
{
	netc_tb_isf_rsp_data_t rsp;
	netc_tb_isf_keye_t keye;
	int rc;

	keye.isEID = handle;
	keye.pcp = pcp;

	rc = SWT_RxPSFPQueryISFTableEntry(&drv->handle, &keye, &rsp);

	switch (rc) {
	case kStatus_Success:
		*entry_id = rsp.entryID;
		memcpy(&config->cfge, &rsp.cfge, sizeof(netc_tb_isf_cfge_t));
		rc = 1;
		break;
	case kStatus_NETC_NotFound:
		rc = 0;
		break;
	default:
		rc = -1;
		break;
	}

	return rc;
}

static int netc_sw_stream_filter_update_hardware(struct netc_sw_drv *drv, struct netc_sw_stream_filter *sf)
{
	netc_tb_isf_config_t config;
	uint32_t entry_id;
	int rc;

	if (sf->priority_spec == GENAVB_PRIORITY_SPEC_WILDCARD) {
		if (SWT_RxPSFPAddISCTableEntry(&drv->handle, sf->isc_eid ) != kStatus_Success)
			goto err;
	} else {
		/* fill hardware entry with the software instance parameters */
		memset(&config, 0, sizeof(netc_tb_isf_config_t));

		rc = netc_sw_stream_filter_find_entry(drv, sf->stream_handle, sf->priority_spec, &entry_id, &config);
		if (rc < 0)
			goto err;

		if (!rc) {
			if (SWT_RxPSFPAddISCTableEntry(&drv->handle, sf->isc_eid) != kStatus_Success)
				goto err;

			config.keye.isEID = sf->stream_handle;
			config.keye.pcp = sf->priority_spec;

			if (SWT_RxPSFPAddISFTableEntry(&drv->handle, &config, &sf->sf_eid) != kStatus_Success)
				goto err_add_isf;
		}

		config.cfge.sduType = kNETC_PDU;
		config.cfge.msdu = sf->max_sdu_size;

		config.cfge.rpEID = sf->flow_meter_ref;
		if (sf->flow_meter_ref != NULL_ENTRY_ID)
			config.cfge.orp = 1;

		config.cfge.sgiEID = sf->stream_gate_ref;
		if (sf->stream_gate_ref != NULL_ENTRY_ID)
			config.cfge.osgi = 1;

		config.cfge.iscEID = sf->isc_eid;

		if (SWT_RxPSFPUpdateISFTableEntry(&drv->handle, sf->sf_eid, &config.cfge) != kStatus_Success)
			goto err_update_isf;
	}

	return 0;

err_update_isf:
	SWT_RxPSFPDelISFTableEntry(&drv->handle, sf->sf_eid);

err_add_isf:
	SWT_RxPSFPResetISCStatistic(&drv->handle, sf->isc_eid);

err:
	return -1;
}

static int netc_sw_stream_filter_update(struct net_bridge *bridge, uint32_t index, struct genavb_stream_filter_instance *sf)
{
	struct netc_sw_drv *drv = get_drv(bridge);
	struct netc_sw_stream_filter *netc_sf;

	if (index >= NUM_STREAM_FILTER_ENTRIES)
		goto err;

	if (sf->stream_handle >= SI_MAX_HANDLE)
		goto err;

	if ((sf->priority_spec >= QOS_PRIORITY_MAX) && (sf->priority_spec != GENAVB_PRIORITY_SPEC_WILDCARD))
		goto err;

	/* flow meter and stream gate not supported for now */
	if (sf->flow_meter_enable || (sf->stream_gate_ref != NULL_ENTRY_ID))
		goto err;

	/* stream blocking feature not supported */
	if ((sf->stream_blocked_due_to_oversize_frame_enabled == true) || ( sf->stream_blocked_due_to_oversize_frame == true))
		goto err;

	if (drv->stream_filters[index].used) {
		if (drv->stream_filters[index].stream_handle != sf->stream_handle)
			goto err;

		if (drv->stream_filters[index].priority_spec != sf->priority_spec)
			goto err;
	}

	if (sf->max_sdu_size > 0xFFFF)
		goto err;

	netc_sf = &drv->stream_filters[index];

	/* keep record of the stream filter parameters in software table */
	netc_sf->stream_handle = sf->stream_handle;
	netc_sf->priority_spec = sf->priority_spec;
	netc_sf->max_sdu_size = sf->max_sdu_size;
	netc_sf->stream_blocked_due_to_oversize_frame_enabled = sf->stream_blocked_due_to_oversize_frame_enabled;
	netc_sf->stream_blocked_due_to_oversize_frame = sf->stream_blocked_due_to_oversize_frame;
	netc_sf->stream_gate_ref = sf->stream_gate_ref;
	netc_sf->flow_meter_ref = sf->flow_meter_ref;
	netc_sf->flow_meter_enable = sf->flow_meter_enable;
	netc_sw_psfp_get_isc_eid(netc_sf, index);

	if (netc_sw_stream_filter_update_hardware(drv, netc_sf) < 0)
		goto err;

	netc_sf->used = true;

	netc_sw_psfp_update_notify(drv, netc_sf);

	return 0;
err:
	return -1;
}

static int netc_sw_stream_filter_delete(struct net_bridge *bridge, uint32_t index)
{
	struct netc_sw_drv *drv = get_drv(bridge);
	struct netc_sw_stream_filter *sf;
	int rc = -1;

	if (index >= NUM_STREAM_FILTER_ENTRIES)
		goto err;

	if (!drv->stream_filters[index].used)
		return 0;

	sf = &drv->stream_filters[index];

	sf->used = false;

	/* notify dependencies */
	netc_sw_psfp_delete_notify(drv, sf);

	if (sf->sf_eid != NULL_ENTRY_ID) {
		if (SWT_RxPSFPDelISFTableEntry(&drv->handle, sf->sf_eid) != kStatus_Success)
			goto err;
	}

	if (sf->isc_eid != NULL_ENTRY_ID) {
		if (SWT_RxPSFPResetISCStatistic(&drv->handle, sf->isc_eid) != kStatus_Success)
			goto err;
	}

	memset(sf, 0, sizeof(struct netc_sw_stream_filter));
	sf->sf_eid = NULL_ENTRY_ID;
	sf->isc_eid = NULL_ENTRY_ID;

	rc = 0;
err:
	return rc;
}

static int netc_sw_stream_filter_read(struct net_bridge *bridge, uint32_t index, struct genavb_stream_filter_instance *sf)
{
	struct netc_sw_drv *drv = get_drv(bridge);
	netc_tb_isf_config_t config = {0};
	netc_tb_isc_stse_t isc_stats;
	uint32_t entry_id;
	int rc = -1;

	if (index >= NUM_STREAM_FILTER_ENTRIES)
		goto err;

	if (!drv->stream_filters[index].used)
		return 0;

	sf->stream_filter_instance_id = index;
	sf->stream_handle = drv->stream_filters[index].stream_handle;
	sf->priority_spec = drv->stream_filters[index].priority_spec;

	if (drv->stream_filters[index].sf_eid == NULL_ENTRY_ID) {
		/* entry not yet programmed in hardware table */
		sf->max_sdu_size = drv->stream_filters[index].max_sdu_size;
		sf->flow_meter_ref = drv->stream_filters[index].flow_meter_ref;
		sf->stream_gate_ref = drv->stream_filters[index].stream_gate_ref;
		rc = 1;
	} else {
		rc = netc_sw_stream_filter_find_entry(drv, sf->stream_handle, sf->priority_spec, &entry_id, &config);
		if (rc <= 0)
			goto err;

		sf->max_sdu_size = config.cfge.msdu;
		sf->flow_meter_ref = config.cfge.rpEID;
		sf->stream_gate_ref = config.cfge.sgiEID;
	}

	if (SWT_RxPSFPGetISCStatistic(&drv->handle, drv->stream_filters[index].isc_eid, &isc_stats) == kStatus_Success) {
		sf->matching_frames_count = isc_stats.rxCount;
		sf->passing_frames_count = sf->matching_frames_count - isc_stats.msduDropCount - isc_stats.policerDropCount - isc_stats.sgDropCount;
		sf->not_passing_frames_count = isc_stats.msduDropCount + isc_stats.sgDropCount + isc_stats.policerDropCount;
		sf->red_frames_count = isc_stats.policerDropCount;
		sf->passing_sdu_count = isc_stats.rxCount - isc_stats.msduDropCount;
		sf->not_passing_sdu_count = isc_stats.msduDropCount;
	}

err:
	return rc;
}

static unsigned int netc_sw_stream_filter_get_max_entries(struct net_bridge *bridge)
{
#if 0
	struct netc_sw_drv *drv = get_drv(bridge);

	return SWT_RxPSFPGetISFTableMaxEntryNum(&drv->handle);
#else
	return NUM_STREAM_FILTER_ENTRIES;
#endif
}

static int netc_sw_stream_gate_update(struct net_bridge *bridge, uint32_t index, struct genavb_stream_gate_instance *sg)
{
	return 0;
}

static int netc_sw_stream_gate_delete(struct net_bridge *bridge, uint32_t index)
{
	return 0;
}

static int netc_sw_stream_gate_read(struct net_bridge *bridge, uint32_t index, struct genavb_stream_gate_instance *sg)
{
	sg->list_length = 0;

	return 0;
}

static unsigned int netc_sw_stream_gate_get_max_entries(struct net_bridge *bridge)
{
	return 0;
}

static unsigned int netc_sw_stream_gate_control_get_max_entries(struct net_bridge *bridge)
{
	return 0;
}

void netc_sw_psfp_init(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv = get_drv(bridge);
	int i;

	bridge->drv_ops.stream_filter_update = netc_sw_stream_filter_update;
	bridge->drv_ops.stream_filter_delete = netc_sw_stream_filter_delete;
	bridge->drv_ops.stream_filter_read = netc_sw_stream_filter_read;
	bridge->drv_ops.stream_filter_get_max_entries = netc_sw_stream_filter_get_max_entries;

	bridge->drv_ops.stream_gate_update = netc_sw_stream_gate_update;
	bridge->drv_ops.stream_gate_delete = netc_sw_stream_gate_delete;
	bridge->drv_ops.stream_gate_read = netc_sw_stream_gate_read;
	bridge->drv_ops.stream_gate_get_max_entries = netc_sw_stream_gate_get_max_entries;
	bridge->drv_ops.stream_gate_control_get_max_entries = netc_sw_stream_gate_control_get_max_entries;

	drv->max_stream_filters_instances = netc_sw_stream_filter_get_max_entries(bridge);

	for (i = 0; i < drv->max_stream_filters_instances; i++) {
		drv->stream_filters[i].sf_eid = NULL_ENTRY_ID;
		drv->stream_filters[i].isc_eid = NULL_ENTRY_ID;
	}
}
#endif
