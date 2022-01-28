/*
* SJA1105 glue layer functions
* Copyright 2016 Freescale Semiconductor, Inc.
* Copyright 2021 NXP
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <asm/div64.h>

#include "genavb/ether.h"
#include "genavb/qos.h"

#include "avbdrv.h"
#include "ptp.h"
#include "mrp.h"
#include "debugfs.h"
#include "sja1105.h"

/* SJA1105 HAL headers files */
#include "NXP_SJA1105_ethIf.h"
#include "NXP_SJA1105_mgmtRoutes.h"
#include "NXP_SJA1105_config.h"

#include "genavb/ptp.h"

static const u8 ptp_dst_mac[6] = MC_ADDR_PTP;


static struct switch_drv *sjadrv;
static struct sja1105_ts_desc egress_ts_array[NUM_PORT][NUM_TS_SLOT]; //FIXME maybe should be retrieved from HAL config.h
static u8 g_host_port; /* master switch logical host port */
static u8 g_host_port_mask; /* master switch host port mask as handled by the HAL */

int sja1105_add_multi(struct logical_port *port, struct net_mc_address *addr)
{
	/* Nothing to do on the switch itself, but host port multicast filter
	 * needs to be configured */
	return eth_avb_add_multi(port->eth, addr);
}

int sja1105_del_multi(struct logical_port *port, struct net_mc_address *addr)
{
	return eth_avb_del_multi(port->eth, addr);
}

static int __sja1105_metaframe_rx(struct eth_avb *eth, struct net_rx_desc *desc)
{
	pool_dma_free(eth->buf_pool, desc);

	return AVB_NET_RX_DROP;
}


int sja1105_metaframe_rx(struct eth_avb *eth, struct net_rx_desc *desc)
{
	if (sja1105_rx_from_network(eth, desc, PTYPE_META) < 0)
		pool_dma_free(eth->buf_pool, desc);

	/* in any cases, the metaframe will be dropped */
	return AVB_NET_RX_DROP;
}


/** net_type_to_hal - translate packet tpe value from genavb to HAL
 * \return 0 on failure else packet type as defined by the HAL
 * \param net_type packet type as defined in genavb net_types.h
 */
static u8 net_type_to_hal(u8 net_type)
{
	u8 frame_type = 0;

	switch (net_type) {
		case PTYPE_PTP:
			frame_type = DESC_FLAG_PTP_FRAME_MASK;
			break;
		case PTYPE_MRP:
			frame_type = DESC_FLAG_SRP_FRAME_MASK;
			break;
		case PTYPE_META:
			frame_type = DESC_FLAG_META_FRAME_MASK;
			break;
		default:
			break;
	}

	return frame_type;
}


/** is_endpoint - check if a packet belongs to the endpoint stack
 * \return 1 if packet is of endpoint type else return 0
 * \param desc avb tx packet descriptor
 */
static u8 is_endpoint(struct avb_tx_desc *desc)
{
	if (desc->common.flags & AVB_TX_FLAG_AED_E)
		return 1;
	else
		return 0;
}


/** is_bridge - check if a packet belongs to the bridge stack
 * \return 1 if packet is of bridge type else return 0
 * \param desc avb tx packet descriptor
 */
static u8 is_bridge(struct avb_tx_desc *desc)
{
	if (desc->common.flags & AVB_TX_FLAG_AED_B)
		return 1;
	else
		return 0;
}


static u8 is_port_ready(struct switch_drv *drv)
{
	return drv->ready;
}


/** sja1105_egress_ts_get_data_by_index - find entry in the TS index storage array matching the index returned by the HAL egress TS callback
 * \return pointer to struct sja1105_ts_desc on success or NULL on failure
 * \param port
 * \param index
 */
static struct sja1105_ts_desc * sja1105_egress_ts_get_data_by_index(u8 port, u8 index)
{
	struct sja1105_ts_desc *ts_desc = NULL;
	int slot;

	for (slot = 0; slot < NUM_TS_SLOT; slot++) {
		if ((egress_ts_array[port][slot].index != UNUSED_TS_SLOT) && (egress_ts_array[port][slot].index == index)) {
			ts_desc = &egress_ts_array[port][slot];
			break;
		}
	}

	return ts_desc;
}


/** sja1105_egress_ts_get_data_by_handle - find entry in the TS index storage array matching the network descriptor it corresponds to
 * \return pointer to struct sja1105_ts_desc on success or NULL on failure
 * \param port
 * \param desc
 */
static struct sja1105_ts_desc * sja1105_egress_ts_get_data_by_handle(u8 port, void *desc)
{
	u8 slot;
	struct sja1105_ts_desc *ts_desc = NULL;

	for (slot = 0; slot < NUM_TS_SLOT; slot++) {
		if ((egress_ts_array[port][slot].index != UNUSED_TS_SLOT) && (egress_ts_array[port][slot].data2 == desc)) {
			ts_desc = &egress_ts_array[port][slot];
			break;
		}
	}

	return ts_desc;
}


/** sja1105_egress_ts_allocate_slot - allocate one slot from the TS index storage array
 * \return pointer to egress time array slot or NULL on failure
 * \param port
 */
static struct sja1105_ts_desc * sja1105_egress_ts_allocate_slot(u8 port)
{
	int slot;
	struct sja1105_ts_desc *ts_desc = NULL;

	for (slot = 0; slot < NUM_TS_SLOT; slot++) {
		if (egress_ts_array[port][slot].index == UNUSED_TS_SLOT) {
				egress_ts_array[port][slot].flags = 0;
				ts_desc = &egress_ts_array[port][slot];
				sjadrv->stats.num_egress_ts_alloc[port]++;
				goto slot_allocated;
		}
	}

	sjadrv->stats.num_egress_ts_alloc_err[port]++;

slot_allocated:
	return ts_desc;
}

