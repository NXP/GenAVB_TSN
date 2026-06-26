/*
* Copyright 2023-2026 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Per Stream Filtering and Policing service implementation
 @details
*/

#include "config.h"
#include "genavb/error.h"
#include "genavb/qos.h"

#include "common/log.h"

#if CFG_NUM_NETC_SW

#include "net_port_netc_psfp.h"
#include "net_port_netc_sw_drv.h"
#include "net_port_netc_stream_identification.h"

#define SG_NUM_SGCL_PER_ENTRY 8
#define RATE_POLICER_BITS_PER_SEC (3.725)

typedef enum {
	FLOW_METER_REF,
	STREAM_GATE_REF,
	STREAM_GATE_CONTROL_REF
} psfp_ref_t;

static unsigned int netc_sw_flow_meter_get_max_entries(struct net_psfp_ops *psfp_ops);

int netc_sw_psfp_get_eid(void *sw_drv, uint32_t handle, uint32_t *rp_eid, uint32_t *sg_eid, uint32_t *isc_eid, bool *sf_enable, uint16_t *msdu)
{
	struct netc_sw_drv *drv = (struct netc_sw_drv *)sw_drv;
	struct netc_psfp *psfp = &drv->psfp;
	struct netc_stream_filter *sf = NULL;
	bool priority_spec_specific = false;
	int index;
	int rc = 0;

	for (index = 0; index < psfp->max_stream_filters_instances; index++) {
		if ((psfp->stream_filters[index].used) && (psfp->stream_filters[index].stream_handle == handle)) {

			if ((sf == NULL) || (psfp->stream_filters[index].priority_spec == GENAVB_PRIORITY_SPEC_WILDCARD))
				sf = &psfp->stream_filters[index];

			if (sf->priority_spec == GENAVB_PRIORITY_SPEC_WILDCARD)
				break;
			else
				priority_spec_specific = true;
		}
	}

	if (sf != NULL) {
		if (sf->priority_spec != GENAVB_PRIORITY_SPEC_WILDCARD) {
			/* Filtering is done for a specific PCP. Flow meter, stream gate, ingress counters EIDs and the 
			MSDU parameters are used from the Ingress Stream Filter hardware table (). */
			*rp_eid = NULL_ENTRY_ID;
			*sg_eid = NULL_ENTRY_ID;
			*isc_eid = NULL_ENTRY_ID;
			*sf_enable = true;
			*msdu = 0;
		} else {
			/* No additional look-up is required in the Ingress Stream Filter table (we do not filter on a specific PCP).
			Flow meter, stream gate, ingress counters EIDs and the MSDU parameters specified by the user are passed 
			to the Ingress Stream table entry */
			if (!priority_spec_specific) {
				*rp_eid = sf->flow_meter_ref;
				*sg_eid = sf->stream_gate_ref;
				*isc_eid = sf->isc_eid;
				*sf_enable = false;
				*msdu = sf->max_sdu_size;
			} else {
				/* Special case where we found a pcp specific entry as well as a wildcard one 
				in the stream filter entries */
				*rp_eid = sf->flow_meter_ref;
				*sg_eid = sf->stream_gate_ref;
				*isc_eid = sf->isc_eid;
				*sf_enable = true;
				*msdu = sf->max_sdu_size;
			}
		}

		rc = 1;
	}

	return rc;
}

static void netc_sw_psfp_get_isc_eid(struct netc_stream_filter *sf, u32 index)
{
	sf->isc_eid = index;
}

static int netc_sw_psfp_delete_notify(struct netc_psfp *psfp, struct netc_stream_filter *sf)
{
	return netc_si_update_sf_ref(psfp->si, sf->stream_handle);
}

static int netc_sw_psfp_update_notify(struct netc_psfp *psfp, struct netc_stream_filter *sf)
{
	int rc = 0;

	if (sf->stream_handle != GENAVB_STREAM_HANDLE_WILDCARD)
		rc = netc_si_update_sf_ref(psfp->si, sf->stream_handle);

	return rc;
}

