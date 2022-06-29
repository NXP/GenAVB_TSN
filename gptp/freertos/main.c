/*
* Copyright 2017, 2020-2021 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

/**
 @file
 @brief NXP GPTP FreeRTOS specific code
 @details Setups FreeRTOS task for NXP GPTP stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "queue.h"

#include "common/ipc.h"
#include "common/log.h"
#include "common/ptp.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/sys_types.h"
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "gptp/gptp_entry.h"

#define GPTP_TASK_NAME		"gPTP Stack"
#define GPTP_TASK_SUCCESS	(1 << 0)
#define GPTP_TASK_ERROR		(1 << 1)

struct gptp_ctx {
	EventGroupHandle_t event_group_handle;
	StaticEventGroup_t event_group;
	struct fgptp_config *gptp_cfg;
	QueueHandle_t event_queue_h;
	TaskHandle_t task_h;
	void *gptp;
};

static void sync_indication_handler(struct gptp_sync_info *info);

const struct fgptp_config gptp_default_config = {
	.log_level = gptp_CFG_LOG,

	.is_bridge = 0,

	.profile = CFG_GPTP_PROFILE_STANDARD,

	.domain_max = CFG_MAX_GPTP_DOMAINS,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,

#ifdef CONFIG_MANAGEMENT
	.management_enabled = 1,
#else
	.management_enabled = 0,
#endif

	.gm_id = htonll(CFG_GPTP_DEFAULT_GM_ID),

	.neighborPropDelayThreshold = CFG_GPTP_NEIGH_THRESH_DEFAULT,

	.rsync = CFG_GPTP_RSYNC_ENABLE_DEFAULT,

	.rsync_interval = CFG_GPTP_RSYNC_INTERVAL_DEFAULT,

	.statsInterval = CFG_GPTP_STATS_INTERVAL_DEFAULT,

	.neighborPropDelay_mode = CFG_GPTP_PDELAY_MODE_STANDARD,

	.initial_neighborPropDelay = {
		[0 ... CFG_MAX_NUM_PORT - 1] = CFG_GPTP_DEFAULT_PDELAY_VALUE,
	},

	.neighborPropDelay_sensitivity = CFG_GPTP_DEFAULT_PDELAY_SENSITIVITY,

	.port_cfg = {
		[0 ... CFG_MAX_NUM_PORT - 1] = {
			.portRole = CFG_GPTP_DEFAULT_PORT_ROLE,
			.ptpPortEnabled = CFG_GPTP_DEFAULT_PTP_ENABLED,
			.rxDelayCompensation = CFG_GPTP_DEFAULT_RX_DELAY_COMP,
			.txDelayCompensation = CFG_GPTP_DEFAULT_TX_DELAY_COMP,
			.initialLogPdelayReqInterval = CFG_GPTP_DFLT_LOG_PDELAY_REQ_INTERVAL,
			.initialLogSyncInterval = CFG_GPTP_DFLT_LOG_SYNC_INTERVAL,
			.initialLogAnnounceInterval = CFG_GPTP_DFLT_LOG_ANNOUNCE_INTERVAL,
			.operLogPdelayReqInterval = CFG_GPTP_DFLT_LOG_PDELAY_REQ_INTERVAL,
			.operLogSyncInterval = CFG_GPTP_DFLT_LOG_SYNC_INTERVAL,
			.delayMechanism[0] = P2P,
#if CFG_MAX_GPTP_DOMAINS > 1
			.delayMechanism[1 ... CFG_MAX_GPTP_DOMAINS - 1] = COMMON_P2P,
#endif
		},
	},

	.sync_indication = sync_indication_handler,

	.gm_indication = NULL,

	.pdelay_indication = NULL,

	.domain_cfg = {
		[0] = {
			.domain_number = 0,

			.gmCapable = 0,
			.priority1 = CFG_GPTP_DEFAULT_PRIORITY1,
			.priority2 = CFG_GPTP_DEFAULT_PRIORITY2,
			.clockClass = CFG_GPTP_DEFAULT_CLOCK_CLASS,
			.clockAccuracy = CFG_GPTP_DEFAULT_CLOCK_ACCURACY,
			.offsetScaledLogVariance = CFG_GPTP_DEFAULT_CLOCK_VARIANCE,
		},
#if CFG_MAX_GPTP_DOMAINS > 1
		[1 ... CFG_MAX_GPTP_DOMAINS - 1] = {
			.domain_number = -1,

			.gmCapable = 0,
			.priority1 = CFG_GPTP_DEFAULT_PRIORITY1,
			.priority2 = CFG_GPTP_DEFAULT_PRIORITY2,
			.clockClass = CFG_GPTP_DEFAULT_CLOCK_CLASS,
			.clockAccuracy = CFG_GPTP_DEFAULT_CLOCK_ACCURACY,
			.offsetScaledLogVariance = CFG_GPTP_DEFAULT_CLOCK_VARIANCE,
		},
#endif
	},
};

/*******************************************************************************
* @function_name sync_indication_handler
* @brief called back by the gptp stack upon synchronization state change
*
*/
static void sync_indication_handler(struct gptp_sync_info *info)
{
	if (info->state == SYNC_STATE_SYNCHRONIZED)
		os_log(LOG_INFO, "port(%d) domain(%u) %s -- synchronization time (ms): %llu\n", info->port_id, info->domain, PTP_SYNC_STATE(info->state), info->sync_time_ms);
	else
		os_log(LOG_INFO, "port(%d) domain(%u) %s\n", info->port_id, info->domain, PTP_SYNC_STATE(info->state));
}

