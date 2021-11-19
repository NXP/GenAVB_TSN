/*
* Copyright 2018, 2020 NXP
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
 @brief NXP AVDECC FreeRTOS specific code
 @details Setups FreeRTOS task for NXP AVDECC stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common/ipc.h"
#include "common/log.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/sys_types.h"
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "avdecc/avdecc_entry.h"

#define AVDECC_TASK_NAME		"AVDECC Stack"

const struct avdecc_config avdecc_default_config = {
	.log_level = avdecc_CFG_LOG,
	.enabled = 1,
	.srp_enabled = 1,
	.num_entities = 1,
	.max_entities_discovery = CFG_ADP_DEFAULT_NUM_ENTITIES_DISCOVERY,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].valid_time = CFG_ADP_DEFAULT_VALID_TIME,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_listener_pairs = CFG_ACMP_DEFAULT_NUM_LISTENER_PAIRS,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_listener_streams = 3,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_talker_streams = 3,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_inflights = CFG_AVDECC_DEFAULT_NUM_INFLIGHTS,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_unsolicited_registrations = CFG_AECP_DEFAULT_NUM_UNSOLICITED,
};


/**
 *
 */
__init static void clip_config_values(unsigned int *val, unsigned int min, unsigned int max)
{
	if (*val < min)
		*val = min;

	if (*val > max)
		*val = max;
}

__init static void process_avdecc_entity(struct avdecc_entity_config *entity_cfg)
{
	entity_cfg->flags &= (AVDECC_FAST_CONNECT_MODE | AVDECC_FAST_CONNECT_BTB);

	clip_config_values(&entity_cfg->talker_entity_id_n, 0, ACMP_CFG_MAX_UNIQUE_ID);
	clip_config_values(&entity_cfg->talker_unique_id_n, 0, ACMP_CFG_MAX_UNIQUE_ID);
	clip_config_values(&entity_cfg->listener_unique_id_n, 0, ACMP_CFG_MAX_UNIQUE_ID);
	clip_config_values(&entity_cfg->valid_time, CFG_ADP_MIN_VALID_TIME, CFG_ADP_MAX_VALID_TIME);
	clip_config_values(&entity_cfg->max_listener_streams, CFG_ACMP_MIN_NUM_LISTENER_STREAMS, CFG_ACMP_MAX_NUM_LISTENER_STREAMS);
	clip_config_values(&entity_cfg->max_talker_streams, CFG_ACMP_MIN_NUM_TALKER_STREAMS, CFG_ACMP_MAX_NUM_TALKER_STREAMS);
	clip_config_values(&entity_cfg->max_listener_pairs, CFG_ACMP_MIN_NUM_LISTENER_PAIRS, CFG_ACMP_MAX_NUM_LISTENER_PAIRS);
	clip_config_values(&entity_cfg->max_inflights, CFG_AVDECC_MIN_NUM_INFLIGHTS, CFG_AVDECC_MAX_NUM_INFLIGHTS);
	clip_config_values(&entity_cfg->max_unsolicited_registrations, CFG_AECP_MIN_NUM_UNSOLICITED, CFG_AECP_MAX_NUM_UNSOLICITED);
}

/**
 *
 */
__init static void process_config(struct avdecc_config *avdecc_cfg)
{
	int entity_index = 0;

	avdecc_cfg->enabled &= 0x1;

	avdecc_cfg->srp_enabled &= 0x1;

	if (avdecc_cfg->num_entities > CFG_AVDECC_NUM_ENTITIES)
		avdecc_cfg->num_entities = CFG_AVDECC_NUM_ENTITIES;

	for (entity_index = 0; entity_index < avdecc_cfg->num_entities; entity_index++) {
		process_avdecc_entity(&avdecc_cfg->entity_cfg[entity_index]);
	}
}

/**
 *
 */
static void avdecc_task(void *pvParameters)
{
	struct avdecc_config *avdecc_cfg = pvParameters;
	void *avdecc = NULL;
	QueueHandle_t event_queue_h;

	os_log(LOG_INIT, "avdecc task init\n");

	event_queue_h = xQueueCreate(AVDECC_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event));
	if (!event_queue_h) {
		os_log(LOG_ERR, "xQueueCreate() failed\n");
		goto err_queue_create;
	}

	/**
	 * AVDECC Config
	 */
	process_config(avdecc_cfg);

	avdecc = avdecc_init(avdecc_cfg, (unsigned long)event_queue_h);
	if (!avdecc)
		goto err_avdecc_init;

	os_log(LOG_INIT, "started\n");

	/**
	 * Main loop
	 */
	while (1) {
		struct event e;

		if (xQueueReceive(event_queue_h, &e, pdMS_TO_TICKS(10000)) != pdTRUE)
			continue;

		switch (e.type) {
		case EVENT_TYPE_NET_RX:
			net_rx((struct net_rx *) e.data);
			break;
		case EVENT_TYPE_TIMER:
			os_timer_process((struct os_timer *)e.data);
			break;
		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;
		default:
			os_log(LOG_ERR, "xQueueReceive(): invalid event type(%u)\n", e.type);
			break;
		}
	}

	avdecc_exit(avdecc);

err_avdecc_init:
	vQueueDelete(event_queue_h);

err_queue_create:
	os_log(LOG_INIT, "avdecc task exited\n");

	vTaskDelete(NULL);
}

/**
 *
 */
__init TaskHandle_t avdecc_task_init(struct avdecc_config *avdecc_cfg)
{
	TaskHandle_t task;
	BaseType_t rc;

	rc = xTaskCreate(avdecc_task, AVDECC_TASK_NAME, AVDECC_CFG_STACK_DEPTH, avdecc_cfg, AVDECC_CFG_PRIORITY, &task);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", AVDECC_TASK_NAME);
		goto err;
	}

	os_log(LOG_INIT, "avdecc main completed\n");

	return task;

err:
	return NULL;
}

__exit void avdecc_task_exit(TaskHandle_t task)
{
}
