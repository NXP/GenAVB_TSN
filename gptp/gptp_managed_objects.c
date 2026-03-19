/*
 * Copyright 2018-2021, 2023-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GPTP managed objects definition
 @details
*/

#include "config.h"

#include "gptp_managed_objects.h"
#include "gptp.h"
#include "ptp_time_ops.h"

#include <stdio.h>

extern const unsigned int leaf_object_size[];

static void selected_role_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_instance *instance = data;
	struct gptp_port *port = (struct gptp_port *)(base + l->val);

	switch (operation) {
	case NODE_GET:
			if (port)
				buf[0] = instance->params.selected_role[port->port_id + 1];
		break;

	default:
		break;
	}
}

static void version_number_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_instance *instance = data;

	switch (operation) {
	case NODE_GET:
		buf[0] = instance->versionNumber;
		break;

	default:
		break;
	}
}

static void priority_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	switch (operation) {
	case NODE_GET:
		buf[0] = *((u8 *)(base + l->val));
		break;

	case NODE_SET:
		*((u8 *)(base + l->val)) = buf[0];
		gptp_instance_priority_vector_update(data);
		break;

	default:
		break;
	}
}

static void offset_scaled_log_variance_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	uint16_t val;

	switch (operation) {
	case NODE_GET:
		val = *((u16 *)(base + l->val));
		val = ntohs(val);
		os_memcpy(buf, &val, sizeof(u16));
		break;

	default:
		break;
	}
}

static struct gptp_port_common *get_port_common(struct gptp_port *port)
{
	struct gptp_port_common *c;

	if (port->params.delay_mechanism != COMMON_P2P)
		c = port->c;
	else
		c = &port->instance->gptp->cmlds.link_ports[port->link_id].c;

	return c;
}

static void neighbor_rate_ratio_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)data;
	uintptr_t object_base = (uintptr_t)get_port_common(port);
	double ratio;
	s32 ratio_s32;

	switch (operation) {
	case NODE_GET:
		ratio = *((double *)(object_base + (uintptr_t)l->val));
		ratio = (ratio - 1.0) * POW_2_41;
		ratio_s32 = (s32)ratio;
		os_memcpy(buf, &ratio_s32, sizeof(s32));
		break;

	default:
		break;
	}
}

static void ptp_port_enabled_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)(base + l->val);

	switch (operation) {
	case NODE_GET:
		buf[0] = port->params.ptp_port_enabled ? 1u : 0u;
		break;

	case NODE_SET:
		port->params.ptp_port_enabled = buf[0];
		gptp_port_update_fsm(port);
		break;

	default:
		break;
	}
}

static void ptp_port_common_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)data;
	uintptr_t object_base = (uintptr_t)get_port_common(port);
	unsigned int object_size;

	switch (operation) {
	case NODE_GET:
		object_size = leaf_object_size[l->type];

		if (l->type == LEAF_BOOL)
			buf[0] = *((bool *)(object_base + l->val));
		else
			os_memcpy(buf, (uint8_t *)(object_base + l->val), object_size);
		break;

	default:
		break;
	}
}

static void instance_index_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	switch (operation) {
	case NODE_GET:
		buf[0] = *((u8 *)(base + l->val));
		buf[1] = 0;
		break;

	default:
		break;
	}
}

static void mean_link_delay_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)data;
	uintptr_t object_base = (uintptr_t)get_port_common(port);
	u64 pdelay;

	u_scaled_ns_to_u64(&pdelay, (struct ptp_u_scaled_ns *)(object_base + (uintptr_t)l->val));

	switch (operation) {
	case NODE_GET:
		os_memcpy(buf, &pdelay, leaf_object_size[l->type]);
		break;

	default:
		break;
	}
}

static void current_time_sec_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	u64 current_time_ns;
	u64 sec;

	u_scaled_ns_to_u64(&current_time_ns, (struct ptp_u_scaled_ns *)(base + (uintptr_t)l->val));
	sec = current_time_ns / NSECS_PER_SEC;

	switch (operation) {
	case NODE_GET:
		os_memcpy(buf, &sec, leaf_object_size[l->type]);
		break;

	default:
		break;
	}
}