__init static uint64_t check_bounds_uint64_t(uint64_t val, uint64_t min, uint64_t max)
{
	if (val < min)
		return min;
	else if (val > max)
		return max;
	else
		return val;
}

__init static unsigned int check_bounds_unsigned_int(unsigned int val, unsigned int min, unsigned int max)
{
	if (val < min)
		return min;
	else if (val > max)
		return max;
	else
		return val;
}

__init static int check_bounds_int(int val, int min, int max)
{
	if (val < min)
		return min;
	else if (val > max)
		return max;
	else
		return val;
}

__init static void process_section_general(struct fgptp_config *cfg, int instance_index)
{
	if (cfg->profile != CFG_GPTP_PROFILE_AUTOMOTIVE)
		cfg->profile = CFG_GPTP_PROFILE_STANDARD;

	cfg->domain_cfg[instance_index].domain_number = check_bounds_int(cfg->domain_cfg[instance_index].domain_number, -1, PTP_DOMAIN_NUMBER_MAX);

	cfg->neighborPropDelayThreshold = check_bounds_uint64_t(cfg->neighborPropDelayThreshold, CFG_GPTP_NEIGH_THRESH_MIN_DEFAULT, CFG_GPTP_NEIGH_THRESH_MAX_DEFAULT);

	cfg->rsync = check_bounds_unsigned_int(cfg->rsync, CFG_GPTP_RSYNC_ENABLE_MIN_DEFAULT, CFG_GPTP_RSYNC_ENABLE_MAX_DEFAULT);

	cfg->rsync_interval = check_bounds_unsigned_int(cfg->rsync_interval, CFG_GPTP_RSYNC_INTERVAL_MIN_DEFAULT, CFG_GPTP_RSYNC_INTERVAL_MAX_DEFAULT);

	cfg->statsInterval = check_bounds_unsigned_int(cfg->statsInterval, CFG_GPTP_STATS_INTERVAL_MIN_DEFAULT, CFG_GPTP_STATS_INTERVAL_MAX_DEFAULT);
}


__init static void process_section_gm_params(struct fgptp_config *cfg, int instance_index)
{
	cfg->domain_cfg[instance_index].gmCapable = check_bounds_unsigned_int(cfg->domain_cfg[instance_index].gmCapable, 0, 1);

	cfg->domain_cfg[instance_index].priority1 = check_bounds_unsigned_int(cfg->domain_cfg[instance_index].priority1, 0, 255);

	cfg->domain_cfg[instance_index].priority2 = check_bounds_unsigned_int(cfg->domain_cfg[instance_index].priority2, 0, 255);

	cfg->domain_cfg[instance_index].clockClass = check_bounds_unsigned_int(cfg->domain_cfg[instance_index].clockClass, 0, 255);

	cfg->domain_cfg[instance_index].clockAccuracy = check_bounds_unsigned_int(cfg->domain_cfg[instance_index].clockAccuracy, 0, 255);

	cfg->domain_cfg[instance_index].offsetScaledLogVariance = check_bounds_unsigned_int(cfg->domain_cfg[instance_index].offsetScaledLogVariance, 0, 0xFFFF);
}