/** sja1105_egress_ts_release_slot - free one slot of the TS index storage array
 * \return none
 * \param port
 * \param slot
 */
static void sja1105_egress_ts_release_slot(u8 port, u8 slot)
{
	egress_ts_array[port][slot].flags = 0;
	egress_ts_array[port][slot].index = UNUSED_TS_SLOT;
	egress_ts_array[port][slot].data1 = NULL;
	egress_ts_array[port][slot].data2 = NULL;
	sjadrv->stats.num_egress_ts_free[port]++;
}


/** sja1105_egress_ts_store_index - associates a TS index returned by the HAL with a corresponding packet descriptor
 * \return pointer to egress time array slot or NULL on failure
 * \param port
 * \param data1
 * \param data2
 * \param index
 */
static struct sja1105_ts_desc * sja1105_egress_ts_store_index(u8 port, void *data1, void *data2, u8 index)
{
	struct sja1105_ts_desc *ts_desc;

	/* check if data2 is not already stored */
	if ((ts_desc = sja1105_egress_ts_get_data_by_handle(port, data2)) != NULL) {
		/* this means an old slot with same descriptor address has never been released from the
		egress_ts_array, let's reuse the already allocated slot and log an error
		*/
		sjadrv->stats.num_egress_ts_store_err[port]++;
	} else {
		ts_desc = sja1105_egress_ts_allocate_slot(port);
	}

	if (ts_desc) {
		ts_desc->index = index;
		ts_desc->data1 = data1;
		ts_desc->data2 = data2;
	}

	return ts_desc;
}


/** sja1105_egress_ts_notifier - notify egress timestamp to upper layer
 * \return none
 * \param ts_desc
 */
static void sja1105_egress_ts_notifier(struct sja1105_ts_desc *ts_desc)
{
	struct net_tx_desc *desc;
	struct eth_avb *eth;
	struct logical_port *port;

	/* notify egress timestamp to upper layer once the switch HW as returned a timestamp
	and the descriptor ownership is back to the application */
	if (ts_desc->flags == (FLAG_TS_RECEIVED | FLAG_DESC_DONE)) {
		eth = (struct eth_avb *)ts_desc->data1;
		desc = (struct net_tx_desc *)ts_desc->data2;
		desc->ts = (u32)ts_desc->timestamp;
		desc->ts64 = ts_desc->timestamp;

		port = bridge_to_logical_port(0, ts_desc->port);

		if (ptp_tx_ts(port, desc) == AVB_WAKE_THREAD)
			ptp_tx_ts_wakeup(port);

		sja1105_egress_ts_release_slot(ts_desc->port, ts_desc->slot);
	}
}


/** sja1105_egress_ts - called back by the HAL to signal an egress timestamp. This function retrieves the private data (tx descriptor)
 * from the previously stored SJA1105 frame descriptor and post the TS to the upper layer
 * \return none
 * \param timestamp
 * \param port
 * \param index
 */
static void sja1105_egress_ts(u64 timestamp, u8 port, unsigned char index)
{
	struct sja1105_ts_desc *ts_desc;
	u64 timestamp_ns = (timestamp << 3); /* SJA1105 HAL timestamps are expressed in multiple of 8ns */

	ts_desc = sja1105_egress_ts_get_data_by_index(port, index);
	if (ts_desc) {
		ts_desc->timestamp = timestamp_ns;
		ts_desc->flags |= FLAG_TS_RECEIVED;

		sjadrv->stats.num_egress_ts[port]++;

		sja1105_egress_ts_notifier(ts_desc);
	} else {
		sjadrv->stats.num_egress_ts_lookup_err++;
	}
}



static void sja1105_egress_ts_to_host(void)
{
	struct sja1105_ts_desc *ts_desc;
	struct net_tx_desc *desc;
	struct avb_drv *avb;

	if (queue_pending(&sjadrv->egress_ts_queue)) {
		desc = (struct net_tx_desc *)queue_dequeue(&sjadrv->egress_ts_queue);
		sjadrv->stats.egress_ts_queue_lvl--;
		sjadrv->stats.egress_ts_dequeue_count++;
		ts_desc = sja1105_egress_ts_get_data_by_handle((desc->priv >> 8) & 0xFF, desc);
		if (ts_desc) {
			ts_desc->flags |= FLAG_DESC_DONE;
			sja1105_egress_ts_notifier(ts_desc);
		} else {
			sjadrv->stats.num_egress_ts_lookup_err++;
			/* free the dequeued descriptor here, to prevent leakage... */
			avb = container_of(sjadrv, struct avb_drv, switch_drv);
			pool_dma_free(&avb->buf_pool, desc);
		}
	}
}


static void sja1105_egress_ts_queue_flush(struct net_tx_desc *desc)
{
	struct avb_drv *avb = container_of(sjadrv, struct avb_drv, switch_drv);

	sjadrv->stats.egress_ts_queue_lvl--;
	sjadrv->stats.egress_ts_dequeue_count++;

	pool_dma_free(&avb->buf_pool, desc);
}


int sja1105_egress_ts_done(struct switch_drv *drv, struct eth_avb *eth, struct net_tx_desc *desc)
{
	int rc = queue_enqueue(&drv->egress_ts_queue, (unsigned long)desc);

	if (rc < 0)
		drv->stats.egress_ts_queue_full++;
	else {
		drv->stats.egress_ts_queue_lvl++;
		drv->stats.egress_ts_queue_count++;
	}

	wake_up_process(drv->kthread);

	return 0;
}

/** sja1105_egress_ts_flush_all - flushes the egress timestamp array and free any pending net descriptor
 * \return none
 */