static int netc_sw_stream_gate_get_entry(struct netc_psfp *psfp, uint32_t entry_id, netc_tb_sgi_config_t *cfg)
{
	int rc;

	rc = SWT_RxPSFPQuerySGITableEntry(psfp->hw.handle, entry_id, cfg);

	switch (rc) {
	case kStatus_Success:
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

static int netc_sw_flow_meter_find_entry(struct netc_psfp *psfp,  uint32_t entry_id, netc_tb_rp_rsp_data_t *rsp)
{
	int rc;

	rc = SWT_RxPSFPQueryRPTableEntry(psfp->hw.handle, entry_id, rsp);

	switch (rc) {
	case kStatus_Success:
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

static int netc_sw_stream_filter_find_entry(struct netc_psfp *psfp, uint32_t handle, uint8_t pcp, uint32_t *entry_id, netc_tb_isf_config_t *config)
{
	netc_tb_isf_rsp_data_t rsp;
	netc_tb_isf_keye_t keye;
	int rc;

	keye.isEID = handle;
	keye.pcp = pcp;

	rc = SWT_RxPSFPQueryISFTableEntry(psfp->hw.handle, &keye, &rsp);

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

static int netc_sw_stream_filter_update_hardware(struct netc_psfp *psfp, struct netc_stream_filter *sf, uint32_t stream_gate_ref, uint32_t flow_meter_ref)
{
	netc_tb_isf_config_t config;
	netc_tb_sgi_config_t sgi_config;
	netc_tb_rp_rsp_data_t rp_rsp;
	int rc;

	if (sf->priority_spec == GENAVB_PRIORITY_SPEC_WILDCARD) {
		if (SWT_RxPSFPAddISCTableEntry(psfp->hw.handle, sf->isc_eid ) != kStatus_Success)
			goto err;
	} else {
		/* fill hardware entry with the software instance parameters */
		memset(&config, 0, sizeof(netc_tb_isf_config_t));

		rc = netc_sw_stream_filter_find_entry(psfp, sf->stream_handle, sf->priority_spec, &sf->sf_eid, &config);
		if (rc < 0)
			goto err;

		if (!rc) {
			if (SWT_RxPSFPAddISCTableEntry(psfp->hw.handle, sf->isc_eid) != kStatus_Success)
				goto err;

			config.keye.isEID = sf->stream_handle;
			config.keye.pcp = sf->priority_spec;

			if (SWT_RxPSFPAddISFTableEntry(psfp->hw.handle, &config, &sf->sf_eid) != kStatus_Success)
				goto err_add_isf;
		}

		config.cfge.sduType = kNETC_PDU;
		config.cfge.msdu = sf->max_sdu_size;
		config.cfge.rpEID = sf->flow_meter_ref;
		config.cfge.sgiEID = sf->stream_gate_ref;

		config.cfge.osgi = 0;
		if (stream_gate_ref != NULL_ENTRY_ID) {
			if (netc_sw_stream_gate_get_entry(psfp, stream_gate_ref, &sgi_config) > 0)
				config.cfge.osgi = 1;
		}

		config.cfge.orp = 0;
		if ((flow_meter_ref != NULL_ENTRY_ID) && (sf->flow_meter_enable)) {
			if (netc_sw_flow_meter_find_entry(psfp, flow_meter_ref, &rp_rsp) > 0)
				config.cfge.orp = 1;
		}

		config.cfge.iscEID = sf->isc_eid;

		if (SWT_RxPSFPUpdateISFTableEntry(psfp->hw.handle, sf->sf_eid, &config.cfge) != kStatus_Success)
			goto err_update_isf;
	}

	return 0;

err_update_isf:
	SWT_RxPSFPDelISFTableEntry(psfp->hw.handle, sf->sf_eid);

err_add_isf:
	SWT_RxPSFPResetISCStatistic(psfp->hw.handle, sf->isc_eid);

err:
	return -1;
}

static void netc_sw_stream_filter_update_ref(struct netc_psfp *psfp, uint32_t ref, psfp_ref_t ref_type)
{
	struct netc_stream_filter *sf;
	int index;

	switch (ref_type) {
	case STREAM_GATE_REF:
		for (index = 0; index < psfp->max_stream_filters_instances; index++) {
			sf = &psfp->stream_filters[index];
			if (sf->stream_gate_ref == ref)
				netc_sw_stream_filter_update_hardware(psfp, sf, sf->stream_gate_ref, sf->flow_meter_ref);
		}
		break;

	case FLOW_METER_REF:
		for (index = 0; index < psfp->max_stream_filters_instances; index++) {
			sf = &psfp->stream_filters[index];
			if (sf->flow_meter_ref == ref)
				netc_sw_stream_filter_update_hardware(psfp, sf, sf->stream_gate_ref, sf->flow_meter_ref);
		}
		break;

	default:
		break;
	}
}

static int netc_sw_stream_filter_update(struct net_psfp_ops *psfp_ops, uint32_t index, struct genavb_stream_filter_instance *sf)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	struct netc_stream_filter *netc_sf;

	if (index >= NUM_STREAM_FILTER_ENTRIES)
		goto err;

	if (sf->stream_handle >= SI_MAX_HANDLE)
		goto err;

	if ((sf->priority_spec >= QOS_PRIORITY_MAX) && (sf->priority_spec != GENAVB_PRIORITY_SPEC_WILDCARD))
		goto err;

	/* stream blocking feature not supported */
	if ((sf->stream_blocked_due_to_oversize_frame_enabled == true) || ( sf->stream_blocked_due_to_oversize_frame == true))
		goto err;

	if (psfp->stream_filters[index].used) {
		if (psfp->stream_filters[index].stream_handle != sf->stream_handle)
			goto err;

		if (psfp->stream_filters[index].priority_spec != sf->priority_spec)
			goto err;
	}

	if (sf->max_sdu_size > 0xFFFF)
		goto err;

	netc_sf = &psfp->stream_filters[index];

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

	if (netc_sw_stream_filter_update_hardware(psfp, netc_sf, sf->stream_gate_ref, sf->flow_meter_ref) < 0)
		goto err;

	netc_sf->used = true;

	netc_sw_psfp_update_notify(psfp, netc_sf);

	return 0;
err:
	return -1;
}

static int netc_sw_stream_filter_delete(struct net_psfp_ops *psfp_ops, uint32_t index)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	struct netc_stream_filter *sf;
	int rc = -1;

	if (index >= NUM_STREAM_FILTER_ENTRIES)
		goto err;

	if (!psfp->stream_filters[index].used)
		return 0;

	sf = &psfp->stream_filters[index];

	sf->used = false;

	/* notify dependencies */
	netc_sw_psfp_delete_notify(psfp, sf);

	if (sf->sf_eid != NULL_ENTRY_ID) {
		if (SWT_RxPSFPDelISFTableEntry(psfp->hw.handle, sf->sf_eid) != kStatus_Success)
			goto err;
	}

	if (sf->isc_eid != NULL_ENTRY_ID) {
		if (SWT_RxPSFPResetISCStatistic(psfp->hw.handle, sf->isc_eid) != kStatus_Success)
			goto err;
	}

	memset(sf, 0, sizeof(struct netc_stream_filter));
	sf->sf_eid = NULL_ENTRY_ID;
	sf->isc_eid = NULL_ENTRY_ID;
	sf->flow_meter_ref = NULL_ENTRY_ID;
	sf->stream_gate_ref = NULL_ENTRY_ID;

	rc = 0;
err:
	return rc;
}

static void netc_sw_stream_filter_delete_ref(struct net_psfp_ops *psfp_ops, uint32_t ref, psfp_ref_t ref_type)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	struct netc_stream_filter *sf;
	int index;

	switch (ref_type) {
	case STREAM_GATE_REF:
		for (index = 0; index < psfp->max_stream_filters_instances; index++) {
			sf = &psfp->stream_filters[index];
			if (sf->stream_gate_ref == ref)
				netc_sw_stream_filter_update_hardware(psfp, sf, NULL_ENTRY_ID, sf->flow_meter_ref);
		}
		break;

	case FLOW_METER_REF:
		for (index = 0; index < psfp->max_stream_filters_instances; index++) {
			sf = &psfp->stream_filters[index];
			if (sf->flow_meter_ref == ref)
				netc_sw_stream_filter_update_hardware(psfp, sf, sf->stream_gate_ref, NULL_ENTRY_ID);
		}
		break;

	default:
		break;
	}
}

static int netc_sw_stream_filter_read(struct net_psfp_ops *psfp_ops, uint32_t index, struct genavb_stream_filter_instance *sf)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_tb_isf_config_t config = {0};
	netc_tb_isc_stse_t isc_stats;
	uint32_t entry_id;
	int rc = -1;

	if (index >= NUM_STREAM_FILTER_ENTRIES)
		goto err;

	if (!psfp->stream_filters[index].used)
		return 0;

	sf->stream_filter_instance_id = index;
	sf->stream_handle = psfp->stream_filters[index].stream_handle;
	sf->priority_spec = psfp->stream_filters[index].priority_spec;
	sf->flow_meter_enable = psfp->stream_filters[index].flow_meter_enable;

	if (psfp->stream_filters[index].sf_eid == NULL_ENTRY_ID) {
		/* entry not yet programmed in hardware table */
		sf->max_sdu_size = psfp->stream_filters[index].max_sdu_size;
		sf->flow_meter_ref = psfp->stream_filters[index].flow_meter_ref;
		sf->stream_gate_ref = psfp->stream_filters[index].stream_gate_ref;
		rc = 1;
	} else {
		rc = netc_sw_stream_filter_find_entry(psfp, sf->stream_handle, sf->priority_spec, &entry_id, &config);
		if (rc <= 0)
			goto err;

		sf->max_sdu_size = config.cfge.msdu;
		sf->flow_meter_ref = config.cfge.rpEID;
		sf->stream_gate_ref = config.cfge.sgiEID;
	}

	if (SWT_RxPSFPGetISCStatistic(psfp->hw.handle, psfp->stream_filters[index].isc_eid, &isc_stats) == kStatus_Success) {
		sf->matching_frames_count = isc_stats.rxCount;
		sf->not_passing_frames_count = isc_stats.msduDropCount + isc_stats.policerDropCount + isc_stats.sgDropCount;
		sf->passing_frames_count = sf->matching_frames_count - sf->not_passing_frames_count;
		sf->red_frames_count = isc_stats.policerDropCount;
		sf->passing_sdu_count = isc_stats.rxCount - isc_stats.msduDropCount;
		sf->not_passing_sdu_count = isc_stats.msduDropCount;
	}

err:
	return rc;
}

static unsigned int netc_sw_stream_filter_get_max_entries(struct net_psfp_ops *psfp_ops)
{
#if 0
	return SWT_RxPSFPGetISFTableMaxEntryNum(psfp->hw.handle);
#else
	return NUM_STREAM_FILTER_ENTRIES;
#endif
}

static int netc_sw_stream_gate_get_entry_state(struct netc_psfp *psfp, uint32_t entry_id, netc_tb_sgi_sgise_t *sgise)
{
	int rc;

	rc = SWT_RxPSFPGetSGIState(psfp->hw.handle, entry_id, sgise);

	switch (rc) {
	case kStatus_Success:
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

static int netc_sw_stream_gate_control_get_entry(struct netc_psfp *psfp, uint32_t entry_id, netc_tb_sgcl_gcl_t *gcl, uint32_t length)
{
	int rc;

	gcl->entryID = entry_id;

	rc = SWT_RxPSFPGetSGCLGateList(psfp->hw.handle, gcl, length);

	switch (rc) {
	case kStatus_Success:
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

static unsigned int netc_sw_stream_gate_get_max_entries(struct net_psfp_ops *psfp_ops)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);

	return SWT_RxPSFPGetSGITableMaxEntryNum(psfp->hw.handle);
}

static unsigned int netc_sw_stream_gate_control_get_max_entries(struct net_psfp_ops *psfp_ops)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);

	return SWT_RxPSFPGetSGCLTableMaxWordNum(psfp->hw.handle);
}

static void netc_sw_stream_gate_update_notify(struct netc_psfp *psfp, struct genavb_stream_gate_instance *sg)
{
	netc_sw_stream_filter_update_ref(psfp, sg->stream_gate_instance_id, STREAM_GATE_REF);
}

static int netc_sw_stream_gate_control_list_alloc_eid(struct net_psfp_ops *psfp_ops, uint32_t *eid)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_sgcl_gate_entry_t control_list;
	netc_tb_sgcl_sgclse_t sgcl_state;
	netc_tb_sgcl_gcl_t gcl;
	int entry_eid;
	int rc = -1;

	gcl.gcList = &control_list;

	for (entry_eid = 0; entry_eid < netc_sw_stream_gate_control_get_max_entries(psfp_ops); entry_eid += SG_NUM_SGCL_PER_ENTRY) {
		if (netc_sw_stream_gate_control_get_entry(psfp, entry_eid, &gcl, 1)) {
			/* entry allocated, delete it from the hw table if not used anymore by any stream gate instance */
			if (SWT_RxPSFPGetSGCLState(psfp->hw.handle, entry_eid, &sgcl_state) == kStatus_Success) {
				if (!sgcl_state.refCount) {
					if (SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, entry_eid) == kStatus_Success) {
						*eid = entry_eid;
						rc = 0;
						break;
					}
				}
			}
		} else {
			/* entry not allocated yet, can be used */
			*eid = entry_eid;
			rc = 0;
			break;
		}
	}

	return rc;
}

