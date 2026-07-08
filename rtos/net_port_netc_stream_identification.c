/*
* Copyright 2023-2026 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "config.h"

#include "net_logical_port.h"

#include "common/log.h"
#include "genavb/error.h"
#include "genavb/ether.h"

#if CFG_NUM_NETC_SW

#include "net_port_netc_stream_identification.h"
#include "net_port_netc_sw_drv.h"
#include "net_port_netc_psfp.h"
#include "net_port_netc_sw_frer.h"

#include "fsl_netc.h"
#include "fsl_netc_switch.h"

#define INTERNAL_VID_BASE NETC_INTERNAL_VID_BASE

/* Internal Egress Frame Modification ID base */
#define INTERNAL_EFM_BASE 32

#define ISI_SUPPORT (1 << 0)
#define IPF_SUPPORT (1 << 1)

static int netc_si_update_is_table_entry(struct netc_sw_drv *drv, uint32_t is_eid, void (*update_entry)(netc_tb_is_config_t *entry, void *data), void *data);

struct si_sf_params {
	uint32_t rp_eid;
	uint32_t sgi_eid;
	uint32_t isc_eid;
	uint16_t msdu;
	bool sf_enable;
};

struct si_frer_params {
	uint32_t isqg_eid;
	uint32_t et_eid;
	uint32_t et_port_map;
};

struct si_ifm_params {
	uint32_t vid;
	uint32_t et_eid;
	uint8_t action;
};

struct si_efm_params {
#ifdef CONFIG_GENAVB_TSN_DSA
	uint32_t dsa_vid;
#endif
	uint32_t vid;
	uint8_t action;
};

#define ACTION_TAG_OPERATION(vlan_oper, rtag_oper) (((vlan_oper) & 0x3) | (((rtag_oper) & 0x1) << 2))
#define ACTION_VLAN_ADD		ACTION_TAG_OPERATION(1, 0)
#define ACTION_VLAN_REMOVE	ACTION_TAG_OPERATION(2, 0)
#define ACTION_VLAN_REPLACE	ACTION_TAG_OPERATION(3, 0)
#define ACTION_RTAG_REMOVE	ACTION_TAG_OPERATION(0, 1)

#define ACTION_TAG2VLAN_OPERATION(oper) ((oper) & 0x3)
#define ACTION_TAG2RTAG_OPERATION(oper) (((oper) >> 2) & 0x1)

#define IS_NETC_EFM_ENCODE_OPTION_1		(1U << 13U)
#define IS_NETC_EFM_ENCODE_OPTION_2		(1U << 14U)
#define IS_NETC_EFM_ENCODE_OPTION_1_SQTA(efmid)	((efmid >> 2U) & 0x7U)
#define IS_NETC_EFM_ENCODE_OPTION_1_VUDA(efmid)	(efmid & 0x3U)
#define IS_NETC_EFM_ENCODE_OPTION_2_VARA(efmid)	((efmid >> 12U) & 0x3U)
#define IS_NETC_EFM_ENCODE_OPTION_2_VID(efmid)	(efmid & 0xFFFU)

typedef enum {
	VLAN_TYPE_TAGGED_VID0 = 0,
	VLAN_TYPE_TAGGED_VIDx,
	VLAN_TYPE_PRIORITY,
	VLAN_TYPE_ALL_VID0,
	VLAN_TYPE_ALL_VIDx
} vlan_type_t;

static const uint8_t si_type_support[] = {
	[GENAVB_SI_NULL] = IPF_SUPPORT | ISI_SUPPORT,
	[GENAVB_SI_SRC_MAC_VLAN] = IPF_SUPPORT | ISI_SUPPORT,
	[GENAVB_SI_DST_MAC_VLAN] = 0,
	[GENAVB_SI_IP] = 0,
	[GENAVB_SI_MASK_AND_MATCH] = 0
};

static const uint8_t si_vlan_type_support[] = {
	[VLAN_TYPE_TAGGED_VID0] = IPF_SUPPORT,
	[VLAN_TYPE_TAGGED_VIDx] = IPF_SUPPORT | ISI_SUPPORT,
	[VLAN_TYPE_PRIORITY] = IPF_SUPPORT | ISI_SUPPORT,
	[VLAN_TYPE_ALL_VID0] = IPF_SUPPORT,
	[VLAN_TYPE_ALL_VIDx] = IPF_SUPPORT | ISI_SUPPORT,
};

static const uint8_t si_vlan_type_entries[] = {
	[VLAN_TYPE_TAGGED_VID0] = 1,
	[VLAN_TYPE_TAGGED_VIDx] = 1,
	[VLAN_TYPE_PRIORITY] = 2,
	[VLAN_TYPE_ALL_VID0] = 1,
	[VLAN_TYPE_ALL_VIDx] = 2,
};

/* Port ingress stream identification configuration
 * for all ports define 2 key construction rules for first and second lookup
 * - first key is used for Null stream identification
 * - second for Source mac Vlan stream identification
 * since stream identification is used per port, both keys contain
 * source port
 */
static const uint8_t si_type_isi_key[] = {
	[GENAVB_SI_NULL] = kNETC_KCRule0,
	[GENAVB_SI_SRC_MAC_VLAN] = kNETC_KCRule1,
	[GENAVB_SI_DST_MAC_VLAN] = kNETC_KCRule0,
	[GENAVB_SI_IP] = -1,
	[GENAVB_SI_MASK_AND_MATCH] = -1
};

static const netc_isi_kc_rule_t key0 = {
	.dmacp = 1, /* destination MAC present in the key */
	.ovidp = 1, /* outer VLAN ID present in the key */
	.portp = 1, /* source port present in the key */
	.valid = 1,
};

static const netc_isi_kc_rule_t key1 = {
	.smacp = 1, /* source MAC address present in the key */
	.ovidp = 1, /* outer VLAN ID present in the key */
	.portp = 1, /* source port present in the key */
	.valid = 1,
};

static vlan_type_t si_vlan_type(struct genavb_stream_identity *entry)
{
	uint16_t vlan;
	genavb_si_vlan_tag_t tagged;
	vlan_type_t type;

	switch (entry->type) {
	case GENAVB_SI_NULL:
	default:
		tagged = entry->parameters.null.tagged;
		vlan = entry->parameters.null.vlan;
		break;

	case GENAVB_SI_SRC_MAC_VLAN:
		tagged = entry->parameters.smac_vlan.tagged;
		vlan = entry->parameters.smac_vlan.vlan;
		break;

	case GENAVB_SI_DST_MAC_VLAN:
		tagged = entry->parameters.dmac_vlan.down.tagged;
		vlan = entry->parameters.dmac_vlan.down.vlan;
		break;
	}

	switch (tagged) {
	case GENAVB_SI_TAGGED:
	default:
		if (!vlan)
			type = VLAN_TYPE_TAGGED_VID0;
		else
			type = VLAN_TYPE_TAGGED_VIDx;

		break;

	case GENAVB_SI_PRIORITY:
		type = VLAN_TYPE_PRIORITY;
		break;

	case GENAVB_SI_ALL:
		if (!vlan)
			type = VLAN_TYPE_ALL_VID0;
		else
			type = VLAN_TYPE_ALL_VIDx;

		break;
	}

	return type;
}