static void current_time_nsec_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	u64 current_time_ns;
	u32 nsec;

	u_scaled_ns_to_u64(&current_time_ns, (struct ptp_u_scaled_ns *)(base + (uintptr_t)l->val));
	nsec = htonl(current_time_ns % NSECS_PER_SEC);

	switch (operation) {
	case NODE_GET:
		os_memcpy(buf, &nsec, leaf_object_size[l->type]);
		break;

	default:
		break;
	}
}

unsigned int gptp_managed_objects_get(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end)
{
	uint8_t *end;

	end = module_iterate(&module->node, NODE_GET, in, in_end, out, out_end);

	return end - out;
}

unsigned int gptp_managed_objects_set(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end)
{
	uint8_t *end = module_iterate(&module->node, NODE_SET, in, in_end, out, out_end);

	return end - out;
}

__init void gptp_managed_objects_init(struct gptp_managed_objects *module, struct gptp_ctx *gptp)
{
	struct gptp_instance *instance;
	struct gptp_port *port;
	uint16_t key_id = 0;
	int i, j;

	MODULE_INIT(module, "ietf-gptp");

	LIST_INIT(module, instanceList, 1, &key_id);

	for (i = 0; i < CFG_MAX_GPTP_DOMAINS; i++) {
		instance = gptp->instances[i];

		LIST_ENTRY_INIT(&module->instanceList, instance[i]);

		#define _instance &module->instanceList.instance[i]

		LEAF_INIT(_instance, instanceIndex, LEAF_UINT16, LEAF_R, &instance->index, &instance_index_handler, instance);

		CONTAINER_INIT(_instance, default_ds);

		LEAF_INIT(_instance.default_ds, clockIdentity, LEAF_CLOCK_IDENTITY, LEAF_R, &instance->params.this_clock, NULL, NULL);
		LEAF_INIT(_instance.default_ds, numberPorts, LEAF_UINT16, LEAF_R, &instance->numberPorts, NULL, NULL);

		CONTAINER_INIT(_instance.default_ds, clock_quality);
		LEAF_INIT(_instance.default_ds.clock_quality, clockClass, LEAF_UINT8, LEAF_R, &instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_class, NULL, NULL);
		LEAF_INIT(_instance.default_ds.clock_quality, clockAccuracy, LEAF_UINT8, LEAF_R, &instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_accuracy, NULL, NULL);
		LEAF_INIT(_instance.default_ds.clock_quality, offsetScaledLogVariance, LEAF_UINT16, LEAF_R, &instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.offset_scaled_log_variance, &offset_scaled_log_variance_handler, NULL);

		LEAF_INIT(_instance.default_ds, priority1, LEAF_UINT8, LEAF_RW, &instance->params.system_priority.u.s.root_system_identity.u.s.priority_1, &priority_handler, instance);
		LEAF_INIT(_instance.default_ds, priority2, LEAF_UINT8, LEAF_RW, &instance->params.system_priority.u.s.root_system_identity.u.s.priority_2, &priority_handler, instance);
		LEAF_INIT(_instance.default_ds, domainNumber, LEAF_UINT8, LEAF_RW, &instance->domain.domain_number, NULL, instance);

		CONTAINER_INIT(_instance.default_ds, current_time);
		LEAF_INIT(_instance.default_ds.current_time, secondsField, LEAF_UINT64, LEAF_RW, &instance->params.current_time, &current_time_sec_handler, NULL);
		LEAF_INIT(_instance.default_ds.current_time, nanosecondsField, LEAF_UINT32, LEAF_RW, &instance->params.current_time, &current_time_nsec_handler, NULL);

		LEAF_INIT(_instance.default_ds, gmCapable, LEAF_BOOL, LEAF_R, &instance->gmCapable, NULL, NULL);
		LEAF_INIT(_instance.default_ds, currentUtcOffset, LEAF_UINT16, LEAF_RW, &instance->params.sys_current_utc_offset, NULL, NULL);
		LEAF_INIT(_instance.default_ds, currentUtcOffetValid, LEAF_BOOL, LEAF_RW, &instance->params.sys_current_utc_offset_valid, NULL, NULL);
		LEAF_INIT(_instance.default_ds, leap59, LEAF_BOOL, LEAF_RW, &instance->params.sys_leap59, NULL, NULL);
		LEAF_INIT(_instance.default_ds, leap61, LEAF_BOOL, LEAF_RW, &instance->params.sys_leap61, NULL, NULL);
		LEAF_INIT(_instance.default_ds, timeTraceable, LEAF_BOOL, LEAF_R, &instance->params.sys_time_traceable, NULL, NULL);
		LEAF_INIT(_instance.default_ds, frequencyTraceable, LEAF_BOOL, LEAF_R, &instance->params.sys_frequency_traceable, NULL, NULL);
		LEAF_INIT(_instance.default_ds, ptpTimescale, LEAF_BOOL, LEAF_R, &instance->params.sys_ptp_timescale, NULL, NULL);
		LEAF_INIT(_instance.default_ds, timeSource, LEAF_UINT8, LEAF_R, &instance->params.sys_time_source, NULL, NULL);

		CONTAINER_INIT(_instance, current_ds);

		LEAF_INIT(_instance.current_ds, stepsRemoved, LEAF_UINT16, LEAF_R, &instance->params.master_steps_removed, NULL, NULL);
		LEAF_INIT(_instance.current_ds, offsetFromMaster, LEAF_SCALED_NS, LEAF_R, NULL, NULL, NULL); /* FIXME Implementation defined, should be provided by the ClockMasterSyncOffset SM, 14.3.2 */
		LEAF_INIT(_instance.current_ds, meanDelay, LEAF_SCALED_NS, LEAF_R, (void *)offsetof(struct gptp_port_common, params.mean_link_delay), &mean_link_delay_handler, &instance->ports[0]);

		LEAF_INIT(_instance.current_ds, lastGmPhaseChange, LEAF_SCALED_NS, LEAF_R, &instance->params.last_gm_phase_change, NULL, NULL);
		LEAF_INIT(_instance.current_ds, lastGmFreqChange, LEAF_DOUBLE, LEAF_R, &instance->params.last_gm_freq_change, NULL, NULL);
		LEAF_INIT(_instance.current_ds, gmTimebaseIndicator, LEAF_UINT16, LEAF_R, &instance->params.gm_time_base_indicator, NULL, NULL);
		LEAF_INIT(_instance.current_ds, gmChangeCount, LEAF_UINT32, LEAF_R, &instance->gmChangeCount, NULL, NULL);
		LEAF_INIT(_instance.current_ds, timeOfLastGmChangeEvent, LEAF_UINT32, LEAF_R, &instance->timeOfLastGmChangeEvent, NULL, NULL);
		LEAF_INIT(_instance.current_ds, timeOfLastGmPhaseChangeEvent, LEAF_UINT32, LEAF_R, &instance->timeOfLastGmPhaseChangeEvent, NULL, NULL);
		LEAF_INIT(_instance.current_ds, timeOfLastGmFreqChangeEvent, LEAF_UINT32, LEAF_R, &instance->timeOfLastGmFreqChangeEvent, NULL, NULL);

		CONTAINER_INIT(_instance, parent_ds);

		CONTAINER_INIT(_instance.parent_ds, parentPortIdentity);
		LEAF_INIT(_instance.parent_ds.parentPortIdentity, clockIdentity, LEAF_CLOCK_IDENTITY, LEAF_R, &instance->params.this_clock, NULL, NULL);
		LEAF_INIT(_instance.parent_ds.parentPortIdentity, portNumber, LEAF_UINT16, LEAF_R, &instance->gm_info.vector.u.s.source_port_identity.port_number, NULL, NULL);

		LEAF_INIT(_instance.parent_ds, parentStats, LEAF_BOOL, LEAF_R, &instance->parentStats, NULL, NULL);
		LEAF_INIT(_instance.parent_ds, grandMasterIdentity, LEAF_CLOCK_IDENTITY, LEAF_R, &instance->gm_info.vector.u.s.root_system_identity.u.s.clock_identity, NULL, NULL);

		CONTAINER_INIT(_instance.parent_ds, grandmaster_clock_quality);
		LEAF_INIT(_instance.parent_ds.grandmaster_clock_quality, clockClass, LEAF_UINT8, LEAF_R, &instance->gm_info.vector.u.s.root_system_identity.u.s.clock_quality.clock_class, NULL, NULL);
		LEAF_INIT(_instance.parent_ds.grandmaster_clock_quality, clockAccuracy, LEAF_UINT8, LEAF_R, &instance->gm_info.vector.u.s.root_system_identity.u.s.clock_quality.clock_accuracy, NULL, NULL);
		LEAF_INIT(_instance.parent_ds.grandmaster_clock_quality, offsetScaledLogVariance, LEAF_UINT16, LEAF_R, &instance->gm_info.vector.u.s.root_system_identity.u.s.clock_quality.offset_scaled_log_variance, &offset_scaled_log_variance_handler, NULL)

		LEAF_INIT(_instance.parent_ds, grandMasterPriority1, LEAF_UINT8, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.priority_1, NULL, NULL);
		LEAF_INIT(_instance.parent_ds, grandMasterPriority2, LEAF_UINT8, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.priority_2, NULL, NULL);
		LEAF_INIT(_instance.parent_ds, cumulativeRateRatio, LEAF_INT32, LEAF_R, &instance->cumulativeRateRatio, NULL, NULL);

		CONTAINER_INIT(_instance, time_properties_ds);

		LEAF_INIT(_instance.time_properties_ds, currentUtcOffset, LEAF_INT16, LEAF_R, &instance->params.current_utc_offset, NULL, NULL);
		LEAF_INIT(_instance.time_properties_ds, currentUtcOffsetValid, LEAF_BOOL, LEAF_R, &instance->params.current_utc_offset_valid, NULL, NULL);
		LEAF_INIT(_instance.time_properties_ds, leap59, LEAF_BOOL, LEAF_R, &instance->params.leap59, NULL, NULL);
		LEAF_INIT(_instance.time_properties_ds, leap61, LEAF_BOOL, LEAF_R, &instance->params.leap61, NULL, NULL);
		LEAF_INIT(_instance.time_properties_ds, timeTraceable, LEAF_BOOL, LEAF_R, &instance->params.time_traceable, NULL, NULL);
		LEAF_INIT(_instance.time_properties_ds, frequencyTraceable, LEAF_BOOL, LEAF_R, &instance->params.frequency_traceable, NULL, NULL);
		LEAF_INIT(_instance.time_properties_ds, timeSource, LEAF_UINT8, LEAF_R, &instance->params.time_source, NULL, NULL);

		CONTAINER_INIT(_instance, ports);

		key_id = 0;
		LIST_INIT(_instance.ports, portList, 1, &key_id);

		for (j = 0; j < instance->numberPorts; j++) {
			port = &instance->ports[j];
			LIST_ENTRY_INIT(_instance.ports.portList, port[j]);

			#define _port _instance.ports.portList.port[j]

			LEAF_INIT(_port, portIndex, LEAF_UINT16, LEAF_R, &port->port_id, NULL, NULL);

			CONTAINER_INIT(_port, port_ds);
			CONTAINER_INIT(_port.port_ds, portIdentity);
			LEAF_INIT(_port.port_ds.portIdentity, clockIdentity, LEAF_CLOCK_IDENTITY, LEAF_R, &instance->params.this_clock, NULL, NULL);
			LEAF_INIT(_port.port_ds.portIdentity, portNumber, LEAF_UINT16, LEAF_R, &port->port_id, NULL, NULL);
			LEAF_INIT(_port.port_ds, portRole, LEAF_UINT8, LEAF_R, port, &selected_role_handler, instance);
			LEAF_INIT(_port.port_ds, meanLinkDelay, LEAF_USCALED_NS, LEAF_R, (void *)offsetof(struct gptp_port_common, params.mean_link_delay), &mean_link_delay_handler, port);
			LEAF_INIT(_port.port_ds, logAnnounceInterval, LEAF_INT8, LEAF_RW, &port->params.initial_log_announce_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, announceReceiptTimeout, LEAF_UINT8, LEAF_RW, &port->announce_receipt_timeout, NULL, NULL); /* FIXME implement interval update, or just wait for next announce? */
			LEAF_INIT(_port.port_ds, logSyncInterval, LEAF_INT8, LEAF_RW, &port->params.current_log_sync_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, versionNumber, LEAF_UINT8, LEAF_R, NULL, &version_number_handler, NULL);
			LEAF_INIT(_port.port_ds, delayAsymmetry, LEAF_SCALED_NS, LEAF_RW, (void *)offsetof(struct gptp_port_common, params.delay_asymmetry), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_ds, pttPortEnabled, LEAF_BOOL, LEAF_RW, port, &ptp_port_enabled_handler, port);
			LEAF_INIT(_port.port_ds, isMeasuringDleay, LEAF_BOOL, LEAF_R, (void *)offsetof(struct gptp_port_common, md.globals.isMeasuringDelay), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_ds, asCapable, LEAF_BOOL, LEAF_R, &port->params.as_capable, NULL, NULL);
			LEAF_INIT(_port.port_ds, neighborPropDelayThresh, LEAF_USCALED_NS, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.meanLinkDelayThresh), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_ds, neighborRateRatio, LEAF_INT32, LEAF_R, (void *)offsetof(struct gptp_port_common, params.neighbor_rate_ratio), &neighbor_rate_ratio_handler, port);
			LEAF_INIT(_port.port_ds, initialLogAnnounceInterval, LEAF_INT8, LEAF_RW, &port->params.initial_log_announce_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, currentLogAnnounceInterval, LEAF_INT8, LEAF_R, &port->params.current_log_announce_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, initialLogSyncInterval, LEAF_INT8, LEAF_R, &port->params.initial_log_sync_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, currentLogSyncInterval, LEAF_INT8, LEAF_R, &port->params.current_log_sync_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, syncReceiptTimeout, LEAF_UINT8, LEAF_RW, &port->sync_receipt_timeout, NULL, NULL); /* FIXME implement interval update, or just wait for next sync? */
			LEAF_INIT(_port.port_ds, syncReceiptTimeoutTimeInterval, LEAF_USCALED_NS, LEAF_R, &port->params.sync_receipt_timeout_time_interval, NULL, NULL);
			LEAF_INIT(_port.port_ds, initialLogPdelayReqInterval, LEAF_INT8, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.initialLogPdelayReqInterval), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_ds, currentLogPdelayReqInterval, LEAF_INT8, LEAF_R, (void *)offsetof(struct gptp_port_common, md.globals.currentLogPdelayReqInterval), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_ds, allowedLostResponses, LEAF_UINT16, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.allowedLostResponses), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_ds, allowedFaults, LEAF_UINT16, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.allowedFaults), &ptp_port_common_handler, port);

			CONTAINER_INIT(_port, port_statistics_ds);
			LEAF_INIT(_port.port_statistics_ds, rxSyncCount, LEAF_UINT32, LEAF_R, &port->stats.num_rx_sync, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, rxFollowUpCount, LEAF_UINT32, LEAF_R, &port->stats.num_rx_fup, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, rxPdelayRequestCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayreq), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, rxPdelayResponseCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayresp), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, rxPdelayResponseFollowUpCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayrespfup), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, rxAnnounceCount, LEAF_UINT32, LEAF_R, &port->stats.num_rx_announce, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, rxPTPPacketDiscardCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_ptp_packet_discard), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, syncReceiptTimeoutCount, LEAF_UINT32, LEAF_R, &port->stats.num_rx_sync_timeout, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, announceReceiptTimeoutCount, LEAF_UINT32, LEAF_R, &port->stats.num_rx_announce_timeout, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, pdelayAllowedLostResponsesExceededCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayresp_lost_exceeded), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, txSyncCount, LEAF_UINT32, LEAF_R, &port->stats.num_tx_sync, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, txFollowUpCount, LEAF_UINT32, LEAF_R, &port->stats.num_tx_fup, NULL, NULL);
			LEAF_INIT(_port.port_statistics_ds, txPdelayRequestCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_tx_pdelayreq), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, txPdelayResponseCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_tx_pdelayresp), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, txPdelayResponseFollowUpCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_tx_pdelayrespfup), &ptp_port_common_handler, port);
			LEAF_INIT(_port.port_statistics_ds, txAnnounceCount, LEAF_UINT32, LEAF_R, &port->stats.num_tx_announce, NULL, NULL);
		}
	}
}
