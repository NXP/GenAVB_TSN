/*
* Copyright 2017-2020 NXP
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
 @brief FreeRTOS specific Network service implementation
 @details
*/
#include "common/log.h"

#include "net_rx.h"
#include "net_tx.h"
#include "net_socket.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "fp.h"

#define NET_TX_STACK_DEPTH	(configMINIMAL_STACK_SIZE + 256)
#define NET_TX_PRIORITY		(configMAX_PRIORITIES - 2)
#define NET_RX_STACK_DEPTH	(configMINIMAL_STACK_SIZE + 150)
#define NET_RX_PRIORITY		(configMAX_PRIORITIES - 4)

#define TIMER_NET_RX		"net_rx"

#define NET_TX_TASK_NAME	"net tx"
#define NET_RX_TASK_NAME	"net rx"

extern struct net_port ports[CFG_PORTS];

static void net_tx_cleanup(void)
{
	struct net_port *port;
	int i;

	for (i = 0; i < CFG_PORTS; i++) {
		port = &ports[i];

		if (port->tx_up) {
			port_tx_cleanup(port);
		}
	}
}

static void net_tx_task(void *pvParameters)
{
	struct net_tx_ctx *net = pvParameters;
	struct event e;

	os_log(LOG_INIT, "networking(%p) tx task started\n", net);

	for (;;) {
		if (xQueueReceive(net->queue_handle, &e, portMAX_DELAY) != pdTRUE)
			continue;

		switch (e.type) {
		case EVENT_QOS_SCHED:
			port_scheduler_event(e.data);

			net_tx_cleanup();

			break;

		case EVENT_SOCKET_CONNECT:
			socket_connect_event(e.data);
			break;

		case EVENT_SOCKET_DISCONNECT:
			socket_disconnect_event(e.data);
			break;

		case EVENT_PHY_UP:
			port_tx_up(e.data);
			break;

		case EVENT_PHY_DOWN:
			port_tx_down(e.data);
			break;

		case EVENT_SR_CONFIG:
			net_qos_sr_config_event(e.data);
			break;

		case EVENT_FP_SCHED:
			fp_schedule(e.data);
			break;

		default:
			break;
		}
	}

	os_log(LOG_INIT, "networking(%p) tx task(%p) exited\n", net);

	vTaskDelete(NULL);
}


static void net_rx_task(void *pvParameters)
{
	struct net_rx_ctx *net = pvParameters;
	struct event e;
	struct net_port *port;
	unsigned int rx_now;
	int i, queue;

	os_log(LOG_INIT, "networking(%p) rx task started\n", net);

	for (;;) {
		if (xQueueReceive(net->queue_handle, &e, portMAX_DELAY) != pdTRUE)
			continue;

		xSemaphoreTake(net->mutex, portMAX_DELAY);

		switch (e.type) {
		case EVENT_RX_TIMER:
			port = e.data;

			net->time += NET_RX_PERIOD;

			for (queue = port->num_rx_q - 1, rx_now = NET_RX_PACKETS; queue >= 0 && rx_now; queue--)
				rx_now -= port_rx(net, port, rx_now, queue);

			socket_poll_all(net);

			break;

		case EVENT_PHY_UP:
			port = e.data;

			port_up(port);

			if (os_timer_start(&net->timer[port->index].handle, 0, net->timer[port->index].period, 1, 0) < 0)
				os_log(LOG_ERR, "Net rx timer failed to start\n");

			break;

		case EVENT_PHY_DOWN:
			port = e.data;

			port_down(port);

			os_timer_stop(&net->timer[port->index].handle);

			net_rx_flush(net, port);
			break;

		/* Control path events */
		case EVENT_SOCKET_BIND:
			socket_bind_event(e.data);
			break;

		case EVENT_SOCKET_UNBIND:
			socket_unbind_event(e.data);
			break;

		case EVENT_SOCKET_CALLBACK:
			socket_set_callback_event(e.data);
			break;

		case EVENT_SOCKET_OPT:
			socket_set_option_event(e.data);
			break;

		default:
			break;
		}

		xSemaphoreGive(net->mutex);
	}

	for (i = 0; i < CFG_PORTS; i++)
		os_timer_destroy(&net->timer[i].handle);

	os_log(LOG_INIT, "networking(%p) rx task exited\n", net);

	vTaskDelete(NULL);
}