static bool si_supported_by_isi(struct genavb_stream_identity *entry)
{
	if (!(si_type_support[entry->type] & ISI_SUPPORT))
		goto not_supported;

	if (!(si_vlan_type_support[si_vlan_type(entry)] & ISI_SUPPORT))
		goto not_supported;

	return true;

not_supported:
	return false;
}

static bool si_supported_by_ipf(struct genavb_stream_identity *entry)
{
	if (!(si_type_support[entry->type] & IPF_SUPPORT))
		goto not_supported;

	if (!(si_vlan_type_support[si_vlan_type(entry)] & IPF_SUPPORT))
		goto not_supported;

	return true;

not_supported:
	return false;
}

static void netc_si_set_used(struct netc_si_entry *si_entry, bool used)
{
	if (used)
		si_entry->used = 1;
	else
		si_entry->used = 0;
}

static bool netc_si_is_used(struct netc_si_entry *si_entry)
{
	return (si_entry->used == 1);
}

static unsigned int netc_si_get_is_references(struct netc_si *si, uint32_t handle)
{
	unsigned int used = 0;
	int i;

	for (i = 0; i < SI_MAX_ENTRIES; i++) {
		struct netc_si_entry *si_entry = &si->entry[i];

		if (netc_si_is_used(si_entry) && si_entry->handle == handle)
			used++;
	}

	return used;
}

static void netc_si_set_type(struct netc_si_entry *si_entry, genavb_si_t type)
{
	si_entry->type = type;
}

static genavb_si_t netc_si_get_type(struct netc_si_entry *si_entry)
{
	return si_entry->type;
}

static void netc_si_encode_isi_vlan_tag(uint8_t *framekey, vlan_type_t vlan_type, uint16_t vlan, unsigned int id)
{
	switch (vlan_type) {
	case VLAN_TYPE_TAGGED_VIDx:
		framekey[0] = (1 << 7) | (vlan >> 8);
		framekey[1] = (vlan & 0x00FF);

		break;

	case VLAN_TYPE_PRIORITY:
		/* To match an untagged frame, set V = 0, PCP = 0 and VID = 0 in key construction. */
		/* FIXME this matches only untagged frames, and should match untagged or tagged with VID = 0 */
		if (!id) {
			/* untagged */
			framekey[0] = 0x00;
			framekey[1] = 0x00;
		} else {
			/* tagged with VID = 0 */
			framekey[0] = (1 << 7);
			framekey[1] = 0x0;
		}

		break;

	case VLAN_TYPE_ALL_VIDx:
		if (!id) {
			/* tagged with VID = vlan */
			framekey[0] = (1 << 7) | (vlan >> 8);
			framekey[1] = (vlan & 0x00FF);
		} else {
			/* untagged */
			framekey[0] = 0x00;
			framekey[1] = 0x00;
		}

		break;

	default:
		break;
	}
}

static uint16_t netc_si_get_vid(struct genavb_stream_identity *entry)
{
	switch (entry->type) {
	case GENAVB_SI_NULL:
		if (entry->parameters.null.tagged == GENAVB_SI_TAGGED)
			return entry->parameters.null.vlan;
		else
			return 1;
		break;
	case GENAVB_SI_SRC_MAC_VLAN:
		if (entry->parameters.smac_vlan.tagged == GENAVB_SI_TAGGED)
			return entry->parameters.smac_vlan.vlan;
		else
			return 1;
		break;
	default:
		break;
	}

	return 0;
}

static void netc_si_encode_isi_entry_key(netc_tb_isi_keye_t *keye, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, unsigned int port_id, unsigned int id)
{
	switch (si_entry->type) {
	case GENAVB_SI_NULL:
		memcpy(&keye->framekey[0], &entry->parameters.null.destination_mac[0], 6);
		netc_si_encode_isi_vlan_tag(&keye->framekey[6], si_entry->vlan_type, (entry->parameters.null.vlan & 0x0FFF), id);
		break;

	case GENAVB_SI_SRC_MAC_VLAN:
		memcpy(&keye->framekey[0], &entry->parameters.smac_vlan.source_mac[0], 6);
		netc_si_encode_isi_vlan_tag(&keye->framekey[6], si_entry->vlan_type, (entry->parameters.smac_vlan.vlan & 0x0FFF), id);
		break;

	case GENAVB_SI_DST_MAC_VLAN:
		memcpy(&keye->framekey[0], &entry->parameters.dmac_vlan.down.destination_mac[0], 6);
		netc_si_encode_isi_vlan_tag(&keye->framekey[6], si_entry->vlan_type, (entry->parameters.dmac_vlan.down.vlan & 0x0FFF), id);
		break;

	default:
		break;
	}

	keye->keyType = si_type_isi_key[si_entry->type];

	/* Source port */
	keye->srcPortID = port_id;
}

static void netc_si_decode_isi_vlan_tag(uint8_t *framekey, vlan_type_t vlan_type, genavb_si_vlan_tag_t *tagged, uint16_t *vlan)
{
	switch (vlan_type) {
	case VLAN_TYPE_TAGGED_VIDx:
		*tagged = GENAVB_SI_TAGGED;
		*vlan = ((framekey[0] & ~(1 << 7)) << 8) | framekey[1];

		break;

	case VLAN_TYPE_PRIORITY:
		*tagged = GENAVB_SI_PRIORITY;
		*vlan = 0;

		break;

	case VLAN_TYPE_ALL_VIDx:
		*tagged = GENAVB_SI_ALL;
		*vlan = ((framekey[0] & ~(1 << 7)) << 8) | framekey[1];

		break;

	default:
		break;
	}
}

static void netc_si_decode_isi_entry_key(netc_tb_isi_keye_t *keye, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry)
{
	switch (si_entry->type) {
	case GENAVB_SI_NULL:
		memcpy(entry->parameters.null.destination_mac, &keye->framekey[0], 6);
		netc_si_decode_isi_vlan_tag(&keye->framekey[6], si_entry->vlan_type, &entry->parameters.null.tagged, &entry->parameters.null.vlan);
		break;

	case GENAVB_SI_SRC_MAC_VLAN:
		memcpy(entry->parameters.smac_vlan.source_mac, &keye->framekey[0], 6);
		netc_si_decode_isi_vlan_tag(&keye->framekey[6], si_entry->vlan_type, &entry->parameters.smac_vlan.tagged, &entry->parameters.smac_vlan.vlan);
		break;

	case GENAVB_SI_DST_MAC_VLAN:
		memcpy(entry->parameters.dmac_vlan.down.destination_mac, &keye->framekey[0], 6);
		netc_si_decode_isi_vlan_tag(&keye->framekey[6], si_entry->vlan_type, &entry->parameters.dmac_vlan.down.tagged, &entry->parameters.dmac_vlan.down.vlan);
		break;

	default:
		break;
	}
}