static void sja1105_egress_ts_flush_all(void)
{
	u8 port, slot;
	struct eth_avb *eth;
	struct net_tx_desc *desc;

	for (port = 0; port < NUM_PORT; port++) {
		for (slot = 0; slot < NUM_TS_SLOT; slot++) {
			if (egress_ts_array[port][slot].index != UNUSED_TS_SLOT) {
				eth = (struct eth_avb *)egress_ts_array[port][slot].data1;
				desc = (struct net_tx_desc *)egress_ts_array[port][slot].data2;

				/* free the net descriptor if flag is DONE, means no one else has ownership on it */
				if (egress_ts_array[port][slot].flags & FLAG_DESC_DONE) {
					pr_err("%s: flushing pending desc(%p)\n", __func__, desc);
					pool_dma_free(eth->buf_pool, desc);
				}

				sja1105_egress_ts_release_slot(port, slot);
			}
		}
	}
}


/** sja1105_egress_ts_init - initializes the egress timestamp array
 * \return none
 */
static void sja1105_egress_ts_init(void)
{
	u8 port, slot;

	for (port = 0; port < NUM_PORT; port++) {
		for (slot = 0; slot < NUM_TS_SLOT; slot++) {
			egress_ts_array[port][slot].port = port;
			egress_ts_array[port][slot].slot = slot;
			egress_ts_array[port][slot].flags = 0;
			egress_ts_array[port][slot].index = UNUSED_TS_SLOT;
			egress_ts_array[port][slot].data1 = NULL;
			egress_ts_array[port][slot].data2 = NULL;
		}
	}
}


/** sja1105_desc_alloc - allocates memory for a SJA1105 frame descriptor
 * \return valid pointer or NULL if buffers pool is empty
 * \param none
 */
static void * sja1105_desc_alloc(void)
{
	void *buf;

	buf = pool_alloc(&sjadrv->buf_pool);

	if (!buf)
		sjadrv->stats.num_alloc_err++;

	return buf;
}


/** sja1105_tx_desc_alloc - allocates and initializes a SJA1105 frame descriptor for transmit
 * \return valid pointer or NULL in case of failure
 * \param none
 */
static SJA1105_frameDescriptor_t * sja1105_tx_desc_alloc(void)
{
	SJA1105_frameDescriptor_t *sja_desc;

	sja_desc = (SJA1105_frameDescriptor_t *)sja1105_desc_alloc();

	if(!sja_desc)
		goto err_alloc;

	memset(sja_desc, 0, sizeof(SJA1105_frameDescriptor_t));

	sjadrv->stats.num_tx_desc_alloc++;

err_alloc:
	return sja_desc;
}



/** sja1105_rx_desc_alloc - allocates and initializes a SJA1105 frame descriptor for receive
 * \return valid pointer or NULL in case of failure
 * \param none
 */
static SJA1105_frameDescriptor_t * sja1105_rx_desc_alloc(void)
{
	SJA1105_frameDescriptor_t *sja_desc;

	sja_desc = (SJA1105_frameDescriptor_t *)sja1105_desc_alloc();

	if (!sja_desc)
		goto err_alloc;

	memset(sja_desc, 0, sizeof(SJA1105_frameDescriptor_t));

	sjadrv->stats.num_rx_desc_alloc++;

err_alloc:
	return sja_desc;
}


/** sja1105_desc_free - free memory allocated for a SJA1105 frame descriptor
 * \return none
 * \param sja_desc pointer to the descriptor to free
 */
void sja1105_desc_free(void *data, unsigned long sja_desc)
{
	pool_free((struct pool *)data, (void*)sja_desc);

	sjadrv->stats.num_desc_free++;
}


static void sja1105_done(void *sja_desc)
{
	void *desc;
	struct eth_avb *eth;

	eth = (struct eth_avb *)((SJA1105_frameDescriptor_t *)sja_desc)->txPrivate1;
	desc = (void *)((SJA1105_frameDescriptor_t *)sja_desc)->txPrivate2;
	pool_dma_free(eth->buf_pool, desc);
	sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)sja_desc);
}


static void sja1105_rx_done(void *sja_desc)
{
	if(sja_desc) {
		sja1105_done(sja_desc);
		sjadrv->stats.num_desc_free_rx++;
	}
}


static void sja1105_tx_done(void *sja_desc)
{
	if(sja_desc) {
		sja1105_done(sja_desc);
		sjadrv->stats.num_desc_free_tx++;
	}
}


/** sja1105_ep_setup_tx_desc - fill in a sja descriptor for endpoint interface
 * \return none
 * \param eth
 * \param tx_desc
 * \param sja_desc
 * \param frame_type
 */
static void sja1105_ep_setup_tx_desc(struct logical_port *port, const struct avb_tx_desc *tx_desc, SJA1105_frameDescriptor_t *sja_desc, u8 frame_type)
{
	sja_desc->txPrivate1 = (uintptr_t)port->eth;
	sja_desc->txPrivate2 = (uintptr_t)tx_desc;

	sja_desc->flags = net_type_to_hal(frame_type);
	sja_desc->len = (u16)tx_desc->common.len;
	sja_desc->ports = g_host_port_mask;
}



/** sja1105_sw_setup_tx_desc -  fill in a sja descriptor for switch interface
 * \return none
 * \param eth
 * \param tx_desc
 * \param sja_desc
 * \param frame_type
 */
static void sja1105_sw_setup_tx_desc(struct logical_port *port, struct avb_tx_desc *tx_desc, SJA1105_frameDescriptor_t *sja_desc, u8 frame_type)
{
	sja_desc->txPrivate1 = (uintptr_t)port->eth;
	sja_desc->txPrivate2 = (uintptr_t)tx_desc;

	sja_desc->flags = net_type_to_hal(frame_type);

	tx_desc->common.private &= ~0xff00;
	tx_desc->common.private |= (port->bridge.port << 8);

	if (tx_desc->common.flags & AVB_TX_FLAG_HW_TS)
		sja_desc->flags |= DESC_FLAG_TAKE_TIME_STAMP_MASK;

	sja_desc->len = (u16)tx_desc->common.len;
	sja_desc->ports = 1 << port->bridge.port;
}