static void net_rx_timer(struct os_timer *t, int count)
{
	struct net_rx_timer *net_t = container_of(t, struct net_rx_timer, handle);
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	struct event e;

	e.type = EVENT_RX_TIMER;
	e.data = net_t->data;

	xQueueSendFromISR(net_rx_ctx.queue_handle, &e, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

__init int net_task_init(void)
{
	int i, j;
	BaseType_t rc;
	struct ptp_ts_ctx *ptp_ctx;

	memset(&net_tx_ctx, 0, sizeof(net_tx_ctx));
	memset(&net_rx_ctx, 0, sizeof(net_rx_ctx));

	net_qos_init(&net_tx_ctx.net_qos);

	logical_port_init();

	net_rx_ctx.mutex = xSemaphoreCreateMutexStatic(&net_rx_ctx.mutex_buffer);
	if (!net_rx_ctx.mutex) {
		os_log(LOG_ERR, "xSemaphoreCreateMutexStatic() failed\n");
		goto err_net_rx_mutex;
	}

	net_rx_ctx.queue_handle = xQueueCreateStatic(NET_RX_EVENT_QUEUE_LENGTH, sizeof(struct event),
						net_rx_ctx.queue_buffer, &net_rx_ctx.queue);
	if (!net_rx_ctx.queue_handle) {
		os_log(LOG_ERR, "xQueueCreateStatic(net rx) failed\n");

		goto err_net_rx_queue;
	}

	for (i = 0; i < CFG_PORTS; i++) {
		ptp_ctx = &net_tx_ctx.ptp_ctx[i];

		ptp_ctx->tx_ts_queue_handle = xQueueCreateStatic(PTP_TX_TS_QUEUE_LENGTH, sizeof(struct socket_tx_ts_data),
								ptp_ctx->tx_ts_queue_buffer, &ptp_ctx->tx_ts_queue);
		if (!ptp_ctx->tx_ts_queue_handle) {
			os_log(LOG_ERR, "xQueueCreateStatic(tx ts) failed\n");

			goto err_net_tx_ts_queue;
		}
	}

	net_tx_ctx.queue_handle = xQueueCreateStatic(NET_TX_EVENT_QUEUE_LENGTH, sizeof(struct event),
						net_tx_ctx.queue_buffer, &net_tx_ctx.queue);
	if (!net_tx_ctx.queue_handle) {
		os_log(LOG_ERR, "xQueueCreateStatic(net tx) failed\n");

		goto err_net_tx_queue;
	}

	for (i = 0; i < CFG_PORTS; i++) {
		net_rx_ctx.timer[i].data = &ports[i];
		net_rx_ctx.timer[i].period = NET_RX_PERIOD;

		if (os_timer_create(&net_rx_ctx.timer[i].handle, OS_CLOCK_SYSTEM_MONOTONIC, 0, net_rx_timer, 0) < 0) {
			os_log(LOG_ERR, "os_timer_create() failed\n");
			goto err_net_rx_timer;
		}
	}

	rc = xTaskCreate(net_tx_task, NET_TX_TASK_NAME, NET_TX_STACK_DEPTH,
			&net_tx_ctx, NET_TX_PRIORITY, &net_tx_ctx.handle);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", NET_TX_TASK_NAME);
		goto err_net_tx;
	}

	rc = xTaskCreate(net_rx_task, NET_RX_TASK_NAME, NET_RX_STACK_DEPTH,
		&net_rx_ctx, NET_RX_PRIORITY, &net_rx_ctx.handle);
	if (rc != pdPASS) {
		os_log(LOG_ERR, "xTaskCreate(%s) failed\n", NET_RX_TASK_NAME);
		goto err_net_rx;
	}

	os_log(LOG_INIT, "networking started\n");

	return 0;

err_net_rx:
	vTaskDelete(net_tx_ctx.handle);

err_net_tx:
err_net_rx_timer:
	for (j = 0 ; j < i; j++)
		os_timer_destroy(&net_rx_ctx.timer[j].handle);

err_net_tx_queue:
err_net_tx_ts_queue:
err_net_rx_queue:
err_net_rx_mutex:

	return -1;
}

__exit void net_task_exit(void)
{
	int i;

	vTaskDelete(net_rx_ctx.handle);

	vTaskDelete(net_tx_ctx.handle);

	for (i = 0; i < CFG_PORTS; i++)
		os_timer_destroy(&net_rx_ctx.timer[i].handle);

	os_log(LOG_INIT, "networking stopped\n");
}
