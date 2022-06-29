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

#ifndef _FREERTOS_NET_PORT_H_
#define _FREERTOS_NET_PORT_H_

#include "os/clock.h"
#include "os/net.h"

#include "genavb/ptp.h"
#include "genavb/net_types.h"
#include "genavb/qos.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "task.h"

#include "port_config.h"

#define NET_PAYLOAD_SIZE_MAX		1518
#define NET_DATA_SIZE			DEFAULT_NET_DATA_SIZE

#if (NET_PAYLOAD_SIZE_MAX > (NET_DATA_SIZE - NET_DATA_OFFSET))
#error "Invalid NET_PAYLOAD_SIZE_MAX"
#endif

#define PORT_ERROR			(1 << 0)
#define PORT_SUCCESS			(1 << 1)
#define PORT_TX_SUCCESS			(1 << 2)

struct net_rx_ctx;
struct net_tx_ctx;
struct net_port;

struct net_port_status {
	uint16_t port;
	bool up;
	bool full_duplex;
	unsigned int rate;
};

struct ptp_frame_data {
	uint8_t version;
	struct ptp_port_identity src_port_id;
	uint16_t sequence_id;
	uint8_t message_type;
};

struct port_tx_ts_data {
	uint32_t priv;
	unsigned int queue;
	struct ptp_frame_data ptp_data;
};

#define TX_QUEUE_FLAGS_STRICT_PRIORITY (1 << 0)
#define TX_QUEUE_FLAGS_CREDIT_SHAPER   (1 << 1)

#define TX_QUEUE_PROP_MAX 8

struct tx_queue_properties {
	unsigned int num_queues;
	unsigned int queue_prop[TX_QUEUE_PROP_MAX];
};

#define NET_PORT_TX_TS_QUEUE_LENGTH 8

typedef enum {
	ENET_t,
	ENET_1G_t,
	ENET_QOS_t,
	ENET_MAX_t
} enet_type_t;

struct net_port_drv_ops {
	int (*add_multi)(struct net_port *port, uint8_t *addr);
	int (*del_multi)(struct net_port *port, uint8_t *addr);
	int (*get_rx_frame_size)(struct net_port *port, uint32_t *length, uint32_t queue);
	int (*read_frame)(struct net_port *port, uint8_t *data, uint32_t length, uint64_t *ts, uint32_t queue);
	int (*send_frame)(struct net_port *port, uint8_t *data, uint32_t length, struct net_tx_desc *desc, uint32_t queue, bool need_ts);
	void (*tx_cleanup)(struct net_port *port);
	void (*link_up)(struct net_port *port);
	void (*link_down)(struct net_port *port);
	int (*set_tx_queue_config)(struct net_port *port, struct tx_queue_properties *cfg);
	int (*set_tx_idle_slope)(struct net_port *port, unsigned int idle_slope, uint32_t queue);
	int (*set_st_config)(struct net_port *port, struct genavb_st_config *config);
	int (*get_st_config)(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length);
	int (*set_fp)(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
	int (*get_fp)(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
	int (*stats_get_number)(struct net_port *port);
	int (*stats_get_strings)(struct net_port *port, char *buf, unsigned int buf_len);
	int (*stats_get)(struct net_port *port, uint64_t *buf, unsigned int buf_len);
	int (*init)(struct net_port *port);
	void (*exit)(struct net_port *port);
	int (*post_init)(struct net_port *port);
	void (*pre_exit)(struct net_port *port);
};

struct timer_cfg {
	unsigned int channel;
};

#define MAX_QUEUES 5

struct queue_stats {
	unsigned int tx;
	unsigned int tx_err;
	unsigned int tx_ts_err;
	unsigned int rx;
	unsigned int rx_err;
	unsigned int rx_alloc_err;
};

struct net_port {
	unsigned int index;
	int pps_timer_channel; /* Channel number for a pps signal, otherwise -1 */

	enet_type_t drv_type;
	unsigned int drv_index;
	void *drv;

	uintptr_t base;
	unsigned int mii_mode;
	unsigned int phy_index;

	struct net_port_drv_ops drv_ops;

	struct timer_cfg timers[3];
	struct timer_cfg timer_event;

	uint8_t mac_addr[6];
	uint32_t phy_addr;
	bool up;		/* port up status, protected by mutex */
	bool tx_up;		/* port up status, only used by net tx task */
	struct port_qos *qos;

	struct logical_port *logical_port;

	unsigned int num_rx_q;
	unsigned int num_tx_q;

	struct tx_queue_properties *tx_q_cap;

	os_clock_id_t clock_local;
	os_clock_id_t clock_gptp;

	struct hw_clock *hw_clock;

	TaskHandle_t phy_task_handle;

	unsigned int phy_speed;
	unsigned int phy_duplex;
	unsigned int old_phy_speed;
	unsigned int old_phy_duplex;

	StaticSemaphore_t mutex_buffer[MAX_QUEUES];
	SemaphoreHandle_t mutex[MAX_QUEUES];

	StaticSemaphore_t config_mutex_buffer;
	SemaphoreHandle_t config_mutex;

	EventGroupHandle_t event_group_handle;

	StaticEventGroup_t event_group;

	uint32_t rx_tstamp_latency;
	uint32_t tx_tstamp_latency;

	struct queue_stats stats[MAX_QUEUES];
};

extern struct net_port ports[CFG_PORTS];

int port_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *cfg);
int port_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, unsigned int queue);
unsigned int port_tx_queue_prop_num_cbs(struct tx_queue_properties *cfg);
unsigned int port_tx_queue_prop_num_sp(struct tx_queue_properties *cfg);
uint8_t *port_get_hwaddr(struct net_port *port);
int port_status(struct net_port *port, struct net_port_status *status);
int port_add_multi(struct net_port *port, uint8_t *addr);
int port_del_multi(struct net_port *port, uint8_t *addr);
uint32_t port_gettime32(struct net_port *port);
uint64_t port_gettime64(struct net_port *port);
void port_tx_cleanup(struct net_port *port);
int port_tx_ts(struct net_port *port);
unsigned int port_rx(struct net_rx_ctx *, struct net_port *port, unsigned int n, unsigned int queue);
int port_tx(struct net_port *port, struct net_tx_desc *desc, unsigned int queue);
void port_up(struct net_port *port);
void port_down(struct net_port *port);
void port_tx_up(struct net_port *port);
void port_tx_down(struct net_port *port);
int port_set_st_config(struct net_port *port, struct genavb_st_config *config);
int port_get_st_config(struct net_port *port, genavb_st_config_type_t type,
		       struct genavb_st_config *config, unsigned int list_length);
int port_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
int port_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
int port_stats_get_number(struct net_port *port);
int port_stats_get_strings(struct net_port *port, char *buf, unsigned int buf_len);
int port_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len);
int port_init(void);
void port_exit(void);
int port_post_init(void);
void port_pre_exit(void);


static inline struct logical_port *physical_to_logical_port(struct net_port *port)
{
	return port->logical_port;
}

static inline void port_tx_clean_desc(struct net_port *port, unsigned long priv)
{
	net_tx_free((struct net_tx_desc *)priv);
}

#endif /* _FREERTOS_NET_PORT_H_ */