/** sja1105_setup_rx_desc -  fill in a sja descriptor for switch interface
 * \return none
 * \param eth
 * \param rx_desc
 * \param sja_desc
 * \param frame_type
 */
static void sja1105_setup_rx_desc(struct eth_avb *eth, const struct net_rx_desc *rx_desc, SJA1105_frameDescriptor_t *sja_desc, u8 frame_type)
{
	sja_desc->txPrivate1 = (uintptr_t)eth;
	sja_desc->txPrivate2 = (uintptr_t)rx_desc;

	sja_desc->len = (u16)rx_desc->len;
	sja_desc->port = 0;
	sja_desc->flags = net_type_to_hal(frame_type);
}



/** sja1105_rx_enqueue - queue a sja descriptor to the receive queue
 * \return 0 on success or negative value on failure
 * \param sja_desc pointer to the sja descriptor to queue
 */
static int sja1105_rx_enqueue(void *sja_desc)
{
	int rc;

	rc = queue_enqueue(&sjadrv->rx_queue, (unsigned long)sja_desc);

	if (rc < 0)
		sjadrv->stats.rx_queue_full++;
	else {
		sjadrv->stats.rx_queue_lvl++;
		sjadrv->stats.rx_enqueue_count++;
	}

	return rc;
}


/** sja1105_ep_tx_enqueue - enqueue a sja descriptor to the endpoint transmit queue
 * \return 0 on success or negative value on failure
 * \param sja_desc pointer to the sja descriptor to queue
 */
static int sja1105_ep_tx_enqueue(SJA1105_frameDescriptor_t *sja_desc)
{
	int rc = queue_enqueue(&sjadrv->tx_queue_ep, (unsigned long)sja_desc);

	if (rc < 0)
		sjadrv->stats.tx_queue_ep_full++;
	else {
		sjadrv->stats.tx_queue_ep_lvl++;
		sjadrv->stats.tx_queue_ep_count++;
	}

	return rc;
}



/** sja1105_sw_tx_enqueue - enqueue a sja descriptor to the switch transmit queue
 * \return 0 on success or negative value on failure
 * \param sja_desc pointer to the sja descriptor to queue
 */
static int sja1105_sw_tx_enqueue(SJA1105_frameDescriptor_t *sja_desc)
{
	int rc = queue_enqueue(&sjadrv->tx_queue_sw, (unsigned long)sja_desc);

	if (rc < 0)
		sjadrv->stats.tx_queue_sw_full++;
	else {
		sjadrv->stats.tx_queue_sw_lvl++;
		sjadrv->stats.tx_queue_sw_count++;
	}

	return rc;
}


/** sja1105_ep_rx_to_host - called by the HAL to put a endpoint frame to the upper layer
 * \return none
 * \param sja_desc
 * \param p_data
 */
static void sja1105_ep_rx_to_host(const SJA1105_frameDescriptor_t *sja_desc, const u8 *p_data)
{
	struct eth_avb *eth = (struct eth_avb *)sja_desc->txPrivate1;
	struct net_rx_desc *rx_desc = (struct net_rx_desc *)sja_desc->txPrivate2;
	struct logical_port *port;

	port = physical_to_logical_port(eth);

	if (!is_port_ready(sjadrv) || !port) {
		pool_dma_free(eth->buf_pool, rx_desc);
		goto err_not_ready;
	}

	sjadrv->stats.rx_to_host_ep++;

	/* call frame receive function here - send packet to upper layer */
	switch (sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK) {
		case DESC_FLAG_PTP_FRAME_MASK:
			sjadrv->stats.rx_to_host_ep_ptp++;
			__ptp_rx(port, rx_desc);
			break;

		case DESC_FLAG_SRP_FRAME_MASK:
			sjadrv->stats.rx_to_host_ep_srp++;
			__mrp_rx(port, rx_desc);
			break;

		case DESC_FLAG_META_FRAME_MASK:
			sjadrv->stats.rx_to_host_ep_meta++;
			__sja1105_metaframe_rx(eth, rx_desc);
			break;

		default:
			printk("%s: unknown frame type (0x%x)\n", __func__, (sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK));
			break;
	}

err_not_ready:
	sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)sja_desc);
	sjadrv->stats.num_desc_free_rx++;
	sjadrv->stats.num_desc_free_rx_ep++;
}



/** sja1105_sw_rx_to_host - called by the HAL to put a switch frame to the upper layer
 * \return none
 * \param sja_desc
 * \param p_data
 */
static void sja1105_sw_rx_to_host(const SJA1105_frameDescriptor_t *sja_desc, const u8 *p_data)
{
	struct eth_avb *eth = (struct eth_avb *)sja_desc->txPrivate1;
	struct net_rx_desc *rx_desc = (struct net_rx_desc *)sja_desc->txPrivate2;
	struct logical_port *port;

	if (!is_port_ready(sjadrv)) {
		pool_dma_free(eth->buf_pool, rx_desc);
		goto err_not_ready;
	}

	/* Only for switch frame put back timestamp computed by the HAL/SJA into the descriptor*/
	rx_desc->ts64 = sja_desc->rxTimeStamp << 3;
	rx_desc->ts = (u32)rx_desc->ts64;

	port = bridge_to_logical_port(0, sja_desc->port);

	sjadrv->stats.rx_to_host_sw[sja_desc->port]++;

	/* call frame receive function here - send packet to upper layer */
	switch (sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK) {
		case DESC_FLAG_PTP_FRAME_MASK:
			sjadrv->stats.rx_to_host_sw_ptp[sja_desc->port]++;
			__ptp_rx(port, rx_desc);
			break;

		case DESC_FLAG_SRP_FRAME_MASK:
			sjadrv->stats.rx_to_host_sw_srp[sja_desc->port]++;
			__mrp_rx(port, rx_desc);
			break;

		case DESC_FLAG_META_FRAME_MASK:
			sjadrv->stats.rx_to_host_sw_meta++;
			__sja1105_metaframe_rx(eth, rx_desc);
			break;

		default:
			printk("%s: unknown frame type (0x%x)\n", __func__, (sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK));
			break;
	}

err_not_ready:
	sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)sja_desc);
	sjadrv->stats.num_desc_free_rx++;
	sjadrv->stats.num_desc_free_rx_sw++;
}