static int netc_sw_stream_gate_control_list_update(struct netc_psfp *psfp, uint32_t eid, struct genavb_stream_gate_instance *sg)
{
	netc_tb_sgcl_gcl_t gcl;
	netc_sgcl_gate_entry_t gcList[SG_NUM_SGCL_PER_ENTRY];
	int i, rc = GENAVB_SUCCESS;

	if (sg->list_length > SG_NUM_SGCL_PER_ENTRY) {
		rc = -GENAVB_ERR_SG_INVALID_GCL_SIZE;
		goto err;
	}

	gcl.entryID = eid; 
	gcl.extOipv = 0;
	gcl.extIpv = 0;
	gcl.extCtd = 0;
	gcl.extGtst = 0;
	gcl.cycleTime = (NSECS_PER_SEC * (uint64_t)sg->cycle_time_p) / sg->cycle_time_q;
	gcl.numEntries = sg->list_length;

	gcl.gcList = (netc_sgcl_gate_entry_t *)&gcList[0];
	for (i = 0; i < gcl.numEntries; i++) {
		gcl.gcList[i].timeInterval = sg->control_list[i].time_interval_value;
		gcl.gcList[i].iom = sg->control_list[i].interval_octet_max;
		if (sg->control_list[i].ipv_spec != GENAVB_IPV_SPEC_NULL) {
			gcl.gcList[i].ipv = sg->control_list[i].ipv_spec;
			gcl.gcList[i].oipv = 1;
		} else {
			gcl.gcList[i].ipv = 0;
			gcl.gcList[i].oipv = 0;
		}
		gcl.gcList[i].ctd = 0;
		gcl.gcList[i].iomen = (sg->control_list[i].interval_octet_max) ? 1 : 0;
		gcl.gcList[i].gtst = sg->control_list[i].gate_state_value;
	}

	if (SWT_RxPSFPAddSGCLTableEntry(psfp->hw.handle, &gcl) != kStatus_Success)
		rc = -GENAVB_ERR_SG_LIST_ADD;

err:
	return rc;
}