__init static void process_section_automotive_params(struct fgptp_config *cfg)
{
	int i;

	if ((cfg->neighborPropDelay_mode != CFG_GPTP_PDELAY_MODE_SILENT) &&
		(cfg->neighborPropDelay_mode != CFG_GPTP_PDELAY_MODE_STATIC))
		cfg->neighborPropDelay_mode = CFG_GPTP_PDELAY_MODE_STANDARD;

	/* applying default value to all port */
	for (i = 0; i < CFG_GPTP_MAX_NUM_PORT; i++)
		cfg->initial_neighborPropDelay[i] = check_bounds_unsigned_int(cfg->initial_neighborPropDelay[i], CFG_GPTP_DEFAULT_PDELAY_VALUE_MIN, CFG_GPTP_DEFAULT_PDELAY_VALUE_MAX);

	cfg->neighborPropDelay_sensitivity = check_bounds_unsigned_int(cfg->neighborPropDelay_sensitivity, CFG_GPTP_DEFAULT_PDELAY_SENSITIVITY_MIN, CFG_GPTP_DEFAULT_PDELAY_SENSITIVITY_MAX);
}

__init static void process_section_port_params(struct fgptp_config *cfg)
{
	int i;

	for (i = 0; i < CFG_GPTP_MAX_NUM_PORT; i++) {
		if ((cfg->port_cfg[i].portRole != DISABLED_PORT) &&
			(cfg->port_cfg[i].portRole != SLAVE_PORT))
			cfg->port_cfg[i].portRole = MASTER_PORT;

		cfg->port_cfg[i].ptpPortEnabled = check_bounds_unsigned_int(cfg->port_cfg[i].ptpPortEnabled, CFG_GPTP_DEFAULT_PTP_ENABLED_MIN, CFG_GPTP_DEFAULT_PTP_ENABLED_MAX);

		cfg->port_cfg[i].rxDelayCompensation = check_bounds_int(cfg->port_cfg[i].rxDelayCompensation, CFG_GPTP_DEFAULT_DELAY_COMP_MIN, CFG_GPTP_DEFAULT_DELAY_COMP_MAX);

		cfg->port_cfg[i].txDelayCompensation = check_bounds_int(cfg->port_cfg[i].txDelayCompensation, CFG_GPTP_DEFAULT_DELAY_COMP_MIN, CFG_GPTP_DEFAULT_DELAY_COMP_MAX);

		cfg->port_cfg[i].initialLogPdelayReqInterval = check_bounds_int(cfg->port_cfg[i].initialLogPdelayReqInterval, CFG_GPTP_MIN_LOG_INTERVAL, CFG_GPTP_MAX_LOG_INTERVAL);

		cfg->port_cfg[i].initialLogSyncInterval = check_bounds_int(cfg->port_cfg[i].initialLogSyncInterval, CFG_GPTP_MIN_LOG_INTERVAL, CFG_GPTP_MAX_LOG_INTERVAL);

		cfg->port_cfg[i].initialLogAnnounceInterval = check_bounds_int(cfg->port_cfg[i].initialLogAnnounceInterval, CFG_GPTP_MIN_LOG_INTERVAL, CFG_GPTP_MAX_LOG_INTERVAL);

		cfg->port_cfg[i].operLogPdelayReqInterval = check_bounds_int(cfg->port_cfg[i].operLogPdelayReqInterval, CFG_GPTP_MIN_LOG_INTERVAL, CFG_GPTP_MAX_LOG_INTERVAL);

		cfg->port_cfg[i].operLogSyncInterval = check_bounds_int(cfg->port_cfg[i].operLogSyncInterval, CFG_GPTP_MIN_LOG_INTERVAL, CFG_GPTP_MAX_LOG_INTERVAL);
	}
}