static int __sja1105_tx_to_network(struct qos_queue *qos_q, struct avb_tx_desc *desc)
{
	struct ethhdr *ethhdr;
	int rc = 0;

	if (!is_port_ready(sjadrv)) {
		qos_q->dropped++;
		rc = -1;
		goto err;
	}

	if (!(qos_q->flags & QOS_QUEUE_FLAG_ENABLED)) {
		rc = -1;
		qos_q->disabled++;
		qos_q->dropped++;
		goto err;
	}

	ethhdr = (void *)desc + desc->common.offset;
	//pr_info("%s: src %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, ethhdr->h_source[0],ethhdr->h_source[1],ethhdr->h_source[2],ethhdr->h_source[3],ethhdr->h_source[4],ethhdr->h_source[5]);
	//pr_info("%s: dst %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, ethhdr->h_dest[0],ethhdr->h_dest[1],ethhdr->h_dest[2],ethhdr->h_dest[3],ethhdr->h_dest[4],ethhdr->h_dest[5]);
	//pr_info("%s: ethhdr %p desc %p qos_q->queue %p\n", __func__, ethhdr, desc, qos_q->queue);

	if ((qos_q->queue == NULL) || (qos_q->tc == NULL)) {
		qos_q->dropped++;
		rc = -1;
		goto err;
	}

	if (queue_enqueue(qos_q->queue, (unsigned long)desc) < 0) {
		qos_q->full++;
		qos_q->dropped++;
		rc = -1;
		goto err;
	}

	set_bit(qos_q->index, &qos_q->tc->shared_pending_mask);

err:
	return rc;
}


/***************************** Public functions *****************************/

/** sja1105_tx_to_network - called back by the HAL upon transmit processing completion (route management).
 * This function retrieves the private data (tx descriptor) from the previously stored SJA1105 frame descriptor
 * and finalizes packet transmit to the network
 * \return 0 always otherwise the underlaying HAL may try to send the packet again
 * \param sja_desc
 * \param p_data
 */
static u8 sja1105_tx_to_network (const SJA1105_frameDescriptor_t *p_sja_desc, u8 *p_data)
{
	struct eth_avb *eth;
	struct avb_tx_desc *desc;
	u8 frame_type, port;
	int rc = 0;
	struct sja1105_ts_desc *ts_desc = NULL;

	eth = (struct eth_avb *)p_sja_desc->txPrivate1;
	desc = (struct avb_tx_desc *)p_sja_desc->txPrivate2;

	frame_type = (p_sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK);

//	printk("%s: sjadesc(%p) eth(%p) desc(%p) frame_type(%d) port(%d)\n", __func__, p_sja_desc, eth, desc, frame_type, ((desc->common.private >> 8) & 0xFF));

	/* some sanity checks */
	if (!is_port_ready(sjadrv)) {
		pool_dma_free(eth->buf_pool, desc);
		goto err_not_ready;
	}

	/* call frame transmit function here - send packet to network interface through physical host port */
	switch (frame_type) {
		case DESC_FLAG_PTP_FRAME_MASK:
			port = (desc->common.private >> 8) & 0xFF;
			if(p_sja_desc->flags & DESC_FLAG_TAKE_TIME_STAMP_MASK) {
				if ((ts_desc = sja1105_egress_ts_store_index(port, (void*)eth, (void*)desc, p_sja_desc->timeStampIndex)) == NULL)
					printk("%s: no available slot for port %d ts_index %d\n", __func__, port, p_sja_desc->timeStampIndex);
			}

			if (__sja1105_tx_to_network(sjadrv->tx_qos_queue, desc) != 0) {
				if (is_bridge(desc))
					sjadrv->stats.tx_to_network_sw_ptp_err[port]++;
				else
					sjadrv->stats.tx_to_network_ep_ptp_err++;

				if ((p_sja_desc->flags & DESC_FLAG_TAKE_TIME_STAMP_MASK) && (ts_desc))
					sja1105_egress_ts_release_slot(port, ts_desc->slot);

				pool_dma_free(eth->buf_pool, desc);
			} else {
				if (is_bridge(desc))
					sjadrv->stats.tx_to_network_sw_ptp[port]++;
				else
					sjadrv->stats.tx_to_network_ep_ptp++;
			}
			break;

		case DESC_FLAG_SRP_FRAME_MASK:
			port = (desc->common.private >> 8) & 0xFF;
			if (__sja1105_tx_to_network(sjadrv->tx_qos_queue, desc) != 0) {
				if (is_bridge(desc))
					sjadrv->stats.tx_to_network_sw_srp_err[port]++;
				else
					sjadrv->stats.tx_to_network_ep_srp_err++;

				pool_dma_free(eth->buf_pool, desc);
			} else {
				if (is_bridge(desc))
					sjadrv->stats.tx_to_network_sw_srp[port]++;
				else
					sjadrv->stats.tx_to_network_ep_srp++;
			}
			break;

		case DESC_FLAG_META_FRAME_MASK:
			/* should not happens */
			printk("%s: error metaframe cannot be transmitted\n", __func__);
			pool_dma_free(eth->buf_pool, desc);
			break;

		default:
			printk("%s: unknown frame type (0x%x)\n", __func__, frame_type);
			pool_dma_free(eth->buf_pool, desc);
			break;
	}

err_not_ready:
	sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)p_sja_desc);

	return rc;
}