static int netc_sw_stream_gate_update(struct net_psfp_ops *psfp_ops, uint32_t index, struct genavb_stream_gate_instance *sg, unsigned int option)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_tb_sgi_config_t sgi_config;
	netc_tb_sgi_sgise_t sgi_state;
	uint32_t new_admin_eid;
	int rc;

	if (index >= netc_sw_stream_gate_get_max_entries(psfp_ops) || index >= NUM_STREAM_GATES_ENTRIES) {
		rc = -GENAVB_ERR_SG_MAX_ENTRIES;
		goto err;
	}

	rc = netc_sw_stream_gate_get_entry(psfp, index, &sgi_config);
	if (rc < 0) {
		rc = -GENAVB_ERR_SG_GET_ENTRY;
		goto err;
	} 

	/* add the new stream gate control list to the stream gate instance */
	sgi_config.entryID = index;

	switch (option) {
	case SG_UPDATE_OPTION_RESET_IRX:
	case SG_UPDATE_OPTION_RESET_OEX:
		if (netc_sw_stream_gate_get_entry_state(psfp, index, &sgi_state) < 0) {
			rc = -GENAVB_ERR_SG_GET_STATE_ENTRY;
			goto err;
		}

		if (((sgi_state.oex) && (option == SG_UPDATE_OPTION_RESET_OEX)) || 
		((sgi_state.irx) && (option == SG_UPDATE_OPTION_RESET_IRX))) {
			if (SWT_RxPSFPResetIRXOEXSGITableEntry(psfp->hw.handle, index) != kStatus_Success) {
				rc = -GENAVB_ERR_SG_RESET_IRX_OEX;
				goto err;
			}
		}
		break;
	
	case SG_UPDATE_OPTION_CONFIG:
		if (netc_sw_stream_gate_control_list_alloc_eid(psfp_ops, &new_admin_eid) < 0) {
			rc = -GENAVB_ERR_SG_LIST_ENTRY_ALLOC_EID;
			goto err;
		}

		/* update stream gate control list in hardware table */
		rc = netc_sw_stream_gate_control_list_update(psfp, new_admin_eid, sg);
		if (rc < 0)
			goto err;

		sgi_config.acfge.adminSgclEID = new_admin_eid;
		memcpy(&sgi_config.acfge.adminBaseTime[0], &sg->base_time, sizeof(uint64_t));
		sgi_config.acfge.adminCycleTimeExt = sg->cycle_time_extension;
		sgi_config.cfge.oexen = sg->gate_closed_due_to_octets_exceeded_enable;
		sgi_config.cfge.irxen = sg->gate_closed_due_to_invalid_rx_enable;
		sgi_config.cfge.sduType = kNETC_PDU;

		sgi_config.icfge.ipv = sg->admin_ipv;
		sgi_config.icfge.oipv = (sg->admin_ipv == GENAVB_IPV_SPEC_NULL)? 0 : 1;
		sgi_config.icfge.gst = sg->admin_gate_state;
		sgi_config.icfge.ctd = 0;

		if (!rc) {
			/* add new entry in the stream gate instance table */
			if (SWT_RxPSFPAddSGITableEntry(psfp->hw.handle, &sgi_config) != kStatus_Success) {
				rc = -GENAVB_ERR_SG_ADD;
				goto err_sgi_add;
			}
		} else {
			/* update existing entry in the stream gate instance table */
			if (SWT_RxPSFPUpdateSGITableEntry(psfp->hw.handle, &sgi_config) != kStatus_Success) {
				rc = -GENAVB_ERR_SG_UPDATE;
				goto err_sgi_update;
			}
		}

		/* notify dependencies */
		netc_sw_stream_gate_update_notify(psfp, sg);

		/* delete sgcl entries that are not used anymore */
		if (netc_sw_stream_gate_get_entry_state(psfp, index, &sgi_state)) {
			if (sgi_state.operSgclEID == NULL_ENTRY_ID) {
				/* we are sure previous admin eid(s) are not in use */
				if (psfp->sg_table[index].admin_eid != NULL_ENTRY_ID)
					SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, psfp->sg_table[index].admin_eid);

				if (psfp->sg_table[index].prev_admin_eid != NULL_ENTRY_ID)
					SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, psfp->sg_table[index].prev_admin_eid);

				psfp->sg_table[index].admin_eid = new_admin_eid;
				psfp->sg_table[index].prev_admin_eid = NULL_ENTRY_ID;
			} else if (sgi_state.operSgclEID == psfp->sg_table[index].prev_admin_eid) {
				/* we are sure previous admin_eid is not in use */
				if (psfp->sg_table[index].admin_eid != NULL_ENTRY_ID)
					SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, psfp->sg_table[index].admin_eid);

				psfp->sg_table[index].admin_eid = new_admin_eid;
			} else if (sgi_state.operSgclEID == psfp->sg_table[index].admin_eid) {
				/* we are sure previous prev_admin_eid is not in use */
				if (psfp->sg_table[index].prev_admin_eid != NULL_ENTRY_ID)
					SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, psfp->sg_table[index].prev_admin_eid);

				psfp->sg_table[index].prev_admin_eid = psfp->sg_table[index].admin_eid;
				psfp->sg_table[index].admin_eid = new_admin_eid;
			}
		}
		break;

	default:
		rc = -GENAVB_ERR_SG_NOT_SUPPORTED;
		goto err;
	}

	return GENAVB_SUCCESS;