static void netc_si_encode_vlan_ipf_entry(netc_tb_ipf_keye_t *keye, vlan_type_t vlan_type, uint16_t vlan, unsigned int id)
{
	switch (vlan_type) {
	case VLAN_TYPE_TAGGED_VID0:
		/* tagged VID = any */
		keye->frameAttr.outerVlan = 1;

		keye->outerVlanTCI = VLAN_LABEL(0, 0, 0);
		keye->outerVlanTCIMask = VLAN_LABEL(0, 0, 0);

		keye->frameAttrMask = kNETC_IPFOuterVlanMask;

		break;

	case VLAN_TYPE_TAGGED_VIDx:
		/* tagged VID = vlan */
		keye->frameAttr.outerVlan = 1;

		keye->outerVlanTCI = VLAN_LABEL(vlan, 0, 0);
		keye->outerVlanTCIMask = VLAN_LABEL(0x3ff, 0, 0);

		keye->frameAttrMask = kNETC_IPFOuterVlanMask;

		break;

	case VLAN_TYPE_PRIORITY:
		if (!id) {
			/* untagged */
			keye->frameAttr.outerVlan = 0;
			keye->frameAttrMask = kNETC_IPFOuterVlanMask;
		} else {
			/* tagged VID = 0 */
			keye->frameAttr.outerVlan = 1;

			keye->outerVlanTCI = VLAN_LABEL(0, 0, 0);
			keye->outerVlanTCIMask = VLAN_LABEL(0x3ff, 0, 0);

			keye->frameAttrMask = kNETC_IPFOuterVlanMask;
		}

		break;

	case VLAN_TYPE_ALL_VID0:
		keye->frameAttrMask = 0;

		break;

	case VLAN_TYPE_ALL_VIDx:
		if (!id) {
			/* tagged VID = vlan */
			keye->frameAttr.outerVlan = 1;

			keye->outerVlanTCI = VLAN_LABEL(vlan, 0, 0);
			keye->outerVlanTCIMask = VLAN_LABEL(0x3ff, 0, 0);

			keye->frameAttrMask = kNETC_IPFOuterVlanMask;
		} else {
			/* untagged */
			keye->frameAttr.outerVlan = 0;
			keye->frameAttrMask = kNETC_IPFOuterVlanMask;
		}

		break;

	default:
		break;
	}
}

static void netc_si_encode_ipf_entry(netc_tb_ipf_keye_t *keye, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, unsigned int port_id, unsigned int id)
{
	memset(keye, 0, sizeof(*keye));

	keye->srcPort = port_id;
	keye->srcPortMask = 0x1f;

	switch (si_entry->type) {
	case GENAVB_SI_NULL:
	default:
		memcpy(keye->dmac, entry->parameters.null.destination_mac, 6);
		memset(keye->dmacMask, 0xff, 6);

		netc_si_encode_vlan_ipf_entry(keye, si_entry->vlan_type, entry->parameters.null.vlan, id);

		break;

	case GENAVB_SI_DST_MAC_VLAN:
		memcpy(keye->dmac, entry->parameters.dmac_vlan.down.destination_mac, 6);
		memset(keye->dmacMask, 0xff, 6);

		netc_si_encode_vlan_ipf_entry(keye, si_entry->vlan_type, entry->parameters.dmac_vlan.down.vlan, id);

		break;

	case GENAVB_SI_SRC_MAC_VLAN:
		memcpy(keye->smac, entry->parameters.smac_vlan.source_mac, 6);
		memset(keye->smacMask, 0xff, 6);

		netc_si_encode_vlan_ipf_entry(keye, si_entry->vlan_type, entry->parameters.smac_vlan.vlan, id);

		break;
	}
}

static void netc_si_decode_ipf_vlan_tag(netc_tb_ipf_keye_t *keye, vlan_type_t vlan_type, genavb_si_vlan_tag_t *tagged, uint16_t *vlan)
{
	switch (vlan_type) {
	case VLAN_TYPE_TAGGED_VID0:
		*tagged = GENAVB_SI_TAGGED;
		*vlan = 0;

		break;

	case VLAN_TYPE_TAGGED_VIDx:
		*tagged = GENAVB_SI_TAGGED;
		*vlan = ntohs(keye->outerVlanTCI) & VLAN_TCI_VID_MASK;

		break;

	case VLAN_TYPE_PRIORITY:
		*tagged = GENAVB_SI_PRIORITY;
		*vlan = 0;

		break;

	case VLAN_TYPE_ALL_VID0:
		*tagged = GENAVB_SI_ALL;
		*vlan = 0;

		break;

	case VLAN_TYPE_ALL_VIDx:
		*tagged = GENAVB_SI_ALL;
		*vlan = ntohs(keye->outerVlanTCI) & VLAN_TCI_VID_MASK;

	default:
		break;
	}
}

static void netc_si_decode_ipf_entry_key(netc_tb_ipf_keye_t *keye, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry)
{
	switch (si_entry->type) {
	case GENAVB_SI_NULL:
	default:
		memcpy(entry->parameters.null.destination_mac, keye->dmac, 6);

		netc_si_decode_ipf_vlan_tag(keye, si_entry->vlan_type, &entry->parameters.null.tagged, &entry->parameters.null.vlan);

		break;

	case GENAVB_SI_DST_MAC_VLAN:
		memcpy(entry->parameters.dmac_vlan.down.destination_mac, keye->dmac, 6);

		netc_si_decode_ipf_vlan_tag(keye, si_entry->vlan_type, &entry->parameters.dmac_vlan.down.tagged, &entry->parameters.dmac_vlan.down.vlan);

		break;

	case GENAVB_SI_SRC_MAC_VLAN:
		memcpy(entry->parameters.smac_vlan.source_mac, keye->smac, 6);

		netc_si_decode_ipf_vlan_tag(keye, si_entry->vlan_type, &entry->parameters.smac_vlan.tagged, &entry->parameters.smac_vlan.vlan);

		break;
	}
}