/** sja1105_tx_process - this function delivers pending transmit packet to the HAL
 * \return 0 on success or negative value on failure
 * \param none
 */
static int sja1105_tx_process(void)
{
	SJA1105_frameDescriptor_t *p_ep_sja_desc;
	SJA1105_frameDescriptor_t *p_sw_sja_desc;
	struct avb_tx_desc *tx_desc;
	struct eth_avb *eth;
	u8 *pdata;
	u8 ts_index;
	int i, rc = 0;
	u32 read;

	/* dequeue pending descriptors and  if the HAL cannot handle it the descriptor is kept in the queue for next round */
	if (queue_pending(&sjadrv->tx_queue_sw)) {
		queue_dequeue_init(&sjadrv->tx_queue_sw, &read);
		p_sw_sja_desc = (void *)queue_dequeue_next(&sjadrv->tx_queue_sw, &read);
		if (p_sw_sja_desc) {
			eth = (struct eth_avb *)p_sw_sja_desc->txPrivate1;
			tx_desc = (struct avb_tx_desc *)p_sw_sja_desc->txPrivate2;
			pdata = (u8 *)tx_desc + tx_desc->common.offset;

			//printk("%s: sjadesc(%p) AED-B port %d (%d)\n", __func__, p_sw_sja_desc, ((tx_desc->common.private >> 8) & 0xFF), p_sw_sja_desc->ports);

			if (SJA1105_sendSwitchFrame(p_sw_sja_desc, pdata, &ts_index)) {
				rc = -1;
			} else {
				queue_dequeue_done(&sjadrv->tx_queue_sw, read);

				sjadrv->stats.tx_queue_sw_lvl--;

				for (i = 0; i < NUM_PORT; i++) {
					if ((1<<i) & p_sw_sja_desc->ports) {
						sjadrv->stats.tx_from_host_sw[i]++;
						switch (p_sw_sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK) {
							case DESC_FLAG_PTP_FRAME_MASK:
								sjadrv->stats.tx_from_host_sw_ptp[i]++;
								break;
							case DESC_FLAG_SRP_FRAME_MASK:
								sjadrv->stats.tx_from_host_sw_srp[i]++;
								break;
							default:
								sjadrv->stats.tx_from_host_sw_err[i]++;
								break;
						}
					}
				}
			}
		}
	}

	if (queue_pending(&sjadrv->tx_queue_ep)) {
		queue_dequeue_init(&sjadrv->tx_queue_ep, &read);
		p_ep_sja_desc = (void *)queue_dequeue_next(&sjadrv->tx_queue_ep, &read);
		if (p_ep_sja_desc) {
			eth = (struct eth_avb *)p_ep_sja_desc->txPrivate1;
			tx_desc = (struct avb_tx_desc *)p_ep_sja_desc->txPrivate2;
			pdata = (u8 *)tx_desc + tx_desc->common.offset;

//			printk("%s: sjadesc(%p) AED-E port %d (%d)\n", __func__, p_ep_sja_desc, ((tx_desc->common.private >> 8) & 0xFF), p_ep_sja_desc->ports);

			if (SJA1105_sendEndPointFrame(p_ep_sja_desc, pdata)) {
				rc = -1;
			} else {
				queue_dequeue_done(&sjadrv->tx_queue_ep, read);

				sjadrv->stats.tx_queue_ep_lvl--;
				sjadrv->stats.tx_from_host_ep++;

				switch (p_ep_sja_desc->flags & DESC_FLAG_FRAME_TYPE_MASK) {
					case DESC_FLAG_PTP_FRAME_MASK:
						sjadrv->stats.tx_from_host_ep_ptp++;
						break;
					case DESC_FLAG_SRP_FRAME_MASK:
						sjadrv->stats.tx_from_host_ep_srp++;
						break;
					default:
						sjadrv->stats.tx_from_host_ep_err++;
						break;
				}
			}
		}
	}

	return rc;
}


/** sja1105_tx_from_host - called by the network transmit stack to give a descriptor to the SJA1105 HAL
 * \return 0 on success or negative value on failure
 * \param eth
 * \param tx_desc
 * \param frame_type
 */
int sja1105_tx_from_host(struct logical_port *port, struct avb_tx_desc *tx_desc, u8 frame_type)
{
	SJA1105_frameDescriptor_t *p_sja_desc;

	if (!is_port_ready(sjadrv))
		goto err_not_ready;

	p_sja_desc = sja1105_tx_desc_alloc();
	if (!p_sja_desc)
		goto err_alloc;

//	printk("%s: sjadesc(%p) %s port %d\n", __func__, p_sja_desc, is_bridge(tx_desc)?"AED-B":"AED-E", ((tx_desc->common.private >> 8) & 0xFF));

	if (is_bridge(tx_desc)) {
		sja1105_sw_setup_tx_desc(port, tx_desc, p_sja_desc, frame_type);

		if (sja1105_sw_tx_enqueue(p_sja_desc) < 0)
			goto err_tx;
	} else if (is_endpoint(tx_desc)){
		sja1105_ep_setup_tx_desc(port, tx_desc, p_sja_desc, frame_type);

		if (sja1105_ep_tx_enqueue(p_sja_desc) < 0)
			goto err_tx;
	} else
		goto err_tx;

	wake_up_process(sjadrv->kthread);

	return 0;

err_tx:
	sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)p_sja_desc);
	sjadrv->stats.num_desc_free_tx++;

	/* wake up the transmit thread even if the packet could not be queued.
	so the transmit queue will be processed as soon as possible */
	wake_up_process(sjadrv->kthread);