err_sgi_add:
err_sgi_update:
	SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, new_admin_eid);

err:
	return rc;
}

static void netc_sw_stream_gate_delete_notify(struct net_psfp_ops *psfp_ops, uint32_t ref)
{
	netc_sw_stream_filter_delete_ref(psfp_ops, ref, STREAM_GATE_REF);
}

static int netc_sw_stream_gate_delete(struct net_psfp_ops *psfp_ops, uint32_t index)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_tb_sgcl_sgclse_t sgcl_state;
	netc_tb_sgi_config_t sgi_config;
	int rc = GENAVB_SUCCESS;

	/* notify dependencies */
	netc_sw_stream_gate_delete_notify(psfp_ops, index);

	sgi_config.entryID = index;
	sgi_config.acfge.adminSgclEID = NULL_ENTRY_ID;
	SWT_RxPSFPUpdateSGITableEntry(psfp->hw.handle, &sgi_config);

	/* remove stream gate entry from hardware table */
	if (SWT_RxPSFPDelSGITableEntry(psfp->hw.handle, index) != kStatus_Success) {
		rc = -GENAVB_ERR_SG_DELETE;
		goto err;
	}

	if (psfp->sg_table[index].prev_admin_eid != NULL_ENTRY_ID) {
		if (SWT_RxPSFPGetSGCLState(psfp->hw.handle, psfp->sg_table[index].prev_admin_eid, &sgcl_state) == kStatus_Success) {
			if (!sgcl_state.refCount) {
				if (SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, psfp->sg_table[index].prev_admin_eid) != kStatus_Success){
					rc = -GENAVB_ERR_SG_LIST_DELETE_ADMIN;
					goto err;
				}
			} else {
				os_log(LOG_ERR, "sgcl eid=%u cannot be deleted (refCount=%u)\n", psfp->sg_table[index].prev_admin_eid, sgcl_state.refCount);
				rc = -GENAVB_ERR_SG_LIST_DELETE_ADMIN;
				goto err;
			}
		}
	}

	psfp->sg_table[index].prev_admin_eid = NULL_ENTRY_ID;

	if (psfp->sg_table[index].admin_eid != NULL_ENTRY_ID) {
		if (SWT_RxPSFPGetSGCLState(psfp->hw.handle, psfp->sg_table[index].admin_eid, &sgcl_state) == kStatus_Success) {
			if (!sgcl_state.refCount) {
				if (SWT_RxPSFPDelSGCLTableEntry(psfp->hw.handle, psfp->sg_table[index].admin_eid) != kStatus_Success){
					rc = -GENAVB_ERR_SG_LIST_DELETE_ADMIN;
					goto err;
				}
			} else {
				os_log(LOG_ERR, "sgcl eid=%u cannot be deleted (refCount=%u)\n", psfp->sg_table[index].admin_eid, sgcl_state.refCount);
				rc = -GENAVB_ERR_SG_LIST_DELETE_ADMIN;
				goto err;
			}
		}
	}

	psfp->sg_table[index].admin_eid = NULL_ENTRY_ID;