static int netc_si_add_isi_table_entry(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, unsigned int port_id, unsigned int id)
{
	netc_tb_isi_config_t isi_entry = {0};
	netc_tb_isi_rsp_data_t isi_rsp;

	if (!entry)
		goto err;

	/* Ingress Stream Table id */
	/* IS_EID = handle */
	isi_entry.cfge.iSEID = entry->handle;

	netc_si_encode_isi_entry_key(&isi_entry.keye, si_entry, entry, port_id, id);

	if (SWT_RxPSFPQueryISITableEntryWithKey(si->hw.handle, &isi_entry.keye, &isi_rsp) == kStatus_Success) {
		os_log(LOG_ERR, "Error: entry already exists\n");
		goto err;
	}

	/* Ingress Stream Identification Table */
	if (SWT_RxPSFPAddISITableEntry(si->hw.handle, &isi_entry, &si_entry->port[port_id].eid[id]) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPAddISITableEntry() failed\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_si_delete_isi_table_entry(struct netc_si *si, uint32_t isi_eid)
{
	if (SWT_RxPSFPDelISITableEntry(si->hw.handle, isi_eid) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPDelISITableEntry() failed\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_si_update_isi_table_entry(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, unsigned int port_id, unsigned int id)
{
	netc_tb_isi_config_t new_isi_entry = {0};
	netc_tb_isi_config_t old_isi_entry = {0};

	if (!entry)
		goto err;

	/* construct new framekey */
	netc_si_encode_isi_entry_key(&new_isi_entry.keye, si_entry, entry, port_id, id);

	/* get programmed framekey */
	if (SWT_RxPSFPQueryISITableEntry(si->hw.handle, si_entry->port[port_id].eid[id], &old_isi_entry) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPQueryISITableEntry failed\n");
		goto err;
	}

	/* compare and update if differs */
	if (memcmp(old_isi_entry.keye.framekey, new_isi_entry.keye.framekey, sizeof(old_isi_entry.keye.framekey)) != 0) {
		if (netc_si_delete_isi_table_entry(si, si_entry->port[port_id].eid[id]) < 0)
			goto err;

		if (netc_si_add_isi_table_entry(si, si_entry, entry, port_id, id) < 0)
			goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_si_add_ipf_table_entry(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, unsigned int port_id, unsigned int id)
{
	netc_tb_ipf_config_t config = { 0 };

	netc_si_encode_ipf_entry(&config.keye, si_entry, entry, port_id, id);

	config.cfge.fltfa = kNETC_IPFForwardPermit;
	config.cfge.flta = kNETC_IPFWithIngressStream;
	config.cfge.fltaTgt = entry->handle;

	if (SWT_RxIPFAddTableEntry(si->hw.handle, &config, &si_entry->port[port_id].eid[id]) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxIPFAddTableEntry failed\n");
		goto err;
	}

	return 0;

err:
	return -1;
}

static int netc_si_delete_ipf_table_entry(struct netc_si *si, uint32_t ipf_eid)
{
	if (SWT_RxIPFDelTableEntry(si->hw.handle, ipf_eid) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxIPFDelTableEntry failed\n");
		goto err;
	}

	return 0;

err:
	return -1;
}

static int netc_si_update_ipf_table_entry(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, unsigned int port_id, unsigned int id)
{
	if (netc_si_delete_ipf_table_entry(si, si_entry->port[port_id].eid[id]) < 0)
		goto err;

	if (netc_si_add_ipf_table_entry(si, si_entry, entry, port_id, id) < 0)
		goto err;

	return 0;

err:
	return -1;
}

static int netc_si_update_isi_ipf_table(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry, uint32_t port_mask)
{
	uint32_t old_port_mask = si_entry->port_mask;
	uint32_t add_port_mask, delete_port_mask, update_port_mask, diff_mask;
	unsigned int id;
	int rc = 0;
	int port;

	diff_mask = port_mask ^ old_port_mask;
	add_port_mask = port_mask & diff_mask;
	update_port_mask = port_mask & old_port_mask;
	delete_port_mask = old_port_mask & diff_mask;

	for (port = 0; port < si->total_ports; port++) {
		for (id = 0; id < si_vlan_type_entries[si_entry->vlan_type]; id++) {
			if (update_port_mask & (1 << port)) {
				if (si_entry->use_isi)
					rc = netc_si_update_isi_table_entry(si, si_entry, entry, port, id);
				else
					rc = netc_si_update_ipf_table_entry(si, si_entry, entry, port, id);

				if (rc < 0)
					goto err;

				si_entry->port_mask |= (1 << port);

			} else if (add_port_mask & (1 << port)) {
				if (si_entry->use_isi)
					rc = netc_si_add_isi_table_entry(si, si_entry, entry, port, id);
				else
					rc = netc_si_add_ipf_table_entry(si, si_entry, entry, port, id);

				if (rc < 0)
					goto err;

				si_entry->port_mask |= (1 << port);

			} else if (delete_port_mask & (1 << port)) {
				if (si_entry->use_isi)
					rc = netc_si_delete_isi_table_entry(si, si_entry->port[port].eid[id]);
				else
					rc = netc_si_delete_ipf_table_entry(si, si_entry->port[port].eid[id]);

				if (rc < 0)
					goto err;

				si_entry->port_mask &= ~(1 << port);
			}
		}
	}

err:
	return rc;
}

static int netc_si_read_isi_table(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry)
{
	netc_tb_isi_config_t config = {0};
	int i;

	for (i = 0; i < si->total_ports; i++) {
		if (si_entry->port_mask & (1 << i)) {

			/*  ISI Entries for all ports differs only on srcPortID,
			*  to fill the API 'parameters' field we need only one of them,
			*  Query only the first one */
			if (SWT_RxPSFPQueryISITableEntry(si->hw.handle, si_entry->port[i].eid[0], &config) != kStatus_Success) {
				os_log(LOG_ERR, "SWT_RxPSFPQueryISITableEntry failed\n");
				goto err;
			}

			netc_si_decode_isi_entry_key(&config.keye, si_entry, entry);

			break;
		}
	}

	return 0;

err:
	return -1;
}

static int netc_si_read_ipf_table(struct netc_si *si, struct netc_si_entry *si_entry, struct genavb_stream_identity *entry)
{
	netc_tb_ipf_config_t ipf_entry;
	int i;

	for (i = 0; i < si->total_ports; i++) {
		if (si_entry->port_mask & (1 << i)) {

			if (SWT_RxIPFQueryTableEntry(si->hw.handle, si_entry->port[i].eid[0], &ipf_entry) != kStatus_Success) {
				os_log(LOG_ERR, "SWT_RxIPFQueryTableEntry failed\n");
				goto err;
			}

			netc_si_decode_ipf_entry_key(&ipf_entry.keye, si_entry, entry);

			break;
		}
	}

	return 0;

err:
	return -1;
}

static void netc_si_set_sf_ref(netc_tb_is_config_t *is_table_entry, uint32_t rp_eid, uint32_t sgi_eid, uint32_t isc_eid, bool sf_enable, uint16_t msdu)
{
	/* Stream Filtering Enable: additional lookup in Ingress Stream Filter Table */
	/* Stream Filtering Disable: No additional lookup, RP_EID and SGI_EID come from Ingress Stream Table */
	is_table_entry->cfge.sfe = sf_enable;

	is_table_entry->cfge.rpEID = rp_eid;
	is_table_entry->cfge.sgiEID = sgi_eid;
	is_table_entry->cfge.iscEID = isc_eid;
	is_table_entry->cfge.msdu = msdu;

	if (rp_eid != NULL_ENTRY_ID)
		is_table_entry->cfge.orp = 1; /* Override default Rate Policer */
	else
		is_table_entry->cfge.orp = 0;

	if (sgi_eid != NULL_ENTRY_ID)
		is_table_entry->cfge.osgi = 1; /* Override default Stream Gate */
	else
		is_table_entry->cfge.osgi = 0;
}

static void netc_si_update_sf_params(netc_tb_is_config_t *entry, void *data)
{
	struct si_sf_params *params = data;

	netc_si_set_sf_ref(entry, params->rp_eid, params->sgi_eid, params->isc_eid, params->sf_enable, params->msdu);
}

int netc_si_update_sf_ref(void *drv, uint32_t handle)
{
	struct si_sf_params params;

	params.sf_enable = false;
	params.rp_eid = params.sgi_eid = params.isc_eid = NULL_ENTRY_ID;
	params.msdu = 0;

	if (netc_sw_psfp_get_eid(drv, handle, &params.rp_eid, &params.sgi_eid, &params.isc_eid, &params.sf_enable, &params.msdu) < 0)
		goto err;

	return netc_si_update_is_table_entry(drv, handle, &netc_si_update_sf_params, &params);

err:
	return -1;
}

static void netc_si_set_frer_ref(netc_tb_is_config_t *entry, uint32_t isqg_eid, uint32_t et_eid, uint32_t et_port_map)
{
	entry->cfge.isqEID = isqg_eid;
	if (isqg_eid != NULL_ENTRY_ID)
		entry->cfge.isqa = kNETC_ISPerformFRER; /* Ingress Sequence Action: Perform FRER */
	else
		entry->cfge.isqa = kNETC_ISNotPerformFRER;

	/* This doesn't work, overwritten by FDB */
	entry->cfge.etEID = et_eid;
	entry->cfge.ePortBitmap = et_port_map;
}

static void netc_si_update_frer_params(netc_tb_is_config_t *entry, void *data)
{
	struct si_frer_params *params = data;

	netc_si_set_frer_ref(entry, params->isqg_eid, params->et_eid, params->et_port_map);
}

int netc_si_update_frer_ref(void *sw_drv, uint32_t handle)
{
	struct netc_sw_drv *drv = (struct netc_sw_drv *)sw_drv;
	struct si_frer_params params;

	params.isqg_eid = params.et_eid = NULL_ENTRY_ID;
	params.et_port_map = 0;

	if (netc_sw_frer_get_eid(drv, handle, &params.isqg_eid, &params.et_eid, &params.et_port_map) < 0)
		goto err;

	return netc_si_update_is_table_entry(drv, handle, &netc_si_update_frer_params, &params);

err:
	return -1;
}

static int netc_si_get_is_table_refs(struct netc_si *si, uint32_t handle, netc_tb_is_config_t *is_table_entry)
{
	uint32_t rp_eid, sgi_eid, isc_eid;
	uint32_t isqg_eid, et_eid, et_port_map;
	uint16_t msdu;
	bool sf_enable;

	/* PSFP */
	sf_enable = false;
	rp_eid = sgi_eid = isc_eid = NULL_ENTRY_ID;
	msdu = 0;

	if (netc_sw_psfp_get_eid(si->drv.sw, handle, &rp_eid, &sgi_eid, &isc_eid, &sf_enable, &msdu) < 0)
		goto err;

	netc_si_set_sf_ref(is_table_entry, rp_eid, sgi_eid, isc_eid, sf_enable, msdu);

	/* FRER */
	isqg_eid = et_eid = NULL_ENTRY_ID;
	et_port_map = 0;

	if (netc_sw_frer_get_eid(si->drv.sw, handle, &isqg_eid, &et_eid, &et_port_map) < 0)
		goto err;

	netc_si_set_frer_ref(is_table_entry, isqg_eid, et_eid, et_port_map);

	return 0;

err:
	return -1;
}

static int netc_si_add_internal_vft_entry(struct netc_si *si, uint16_t org_vid, uint16_t new_vid, uint32_t base_et_eid)
{
	uint32_t vft_entry_id;
	netc_tb_vf_config_t config = {0};
	uint32_t entry_id = NULL_ENTRY_ID;

	/*
	 * NOTE: this function assume that original vid entry exists:
	 * Vlan configuration exists before stream identification
	 */
	if (netc_sw_vft_find_entry(si->drv.sw, org_vid, &entry_id, &config.cfge) < 0) {
		os_log(LOG_ERR, "Unable to get forwarding configuration for the stream\n");
		goto err;
	}

	config.keye.vid = new_vid;

	/* secondary Egress Treatment group */
	config.cfge.etaPortBitmap = 0x1F;
	config.cfge.baseETEID = base_et_eid;

	if (SWT_BridgeAddVFTableEntry(si->hw.handle, &config, &vft_entry_id) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_BridgeAddVFTableEntry() failed\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

static void netc_si_get_params_from_efmid(struct netc_sw_drv *drv, uint32_t efmid, struct si_efm_params *params)
{
#ifndef CONFIG_GENAVB_TSN_DSA
	netc_tb_fm_config_t fm_conf = {0};
#endif

	if (efmid & IS_NETC_EFM_ENCODE_OPTION_2) {
		if (IS_NETC_EFM_ENCODE_OPTION_2_VARA(efmid) == kNETC_ReplVidOnly) {
			params->action |= ACTION_VLAN_REPLACE;
			params->vid = IS_NETC_EFM_ENCODE_OPTION_2_VID(efmid);
		} else if (IS_NETC_EFM_ENCODE_OPTION_2_VARA(efmid) == kNETC_AddCVlanPcpAndDei) {
			params->action |= ACTION_VLAN_ADD;
		}
	} else if (efmid & IS_NETC_EFM_ENCODE_OPTION_1) {
		if (IS_NETC_EFM_ENCODE_OPTION_1_SQTA(efmid) == kNETC_ReomveRTag)
			params->action = ACTION_RTAG_REMOVE;
		if (IS_NETC_EFM_ENCODE_OPTION_1_VUDA(efmid) == kNETC_DelVlan)
			params->action |= ACTION_VLAN_REMOVE;
	} else {
		params->action = ACTION_VLAN_REPLACE | ACTION_RTAG_REMOVE;
#ifdef CONFIG_GENAVB_TSN_DSA
		params->vid = DSA_TAG_8021Q_RSV | (efmid - DSA_NETC_EFM_DEL_SQ_TAG_CHANGE_VLAN_BASE);
#else
		if (SWT_FMQueryTableEntry(&drv->handle, efmid, &fm_conf) == kStatus_Success)
			params->vid = fm_conf.cfge.outerVlanID;
#endif
	}
}

#ifndef CONFIG_GENAVB_TSN_DSA
static int netc_si_alloc_fm_eid(struct netc_sw_drv *drv)
{
	int i;

	for (i = INTERNAL_EFM_BASE; i < drv->fm_table_entries; i++) {
		if (!rtos_atomic_test_bit(i, drv->fm_table)) {
			rtos_atomic_set_bit(i, drv->fm_table);
			return i;
		}
	}

	return -1;
}
#endif

void netc_si_free_fm_eid(void *sw_drv, uint32_t eid)
{
	struct netc_sw_drv *drv = (struct netc_sw_drv *)sw_drv;

	if (eid < drv->fm_table_entries)
		rtos_atomic_clear_bit(eid, drv->fm_table);
}

static void netc_si_update_efm_params(struct netc_sw_drv *drv, netc_tb_et_config_t *entry, void *data)
{
	struct si_efm_params *params = data;
	struct si_efm_params old_params = {0};
	uint32_t action, vlan_action, rtag_action;
	uint32_t vid;

	netc_si_get_params_from_efmid(drv, entry->cfge.efmEID, &old_params);

	vlan_action = ACTION_TAG2VLAN_OPERATION(params->action);

	switch (vlan_action) {
	case ACTION_VLAN_ADD:
	case ACTION_VLAN_REMOVE:
	case ACTION_VLAN_REPLACE:
		rtag_action = ACTION_TAG2RTAG_OPERATION(old_params.action);
		vid = params->vid;
		break;

	default:
		vlan_action = ACTION_TAG2VLAN_OPERATION(old_params.action);
		rtag_action = ACTION_TAG2RTAG_OPERATION(params->action);
		vid = old_params.vid;
		break;
	}

	action = ACTION_TAG_OPERATION(vlan_action, rtag_action);

	if (action != (ACTION_VLAN_REPLACE | ACTION_RTAG_REMOVE) && !(entry->cfge.efmEID & (IS_NETC_EFM_ENCODE_OPTION_2 | IS_NETC_EFM_ENCODE_OPTION_1)))
		netc_si_free_fm_eid(drv, entry->cfge.efmEID);

	switch (action) {
	case ACTION_VLAN_ADD:
		break;

	case ACTION_VLAN_REMOVE:
		/* Egress Frame Modification: Delete internal VLAN Tag */
		entry->cfge.efmLenChange = -4;
		entry->cfge.efmEID = NETC_FD_EID_ENCODE_OPTION_1(kNETC_NoSqtAction, kNETC_DelVlan);
		break;

	case ACTION_VLAN_REPLACE:
		/* Restore original VLAN Tag */
		entry->cfge.efmEID = NETC_FD_EID_ENCODE_OPTION_2(kNETC_ReplVidOnly, vid);
		break;

	case ACTION_VLAN_REMOVE | ACTION_RTAG_REMOVE:
		entry->cfge.efmEID = NETC_FD_EID_ENCODE_OPTION_1(kNETC_ReomveRTag, kNETC_DelVlan);
		entry->cfge.efmLenChange = -10;
		break;

	case ACTION_VLAN_REPLACE | ACTION_RTAG_REMOVE:
#ifdef CONFIG_GENAVB_TSN_DSA
		entry->cfge.efmEID = DSA_NETC_EFM_DEL_SQ_TAG_CHANGE_VLAN_BASE + (vid & 0xf);
#else
		/* Delete Sequence and restore original VLAN Tag */
		netc_tb_fm_config_t fm_conf = {0};

		fm_conf.entryID = netc_si_alloc_fm_eid(drv);
		if (fm_conf.entryID != -1) {
			fm_conf.cfge.vlanHdrAct = kNETC_ReplOuterVlan;
			fm_conf.cfge.outerVidAct = kNETC_UseSpecVid;
			fm_conf.cfge.sqtAct = kNETC_ReomveRTag;
			fm_conf.cfge.outerTpidAct = kNETC_UseFrameTpid;
			fm_conf.cfge.outerPcpAct = kNETC_UseFramePcp;
			fm_conf.cfge.outerDeiAct = kNETC_UseFrameDei;
			fm_conf.cfge.outerVlanID = vid;
			if (SWT_FMAddTableEntry(&drv->handle, &fm_conf) != kStatus_Success) {
				os_log(LOG_ERR, "SWT_FMAddTableEntry() - failed\n");
				netc_si_free_fm_eid(drv, fm_conf.entryID);
			} else {
				entry->cfge.efmEID = fm_conf.entryID;
			}
		}
#endif
		break;

	default:
		break;
	}
}

int netc_si_update_et_rtag(void *sw_drv, uint32_t stream_handle, unsigned int port_index, bool del_tag)
{
	struct netc_sw_drv *drv = (struct netc_sw_drv *)sw_drv;
	uint32_t et_eid = netc_sw_get_stream_et_eid(stream_handle, port_index);
	struct si_efm_params params = {0};

	if (del_tag)
		params.action = ACTION_RTAG_REMOVE;

	return netc_sw_add_or_update_et_table_entry(drv, et_eid, &netc_si_update_efm_params, &params);
}

static int netc_si_add_et_entries(struct netc_si *si, uint32_t base_et_eid, struct si_efm_params *params)
{
	struct si_efm_params *efm_params = params;
#ifdef CONFIG_GENAVB_TSN_DSA
	struct si_efm_params new_params = {0};
#endif
	unsigned int port;

	for (port = 0; port < si->total_ports; port++) {
#ifdef CONFIG_GENAVB_TSN_DSA
		if (port == netc_sw_dsa_cpu_port(si->drv.sw)) {
			new_params.vid = params->dsa_vid;
			new_params.action = ACTION_VLAN_REPLACE;
			efm_params = &new_params;
		} else {
			efm_params = params;
		}
#endif

		if (netc_sw_add_or_update_et_table_entry(si->drv.sw, base_et_eid + port, &netc_si_update_efm_params, efm_params) < 0)
			goto err;
	}

	return 0;
err:
	return -1;
}

static void netc_si_update_ifm(netc_tb_is_config_t *entry, void *data)
{
	struct si_ifm_params *params = data;

	switch (params->action) {
	case ACTION_VLAN_ADD:
		/* Ingress Frame Modification: ADD internal vlan tag */
		entry->cfge.ifmEID = NETC_FD_EID_ENCODE_OPTION_2(kNETC_AddCVlanPcpAndDei, params->vid);
		entry->cfge.ifmeLenChange = 4;

		/* this doesn't work, it's overwritten by FDB Table entry */
		entry->cfge.etEID = params->et_eid;
		break;

	case ACTION_VLAN_REMOVE:
		break;

	case ACTION_VLAN_REPLACE:
		/* Ingress Frame Modification: REPLACE frame's VLAN tag with internal tag */
		entry->cfge.ifmEID = NETC_FD_EID_ENCODE_OPTION_2(kNETC_ReplVidOnly, params->vid);
		break;

	default:
		break;
	}
}

static int netc_si_fdb_workaround(struct netc_si *si, struct genavb_stream_identity *entry)
{
	/* IS_EID = stream_handle */
	uint32_t is_eid = entry->handle;
	uint16_t vid = netc_si_get_vid(entry);
	bool tagged = false;
	struct si_ifm_params ifm_params;
	struct si_efm_params efm_params;
	uint8_t ifm_action, efm_action;
	uint16_t internal_vid;
	uint32_t base_et_eid;

#ifdef CONFIG_GENAVB_TSN_DSA
	uint32_t iport = logical_port_to_bridge_port(entry->port->id);
	efm_params.dsa_vid = DSA_TAG_8021Q_RSV | iport;
#endif

	internal_vid = INTERNAL_VID_BASE + is_eid;
	base_et_eid = netc_sw_get_stream_et_eid(is_eid, 0);

	/*
	 * ERRATA051616:
	 * When stream use Bridge Forwarding, the FDB Entry always overwrite Egress Treatment
	 * from IS Table, even if it's NULL.
	 * Workaround: use VF Table secondary Egress treatment group.
	 *
	 * Untagged frames
	 * ---------------
	 * Ingress:
	 * - Add internal VLAN Tag to the frame using ingress frame modification
	 * - Add VF Table entry for that internal VLAN
	 * - The original VLAN forwarding information is re-used for the internal VLAN
	 *		a) FID (so the internal VLAN will match the same FDB entry as before)
	 *		b) Egress port mask
	 * Egress:
	 * - Remove internal VLAN tag with egress frame modification
	 *
	 * Tagged frames
	 * -------------
	 * Ingress:
	 * - Replace frame's VLAN Tag with internal VLAN Tag using ingress frame modification
	 * - Add VF Table entry for that internal VLAN
	 * - The original VLAN forwarding information is re-used for the internal VLAN
	 *		a) FID (so the internal VLAN will match the same FDB entry as before)
	 *		b) Egress port mask
	 * Egress:
	 * - Restore original VLAN Tagged with egress frame modification
	 *
	 * Now the Egress Treatment from VF Table is applied on the stream
	 * while using Bridge forwarding
	 */
	switch (entry->type) {
	case GENAVB_SI_NULL:
		if (entry->parameters.null.tagged == GENAVB_SI_TAGGED)
			tagged = true;
		break;
	case GENAVB_SI_SRC_MAC_VLAN:
		if (entry->parameters.smac_vlan.tagged == GENAVB_SI_TAGGED)
			tagged = true;
		break;
	default:
		break;
	}

	/* FIXME, this condition doesn't cover 100% of cases, since GENAVB_SI_PRIORITY/ALL can also have a vlan tag */
	if (tagged) {
		efm_action = ACTION_VLAN_REPLACE;
		ifm_action = ACTION_VLAN_REPLACE;
	} else {
		efm_action = ACTION_VLAN_REMOVE;
		ifm_action = ACTION_VLAN_ADD;
	}

	/* Update efm */
	efm_params.vid = vid;
	efm_params.action = efm_action;
#ifdef CONFIG_GENAVB_TSN_DSA
	if (iport == netc_sw_dsa_cpu_port(si->drv.sw))
		efm_params.action = ACTION_VLAN_REMOVE;
#endif

	/* Egress Treatment entries (one for each port): Replace or Remove internal VLAN Tag */
	if (netc_si_add_et_entries(si, base_et_eid, &efm_params) < 0) {
		goto err;
	}

	/* VF Table entry for internal VLAN */
	if (netc_si_add_internal_vft_entry(si, vid, internal_vid, base_et_eid) < 0) {
		goto err;
	}

	/* Update IS entry ifm */
	ifm_params.vid = internal_vid;
	ifm_params.et_eid = base_et_eid;
	ifm_params.action = ifm_action;

#ifdef CONFIG_GENAVB_TSN_DSA
	if (iport == netc_sw_dsa_cpu_port(si->drv.sw))
		ifm_params.action = ACTION_VLAN_REPLACE;
#endif

	/* Ingress Treatment entries (one for each port): Add or Replace internal VLAN Tag */
	if (netc_si_update_is_table_entry(si->drv.sw, is_eid, &netc_si_update_ifm, &ifm_params) < 0) {
		goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_si_add_is_table_entry(struct netc_si *si, uint32_t is_eid)
{
	netc_tb_is_config_t is_table_entry = {0};
	unsigned int refs = 0;
	int rc = 0;

	/* If it's already configured, goto out */
	refs = netc_si_get_is_references(si, is_eid);
	if (refs)
		goto out;

	is_table_entry.entryID = is_eid;
	is_table_entry.cfge.fa = kNETC_ISBridgeForward; /* Frame Action */

	is_table_entry.cfge.isqEID = NULL_ENTRY_ID;
	is_table_entry.cfge.sgiEID = NULL_ENTRY_ID;
	is_table_entry.cfge.ifmEID = NULL_ENTRY_ID;
	is_table_entry.cfge.etEID = NULL_ENTRY_ID;
	is_table_entry.cfge.iscEID = NULL_ENTRY_ID;
	is_table_entry.cfge.rpEID = NULL_ENTRY_ID;

	netc_si_get_is_table_refs(si, is_eid, &is_table_entry);

	/* Ingress Stream Table */
	if (SWT_RxPSFPAddISTableEntry(si->hw.handle, &is_table_entry) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPAddISTableEntry() failed\n");
		rc = -1;
		goto err;
	}

err:
out:
	return rc;
}

static int netc_si_delete_is_table_entry(struct netc_si *si, uint32_t is_eid)
{
	unsigned int refs = 0;
	int rc = 0;

	/* count how many entries uses this handle */
	refs = netc_si_get_is_references(si, is_eid);

	/* if it's the last one, remove IS Table entry */
	if (refs == 1) {
		if (SWT_RxPSFPDelISTableEntry(si->hw.handle, is_eid) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_RxPSFPDelISTableEntry() failed\n");
			rc = -1;
		}
	}

	return rc;
}

static int netc_si_update_is_table_entry(struct netc_sw_drv *drv, uint32_t is_eid, void (*update_entry)(netc_tb_is_config_t *entry, void *data), void *data)
{
	netc_tb_is_config_t is_table_entry;
	status_t status;
	int rc = -1;

	if (!update_entry)
		goto err;

	status = SWT_RxPSFPQueryISTableEntry(&drv->handle, is_eid, &is_table_entry);
	if (status == kStatus_NETC_NotFound) {
		rc = 0;
	} else if (status == kStatus_Success) {
		update_entry(&is_table_entry, data);

		if (SWT_RxPSFPUpdateISTableEntry(&drv->handle, &is_table_entry) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_RxPSFPUpdateISTableEntry() failed\n");
			rc = -1;
			goto err;
		}
		rc = 1;
	}

err:
	return rc;
}

static int netc_si_update_entry(struct net_si_ops *si_ops, uint32_t index, struct genavb_stream_identity *entry)
{
	struct netc_si *si = netc_si_drv(si_ops);
	struct netc_si_entry *si_entry;
	uint32_t port_mask = 0;
	int rc = GENAVB_SUCCESS;
	int i;

	if (!entry) {
		rc = -GENAVB_ERR_SI_INVALID_STREAM_IDENTITY;
		goto err;
	}

	if (index >= SI_MAX_ENTRIES) {
		rc = -GENAVB_ERR_SI_MAX_ENTRIES;
		goto err;
	}

	if (entry->handle >= SI_MAX_HANDLE) {
		rc = -GENAVB_ERR_SI_MAX_HANDLE;
		goto err;
	}

	si_entry = &si->entry[index];

	if (netc_si_is_used(si_entry)) {
		if (netc_si_get_type(si_entry) != entry->type) {
			rc = -GENAVB_ERR_SI_TYPE_UPDATE_NOT_SUPPORTED;
			goto err;
		}

		if (si_entry->vlan_type != si_vlan_type(entry)) {
			rc = -GENAVB_ERR_SI_VLAN_TYPE_UPDATE_NOT_SUPPORTED;
			goto err;
		}

		if (si_entry->handle != entry->handle) {
			rc = -GENAVB_ERR_SI_HANDLE_UPDATE_NOT_SUPPORTED;
			goto err;
		}
	}

	rc = netc_si_add_is_table_entry(si, entry->handle);
	if (rc < 0) {
		rc = -GENAVB_ERR_SI_ADD;
		goto err;
	}

	netc_si_set_used(si_entry, true);
	si_entry->handle = entry->handle;
	netc_si_set_type(si_entry, entry->type);
	si_entry->vlan_type = si_vlan_type(entry);

	for (i = 0; i < entry->port_n; i++) {
		struct logical_port *port = __logical_port_get(entry->port[i].id);

		port_mask |= (1 << port->phys->base);
	}

	if (si_supported_by_isi(entry)) {
		si_entry->use_isi = 1;
	} else if (si_supported_by_ipf(entry)) {
		si_entry->use_isi = 0;
	} else {
		rc = -GENAVB_ERR_SI_TYPE_UPDATE_NOT_SUPPORTED;
		goto err_supported;
	}

	rc = netc_si_update_isi_ipf_table(si, si_entry, entry, port_mask);
	if (rc < 0) {
		rc = -GENAVB_ERR_SI_HW_CONFIG;
		goto err_itable;
	}

	if (netc_si_fdb_workaround(si, entry) < 0) {
		rc = -GENAVB_ERR_SI_UPDATE;
		goto err_workaround;
	}

	rc = netc_sw_frer_apply(si->drv.sw, entry->handle, port_mask);
	if (rc < 0)
		goto err_frer;

	return rc;

err_frer:
err_workaround:
err_itable:
	netc_si_update_isi_ipf_table(si, si_entry, NULL, 0);

err_supported:
	netc_si_delete_is_table_entry(si, entry->handle);
	netc_si_set_used(si_entry, false);
err:
	return rc;
}

static int netc_si_delete_entry(struct net_si_ops *si_ops, uint32_t index)
{
	struct netc_si *si = netc_si_drv(si_ops);
	struct netc_si_entry *si_entry;
	int rc = GENAVB_SUCCESS;

	if (index >= SI_MAX_ENTRIES) {
		rc = -GENAVB_ERR_SI_MAX_ENTRIES;
		goto err;
	}

	si_entry = &si->entry[index];

	if (!netc_si_is_used(si_entry)) {
		rc = -GENAVB_ERR_SI_ENTRY_NOT_FOUND;
		goto err;
	}

	if (netc_si_update_isi_ipf_table(si, si_entry, NULL, 0) < 0) {
		rc = -GENAVB_ERR_SI_HW_CONFIG;
		goto err;
	}

	if (netc_si_delete_is_table_entry(si, si_entry->handle) < 0) {
		rc = -GENAVB_ERR_SI_DELETE;
		goto err;
	}

	for (int port = 0; port < si->total_ports; port++)
		if (netc_sw_delete_et_table_entry(si->drv.sw, netc_sw_get_stream_et_eid(si_entry->handle, port)) < 0)
			goto err;

	memset(si_entry, 0, sizeof(struct netc_si_entry));

err:
	return rc;
}

static int netc_si_read_entry(struct net_si_ops *si_ops, uint32_t index, struct genavb_stream_identity *entry)
{
	struct netc_si *si = netc_si_drv(si_ops);
	struct netc_si_entry *si_entry;
	int rc = GENAVB_SUCCESS;
	int i;

	if (!entry) {
		rc = -GENAVB_ERR_SI_INVALID_STREAM_IDENTITY;
		goto err;
	}

	if (index >= SI_MAX_ENTRIES) {
		rc = -GENAVB_ERR_SI_MAX_ENTRIES;
		goto err;
	}

	si_entry = &si->entry[index];

	if (!netc_si_is_used(si_entry)) {
		rc = -GENAVB_ERR_SI_ENTRY_NOT_FOUND;
		goto err;
	}

	entry->type = netc_si_get_type(si_entry);
	entry->handle = si_entry->handle;

	entry->port_n = 0;
	for (i = 0; i < si->total_ports; i++) {
		if (si_entry->port_mask & (1 << i)) {
			entry->port[entry->port_n].id = netc_sw_port_to_net_port[i];
			entry->port_n++;
		}
	}

	if (si_entry->use_isi) {
		if (netc_si_read_isi_table(si, si_entry, entry) < 0) {
			rc = -GENAVB_ERR_SI_READ;
			goto err;
		}
	} else {
		if (netc_si_read_ipf_table(si, si_entry, entry) < 0) {
			rc = -GENAVB_ERR_SI_READ;
			goto err;
		}
	}

err:
	return rc;
}

__init static int netc_si_port_isi_init(struct netc_si *si)
{
	netc_port_psfp_isi_config psfp_isi_config = {0};
	netc_swt_psfp_config_t psfp_config = {0};
	int i;

	psfp_config.kcRule[0] = key0;
	psfp_config.kcRule[1] = key1;

	if (SWT_RxPSFPInit(si->hw.handle, &psfp_config) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPInit() failed\n");
		goto err;
	}

	/* configure all switch ports to use first pair of keys for first and second lookup */
	for (i = 0; i < si->total_ports; i++) {
		if (si->port[i]) {
			struct net_port *port = si->port[i];

			psfp_isi_config.defaultISEID = 0xFFFF;
			psfp_isi_config.enKC0 = true; /* lookup is performed for the first */
			psfp_isi_config.enKC1 = true; /* and second stream identification */
			psfp_isi_config.kcPair = false; /* Key pair: Use key 0 and key 1 (first and second lookup) */

			if (SWT_RxPSFPConfigPortISI(si->hw.handle, port->base, &psfp_isi_config) != kStatus_Success) {
				os_log(LOG_ERR, "SWT_RxPSFPConfigPortISI() failed\n");
				goto err;
			}
		}
	}

	return 0;
err:
	return -1;
}

__init int netc_si_init(struct net_si_ops *si_ops)
{
	struct netc_si *si = netc_si_drv(si_ops);

	si_ops->si_update = &netc_si_update_entry;
	si_ops->si_delete = &netc_si_delete_entry;
	si_ops->si_read = &netc_si_read_entry;

	return netc_si_port_isi_init(si);
}

#else
#endif /* CFG_NUM_NETC_SW */