err_alloc:
err_not_ready:
	return -1;
}


/** sja1105_rx_to_hal - called by the HAL to get a new rx buffer to process
 * \return 1 on success, 0 on failure (i.e. no element dequeued)
 * \param sja_desc
 * \param p_data
 */
u16 sja1105_rx_to_hal(const SJA1105_frameDescriptor_t **sja_desc, const u8 **p_data)
{
	struct net_rx_desc *rx_desc;
	struct eth_avb *eth;
	u16 rc = 0;

	if (queue_pending(&sjadrv->rx_queue)) {
		*sja_desc = (void *)queue_dequeue(&sjadrv->rx_queue);
		sjadrv->stats.rx_dequeue_count++;
		sjadrv->stats.rx_queue_lvl--;
		if (*sja_desc) {
			eth = (struct eth_avb *)(*sja_desc)->txPrivate1;
			rx_desc = (struct net_rx_desc *)(*sja_desc)->txPrivate2;
			*p_data = (u8 *)rx_desc + rx_desc->l2_offset;
			rc = 1;
		}
	}

	return rc;
}


/* free the sja descriptor if it has been passed by the caller (the HAL). Means the HAL
could not process it (eg. internal error, routes exhausted,...) */
u16 sja1105_rx_to_hal_done(void *sja_desc)
{
	void *desc;
	struct eth_avb *eth;

	if (sja_desc) {
		eth = (struct eth_avb *)((SJA1105_frameDescriptor_t *)sja_desc)->txPrivate1;
		desc = (void *)((SJA1105_frameDescriptor_t *)sja_desc)->txPrivate2;
		pool_dma_free(eth->buf_pool, desc);

		sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)sja_desc);
		sjadrv->stats.num_desc_free_rx++;
	}

	return 0;
}


/** sja1105_rx_from_network - called by the networking stack receive function to give a buffer to the SJA1105 HAL
 * \return 0 on success or negative value on failure
 * \param eth
 * \param rx_desc
 * \param frame_type
 */
int sja1105_rx_from_network(struct eth_avb *eth, struct net_rx_desc *rx_desc, u8 frame_type)
{
	SJA1105_frameDescriptor_t *sja_desc;
	int rc = 0;

	if (!is_port_ready(sjadrv)) {
		rc = -1;
		goto err_not_ready;
	}

	sja_desc = sja1105_rx_desc_alloc();
	if (!sja_desc) {
		rc = -1;
		goto err_alloc;
	}

	switch (frame_type) {
		case PTYPE_PTP:
			sjadrv->stats.rx_from_network_ptp++;
			break;
		case PTYPE_MRP:
			sjadrv->stats.rx_from_network_srp++;
			break;
		case PTYPE_META:
			sjadrv->stats.rx_from_network_meta++;
			break;
		default:
			sjadrv->stats.rx_from_network_err++;
			sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)sja_desc);
			sjadrv->stats.num_desc_free_rx++;
			rc = -1;
			goto err_unknown_type;
	}

	sjadrv->stats.rx_from_network++;

	sja1105_setup_rx_desc(eth, rx_desc, sja_desc, frame_type);

	rc = sja1105_rx_enqueue((void*)sja_desc);
	if (rc < 0) {
		sja1105_desc_free(&sjadrv->buf_pool, (unsigned long)sja_desc);
		sjadrv->stats.num_desc_free_rx++;
		rc = -1;
	}

	/* wake up thread even if the packets could not be queued. This will
	gives more chance to process descriptor already waiting in the rx queue */
	wake_up_process(sjadrv->kthread);

err_unknown_type:
err_alloc:
err_not_ready:
	return rc;
}



/** sja1105_kthread - main kernel thread used to give tick to the HAL to perform Rx and Tx tasks
 * \return 0 on success or negative value on failure
 * \param data
 */
#define KTHREAD_SLEEP_TIME_US 1000

int sja1105_kthread(void *data)
{
	int rc;
	ktime_t kt;

	set_current_state(TASK_INTERRUPTIBLE);

	kt = ns_to_ktime(KTHREAD_SLEEP_TIME_US*1000);

	while (1) {
		rc = schedule_hrtimeout(&kt, HRTIMER_MODE_REL);

		if (kthread_should_stop()) {
			printk("kthread_should_stop\n");
			break;
		}

		mutex_lock(&sjadrv->lock);

		if (sjadrv->ready) {
			/* process egress descriptor from host waiting for timestamp */
			sja1105_egress_ts_to_host();

			/* call HAL tick event here */
			if (SJA1105_ethIfTick() != 0)
				sjadrv->stats.num_sja_tick_err++;

			/* send pending packets to the HAL */
			sja1105_tx_process();
		}

		mutex_unlock(&sjadrv->lock);

		set_current_state(TASK_INTERRUPTIBLE);
	}

	return 0;
}


/** sja1105_queues_init -
 * \return none
 * \param none
 */
static void sja1105_queues_init(void)
{
	queue_init(&sjadrv->rx_queue, sja1105_desc_free);
	sjadrv->rx_queue.size += CFG_RX_EXTRA_ENTRIES;
	queue_init(&sjadrv->tx_queue_ep, sja1105_desc_free);
	queue_init(&sjadrv->tx_queue_sw, sja1105_desc_free);
	queue_init(&sjadrv->egress_ts_queue, pool_dma_free_virt);
}