err:
	return rc;
}

static int netc_sw_stream_gate_read(struct net_psfp_ops *psfp_ops, uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *sg)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_tb_sgi_config_t sgi_config;
	netc_tb_sgcl_gcl_t gcl;
	netc_tb_sgi_sgise_t sgise;
	uint32_t max_entries = sg->list_length;
	int i, rc = GENAVB_SUCCESS;

	if (index >= netc_sw_stream_gate_get_max_entries(psfp_ops)|| index >= NUM_STREAM_GATES_ENTRIES) {
		rc = -GENAVB_ERR_SG_MAX_ENTRIES;
		goto err;
	}

	if (netc_sw_stream_gate_get_entry(psfp, index, &sgi_config) <= 0) {
		rc = -GENAVB_ERR_SG_ENTRY_NOT_FOUND;
		goto err;
	}

	sg->stream_gate_instance_id = index;

	if (netc_sw_stream_gate_get_entry_state(psfp, index, &sgise) < 0) {
		rc = -GENAVB_ERR_SG_GET_STATE_ENTRY;
		goto err;
	}

	sg->cycle_time_extension = sgise.operCycleTimeExt;
	memcpy(&sg->base_time, &sgise.operBaseTime[0], sizeof(uint64_t));
	sg->gate_enable = sgise.state;
	sg->gate_closed_due_to_invalid_rx_enable = sgi_config.cfge.irxen;
	sg->gate_closed_due_to_invalid_rx = sgise.irx;
	sg->gate_closed_due_to_octets_exceeded_enable = sgi_config.cfge.oexen;
	sg->gate_closed_due_to_octets_exceeded = sgise.oex;

	sg->admin_gate_state = sgi_config.icfge.gst;
	sg->admin_ipv = (sgi_config.icfge.oipv) ? sgi_config.icfge.ipv : GENAVB_IPV_SPEC_NULL;

	gcl.gcList = rtos_malloc(max_entries * sizeof(netc_sgcl_gate_entry_t));
	if (!gcl.gcList) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto err;
	}

	if (type == GENAVB_SG_ADMIN) {
		if (netc_sw_stream_gate_control_get_entry(psfp, psfp->sg_table[index].admin_eid, &gcl, max_entries) < 0) {
			rc = -GENAVB_ERR_SG_LIST_GET_ADMIN_ENTRY;
			goto err_sgcl_entry;
		}
	} else {
		if ((sgise.state == kNETC_GSUseOperUntilAdminAct) || (sgise.state == kNETC_GSUseOperList)) {
			if (netc_sw_stream_gate_control_get_entry(psfp, sgise.operSgclEID, &gcl, max_entries) < 0) {
				rc = -GENAVB_ERR_SG_LIST_GET_OPER_ENTRY;
				goto err_sgcl_entry;
			}
		} else {
			rc = -GENAVB_ERR_SG_LIST_GET_OPER_ENTRY;
			goto err_sgcl_entry;
		}
	}

	sg->cycle_time_p = gcl.cycleTime;
	sg->cycle_time_q = NSECS_PER_SEC;
	sg->list_length = 0;

	if (gcl.numEntries) {
		for (i = 0; i < gcl.numEntries; i++) {
			sg->control_list[i].operation_name = GENAVB_SG_SET_GATE_AND_IPV;
			sg->control_list[i].gate_state_value = gcl.gcList[i].gtst;
			sg->control_list[i].ipv_spec = (gcl.gcList[i].oipv) ? gcl.gcList[i].ipv : GENAVB_IPV_SPEC_NULL;
			sg->control_list[i].time_interval_value = gcl.gcList[i].timeInterval;
			sg->control_list[i].interval_octet_max = gcl.gcList[i].iom;
			sg->list_length++;
		}
	}