__init static void process_config(struct fgptp_config *cfg)
{
	int instance;

	for (instance = 0; instance < CFG_MAX_GPTP_DOMAINS; instance++) {
		process_section_general(cfg, instance);
		process_section_gm_params(cfg, instance);
	}

	process_section_automotive_params(cfg);
	process_section_port_params(cfg);
}

static void gptp_task(void *pvParameters)
{
	struct gptp_ctx *ctx = pvParameters;
	struct fgptp_config *cfg = ctx->gptp_cfg;
	void *gptp;
	QueueHandle_t event_queue_h;
	unsigned int i;

	os_log(LOG_INIT, "gptp task started\n");

	event_queue_h = xQueueCreate(GPTP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	ctx->event_queue_h = event_queue_h;

	process_config(cfg);

	/*
	 * For a bridge, all ports are using the same local clock
	 */
	cfg->clock_local = logical_port_to_local_clock(cfg->logical_port_list[CFG_DEFAULT_PORT_ID]);

	for (i = 0; i < CFG_MAX_GPTP_DOMAINS; i++) {
		cfg->domain_cfg[i].clock_target = logical_port_to_gptp_clock(cfg->logical_port_list[CFG_DEFAULT_PORT_ID], i);
		cfg->domain_cfg[i].clock_source = cfg->domain_cfg[i].clock_target;
	}

	/*
	* Intialize gptp stack and apply configuration
	*/
	gptp = gptp_init(cfg, (unsigned long)event_queue_h);
	if (!gptp)
		goto err_gptp_init;

	ctx->gptp = gptp;

	xEventGroupSetBits(ctx->event_group_handle, GPTP_TASK_SUCCESS);

	/*
	* Main event loop
	*/
	while (1) {
		BaseType_t handle;
		struct event e;

		handle = xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(10000));
		if (!handle) {
			gptp_stats_dump(gptp);
			continue;
		}

		switch (e.type) {
		case EVENT_TYPE_NET_RX:
			net_rx((struct net_rx *)e.data);
			break;

		case EVENT_TYPE_TIMER:
			os_timer_process((struct os_timer *)e.data);
			break;

		case EVENT_TYPE_NET_TX_TS:
			net_tx_ts_process((struct net_tx *)e.data);
			break;

		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;

		default:
			os_log(LOG_ERR, "xQueueReceivet(): invalid event type(%u)\n", e.type);
			break;
		}

		log_update_time(OS_CLOCK_GPTP_EP_0_0);
	}

	/* Not reached */

err_gptp_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "gptp task exited\n");

	xEventGroupSetBits(ctx->event_group_handle, GPTP_TASK_ERROR);

	vTaskDelete(NULL);
}

__init void *gptp_task_init(struct fgptp_config *gptp_cfg)
{
	struct gptp_ctx *ctx;
	BaseType_t rc;
	EventBits_t bits;

	ctx = pvPortMalloc(sizeof(struct gptp_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	ctx->event_group_handle = xEventGroupCreateStatic(&ctx->event_group);
	if (!ctx->event_group_handle)
		goto err_event_group;

	ctx->gptp_cfg = gptp_cfg;

	rc = xTaskCreate(gptp_task, GPTP_TASK_NAME, GPTP_CFG_STACK_DEPTH, ctx, GPTP_CFG_PRIORITY, &ctx->task_h);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", GPTP_TASK_NAME);
		goto err_task_create;
	}

	bits = xEventGroupWaitBits(ctx->event_group_handle, GPTP_TASK_SUCCESS | GPTP_TASK_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);
	if (bits & GPTP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "gptp main completed\n");

	return ctx;

err_event_group:
err_task_create:
	vPortFree(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void gptp_task_exit(void *handle)
{
	struct gptp_ctx *ctx = handle;

	vTaskDelete(ctx->task_h);

	gptp_exit(ctx->gptp);

	vQueueDelete(ctx->event_queue_h);

	vPortFree(ctx);
}