static void sja1105_queues_flush(void)
{
	while (queue_pending(&sjadrv->rx_queue))
		sja1105_rx_done((SJA1105_frameDescriptor_t *)queue_dequeue(&sjadrv->rx_queue));

	sjadrv->stats.rx_queue_lvl = 0;
	sjadrv->stats.rx_queue_full = 0;

	while (queue_pending(&sjadrv->tx_queue_ep))
		sja1105_tx_done((SJA1105_frameDescriptor_t *)queue_dequeue(&sjadrv->tx_queue_ep));

	sjadrv->stats.tx_queue_ep_lvl = 0;
	sjadrv->stats.tx_queue_ep_full = 0;

	while (queue_pending(&sjadrv->tx_queue_sw))
		sja1105_tx_done((SJA1105_frameDescriptor_t *)queue_dequeue(&sjadrv->tx_queue_sw));

	sjadrv->stats.tx_queue_sw_lvl = 0;
	sjadrv->stats.tx_queue_sw_full = 0;


	while (queue_pending(&sjadrv->egress_ts_queue))
		sja1105_egress_ts_queue_flush((struct net_tx_desc *)queue_dequeue(&sjadrv->egress_ts_queue));

	sjadrv->stats.tx_queue_sw_lvl = 0;
	sjadrv->stats.tx_queue_sw_full = 0;
}


/** sja1105_pool_exit -
 * \return none
 * \param none
 */
static void sja1105_pool_exit(void)
{
	pool_exit(&sjadrv->buf_pool);

	avb_free_range(sjadrv->buf_baseaddr, SJA_BUF_POOL_SIZE);
}


/** sja1105_pool_init -
 * \return 0 on success or negative value on failure
 * \param none
 */
static int sja1105_pool_init(void)
{
	int rc;

	sjadrv->buf_baseaddr = avb_alloc_range(SJA_BUF_POOL_SIZE);
	if (!sjadrv->buf_baseaddr) {
		pr_err("%s: avb_alloc_range() failed\n", __func__);
		rc = -ENOMEM;
		goto err_alloc_range;
	}

	if (pool_init(&sjadrv->buf_pool, sjadrv->buf_baseaddr, SJA_BUF_POOL_SIZE, SJA_BUF_ORDER) < 0) {
		pr_err("%s: pool_init() failed\n", __func__);
		rc = -ENOMEM;
		goto err_pool_init;
	}

	printk("%s: pool(%p) base address(%p) size(%lu) buf size (%u)\n", __func__, &sjadrv->buf_pool, sjadrv->buf_baseaddr, SJA_BUF_POOL_SIZE, SJA_BUF_SIZE);

	return 0;

err_pool_init:
	avb_free_range(sjadrv->buf_baseaddr, SJA_BUF_POOL_SIZE);

err_alloc_range:
	return rc;
}


int sja1105_open(struct switch_drv *drv)
{
	return 0;
}

int sja1105_close(struct switch_drv *drv)
{
	SJA1105_flushAllMgmtRoute();

	SJA1105_flushEthItf();

	sja1105_egress_ts_flush_all();

	sja1105_queues_flush();

	return 0;
}

int sja1105_enable(struct switch_drv *drv)
{
	mutex_lock(&drv->lock);

	drv->ready = 1;

	mutex_unlock(&sjadrv->lock);

	return 0;
}

int sja1105_disable(struct switch_drv *drv)
{
	mutex_lock(&drv->lock);

	drv->ready = 0;

	mutex_unlock(&drv->lock);

	return 0;
}

static void sja1105_hal_init(struct switch_drv *drv)
{
	/* setup rx/tx handlers for both sw and ep */
	SJA1105_registerEgressTimeStampHandler(sja1105_egress_ts);
	SJA1105_registerFrameSendCB(sja1105_tx_to_network);
	SJA1105_registerFrameRecvCB(sja1105_rx_to_hal);
	/* set a callback, to free the sja descriptor if it has been passed by the caller (the HAL). Means the HAL
	could not process it (eg. internal error, routes exhausted,...) */
	SJA1105_registerFrameRecvDoneCB(sja1105_rx_to_hal_done);

	/* Register receive callback to the HAL */
	SJA1105_recvSwitchFrameLoop(0, sja1105_sw_rx_to_host);
	SJA1105_recvEndPointFrameLoop(0, sja1105_ep_rx_to_host);
}


/** switch_init -
 * \return 0 on success or negative value on failure
 * \param sja1105
 * \param avb_dentry
 */
int  switch_init(struct switch_drv *drv, struct net_qos *qos, struct dentry *avb_dentry)
{
	printk("%s: sja1105 driver(%p)\n", __func__, drv);

	sjadrv = drv;

	mutex_init(&sjadrv->lock);

	sja1105_egress_ts_init();

	sjadrv->qos = &qos->port[0];
	queue_init(&sjadrv->tx_queue, pool_dma_free_virt);
	sjadrv->tx_qos_queue = qos_queue_connect(sjadrv->qos, PTPV2_DEFAULT_PRIORITY, &sjadrv->tx_queue, 0);
	if (sjadrv->tx_qos_queue == NULL) {
		goto err_qos_init;
	}

	if (sja1105_pool_init() < 0)
		goto err_pool_init;

	sja1105_queues_init();
	switch_debugfs_init(sjadrv, avb_dentry);

	sja1105_hal_init(drv);

	g_host_port = SJA1105_getHostLogicalPort();
	g_host_port_mask = (1 << g_host_port);

	drv->kthread = kthread_run(sja1105_kthread, sjadrv, "sja1105 tick");
	if (IS_ERR(drv->kthread)) {
		pr_err("%s: kthread_run() failed\n", __func__);
		goto err_thread_init;
	}

	return 0;

err_thread_init:
	sja1105_pool_exit();

err_pool_init:
	qos_queue_disconnect(sjadrv->qos, sjadrv->tx_qos_queue);

err_qos_init:
	return -1;
}


/** sja1105_exit -
 * \return none
 * \param sja1105
 */

void switch_exit(struct switch_drv *drv)
{
	kthread_stop(drv->kthread);

	sja1105_disable(drv);

	sja1105_close(drv);

	sja1105_pool_exit();

	qos_queue_disconnect(drv->qos, drv->tx_qos_queue);
}