err_sgcl_entry:
	rtos_free(gcl.gcList);

err:
	return rc;
}

static unsigned int netc_sw_flow_meter_get_max_entries(struct net_psfp_ops *psfp_ops)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);

	return SWT_RxPSFPGetRPTableMaxEntryNum(psfp->hw.handle);
}

static void netc_sw_flow_meter_update_notify(struct netc_psfp *psfp, uint32_t index, struct genavb_flow_meter_instance *fm)
{
	netc_sw_stream_filter_update_ref(psfp, fm->flow_meter_instance_id, FLOW_METER_REF);
}

static int netc_sw_flow_meter_update(struct net_psfp_ops *psfp_ops, uint32_t index, struct genavb_flow_meter_instance *fm, unsigned int option)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_tb_rp_config_t rp_config;
	int rc = GENAVB_SUCCESS;

	if (index >= netc_sw_flow_meter_get_max_entries(psfp_ops)) {
		rc = -GENAVB_ERR_FM_MAX_ENTRIES;
		goto err;
	}

	/* update flow meter hardware table */
	rp_config.entryID = index;

	switch (option) {
	case FM_UPDATE_OPTION_RESET_MR:
		if (SWT_RxPSFPResetMRRPTableEntry(psfp->hw.handle, rp_config.entryID) != kStatus_Success) {
			rc = -GENAVB_ERR_FM_RESET_MR;
			goto err;
		}
		break;

	case FM_UPDATE_OPTION_CONFIG:
		rp_config.cfge.cir = (fm->committed_information_rate / RATE_POLICER_BITS_PER_SEC);
		rp_config.cfge.cbs = fm->committed_burst_size;
		rp_config.cfge.eir = (fm->excess_information_rate / RATE_POLICER_BITS_PER_SEC);
		rp_config.cfge.ebs = fm->excess_burst_size;
		rp_config.cfge.cf = fm->coupling_flag;
		rp_config.cfge.cm = fm->color_mode;
		rp_config.cfge.doy = fm->drop_on_yellow;
		rp_config.cfge.mren = fm->mark_all_frames_red_enable;
		rp_config.cfge.sduType = kNETC_MPDU;
		rp_config.cfge.ndor = 0;
		rp_config.fee.fen = 1;

		if (SWT_RxPSFPAddOrUpdateRPTableEntry(psfp->hw.handle, &rp_config) != kStatus_Success) {
			rc = -GENAVB_ERR_FM_UPDATE;
			goto err;
		}

		/* notify dependencies */
		netc_sw_flow_meter_update_notify(psfp, index, fm);
		break;

	default:
		rc = -GENAVB_ERR_FM_NOT_SUPPORTED;
		goto err;
	}

err:
	return rc;
}

static void netc_sw_flow_meter_delete_notify(struct net_psfp_ops *psfp_ops, uint32_t ref)
{
	netc_sw_stream_filter_delete_ref(psfp_ops, ref, FLOW_METER_REF);
}

static int netc_sw_flow_meter_delete(struct net_psfp_ops *psfp_ops, uint32_t index)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	int rc = GENAVB_SUCCESS;

	/* notify dependencies */
	netc_sw_flow_meter_delete_notify(psfp_ops, index);

	/* remove stream flow meter entry from hardware table */
	if (SWT_RxPSFPDelRPTableEntry(psfp->hw.handle, index) != kStatus_Success)
		rc = -GENAVB_ERR_FM_DELETE;

	return rc;
}

static int netc_sw_flow_meter_read(struct net_psfp_ops *psfp_ops, uint32_t index, struct genavb_flow_meter_instance *fm)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);
	netc_tb_rp_rsp_data_t rsp;
	int rc = GENAVB_SUCCESS;

	if (index >= netc_sw_flow_meter_get_max_entries(psfp_ops)) {
		rc = -GENAVB_ERR_FM_MAX_ENTRIES;
		goto err;
	}

	if (netc_sw_flow_meter_find_entry(psfp, index, &rsp) <= 0) {
		rc = -GENAVB_ERR_FM_ENTRY_NOT_FOUND;
		goto err;
	}

	fm->flow_meter_instance_id = index;
	fm->committed_information_rate = (rsp.cfge.cir * RATE_POLICER_BITS_PER_SEC);
	fm->committed_burst_size = rsp.cfge.cbs;
	fm->excess_information_rate = (rsp.cfge.eir * RATE_POLICER_BITS_PER_SEC);
	fm->excess_burst_size = rsp.cfge.ebs;
	fm->coupling_flag = rsp.cfge.cf;
	fm->color_mode = rsp.cfge.cm;
	fm->drop_on_yellow = rsp.cfge.doy;
	fm->mark_all_frames_red_enable = rsp.cfge.mren;
	/* read current state in the HW, i.e. if dropping or not all frames due to red frame detected */
	fm->mark_all_frames_red = rsp.pse.mr;

err:
	return rc;
}

__exit void netc_sw_psfp_exit(struct net_psfp_ops *psfp_ops)
{
	rtos_free(psfp_ops->sg_base_time);
}

__init int netc_sw_psfp_init(struct net_psfp_ops *psfp_ops)
{
	struct netc_psfp *psfp = netc_psfp_drv(psfp_ops);

	int rc = 0;
	int i;

	psfp_ops->stream_filter_update = &netc_sw_stream_filter_update;
	psfp_ops->stream_filter_delete = &netc_sw_stream_filter_delete;
	psfp_ops->stream_filter_read = &netc_sw_stream_filter_read;
	psfp_ops->stream_filter_get_max_entries = &netc_sw_stream_filter_get_max_entries;

	psfp_ops->stream_gate_update = &netc_sw_stream_gate_update;
	psfp_ops->stream_gate_delete = &netc_sw_stream_gate_delete;
	psfp_ops->stream_gate_read = &netc_sw_stream_gate_read;
	psfp_ops->stream_gate_get_max_entries = &netc_sw_stream_gate_get_max_entries;
	psfp_ops->stream_gate_control_get_max_entries = &netc_sw_stream_gate_control_get_max_entries;
	psfp_ops->sg_base_time = rtos_malloc(sizeof(uint64_t) * netc_sw_stream_gate_get_max_entries(psfp_ops));
	if (!psfp_ops->sg_base_time) {
		os_log(LOG_ERR, "rtos_malloc() failed\n");
		rc = -1;
		goto err;
	}

	psfp_ops->flow_meter_update = &netc_sw_flow_meter_update;
	psfp_ops->flow_meter_delete = &netc_sw_flow_meter_delete;
	psfp_ops->flow_meter_read = &netc_sw_flow_meter_read;
	psfp_ops->flow_meter_get_max_entries = &netc_sw_flow_meter_get_max_entries;

	psfp->max_stream_filters_instances = netc_sw_stream_filter_get_max_entries(psfp_ops);

	for (i = 0; i < psfp->max_stream_filters_instances; i++) {
		psfp->stream_filters[i].flow_meter_ref = NULL_ENTRY_ID;
		psfp->stream_filters[i].stream_gate_ref = NULL_ENTRY_ID;
		psfp->stream_filters[i].sf_eid = NULL_ENTRY_ID;
		psfp->stream_filters[i].isc_eid = NULL_ENTRY_ID;
	}

	for (i = 0; i < NUM_STREAM_GATES_ENTRIES; i++) {
		psfp->sg_table[i].prev_admin_eid = NULL_ENTRY_ID;
		psfp->sg_table[i].admin_eid = NULL_ENTRY_ID;
	}

err:
	return rc;
}
#endif
