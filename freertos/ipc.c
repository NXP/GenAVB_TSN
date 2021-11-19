/*
* Copyright 2018, 2020 NXP
* Copyright 2020 NXP
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
 @brief IPC Service implementation
 @details
*/

#include <string.h>

#include "common/ipc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "common/log.h"

#include "ipc.h"

/*
 * For all IPC types, the event queue is on the reader side (so there are many event queues for IPC_TYPE_MANY_READERS).
 *
 * IPC message pool
 *	IPC messages are fixed size and allocated on the heap
 *	IPC message allocation is always done on the writer side
 *	IPC message freeing is done both on writer and reader side
 *	IPC message "src" field is used to track which IPC slot allocated a message
 *	There is an hard limit of messages allocated per slot
 *
 * IPC_TYPE_SINGLE_READER_WRITER:
 *	Reader slot is always 0.
 *	Writer slot is always 1.
 *	IPC message queue is on reader side.
 *	When reader exits, the message queue is flushed/free.
 *	If reader doesn't exist, writer gets write errors.
 *
 * IPC_TYPE_MANY_READERS
 *	Reader slot is allocated between 1 and IPC_MAX_READER_WRITERS.
 * 	Writer slot is always 0.
 * 	IPC message queue is on reader side.
 * 	When a reader exits, it's message queue is flushed/free.
 * 	Writer can send messages to all readers (indications) or one in particular (responses).
 * 	When sending to all readers, one new message is allocated for each and the original message is copied and finally free.
 * 	When sending to a specific reader, message slot is moved from writer to reader
 *
 * IPC_TYPE_MANY_WRITERS
 * 	Reader slot is always 0.
 *	Writer slot is allocated between 1 and IPC_MAX_READER_WRITERS.
 * 	IPC message queue is on _writer_ side.
 * 	If reader doesn't exist, writer gets write errors.
 * 	When writer exits, message queue is flushed/free.
 */

#define IPC_MAX_READER_WRITERS	4

#define IPC_MAX_PENDING		20

/*
 * To avoid excessive memory fragmentation define ipc memory allocation
 * sizes same as for net_tx/net_rx
 *
 * IPC_DATA_OFFSET includes all ipc_desc metadata fields
 */
#define IPC_DATA_OFFSET		64

#define IPC_BUF_SIZE		DEFAULT_IPC_DATA_SIZE

#define IPC_DATA_SIZE_MAX_LOW	128
#define IPC_BUF_SIZE_LOW	(IPC_DATA_SIZE_MAX_LOW + IPC_DATA_OFFSET)

#define IPC_ALLOC_SIZE(size) ((size) < IPC_DATA_SIZE_MAX_LOW ? IPC_BUF_SIZE_LOW : IPC_BUF_SIZE)

struct ipc_slot {
	uint16_t count;

	int (*callback)(void *data);
	void *callback_data;
	bool callback_enabled;

	QueueHandle_t event_queue_handle;
	void *event_data;

	QueueHandle_t queue_handle;
	StaticQueue_t queue;
	uint8_t queue_buffer[IPC_MAX_PENDING * sizeof(struct ipc_desc *)];

	struct ipc_channel *ipc;
	unsigned int index;
};

struct ipc_dst_map {
	unsigned int dst;
	struct ipc_slot *dst_slot;
};

struct ipc_channel {
	unsigned int type;

	SemaphoreHandle_t mutex;
	StaticSemaphore_t mutex_buffer;

	struct ipc_slot *slot[IPC_MAX_READER_WRITERS + 1];

	struct ipc_dst_map dst_map[IPC_MAX_READER_WRITERS + 1];

	unsigned int last;
};

static struct ipc_channel ipc_channel[IPC_ID_MAX] = {
	[IPC_MEDIA_STACK_AVDECC].type = IPC_MEDIA_STACK_AVDECC_TYPE,
	[IPC_AVDECC_MEDIA_STACK].type = IPC_AVDECC_MEDIA_STACK_TYPE,

	[IPC_CONTROLLER_AVDECC].type = IPC_CONTROLLER_AVDECC_TYPE,
	[IPC_AVDECC_CONTROLLER].type = IPC_AVDECC_CONTROLLER_TYPE,
	[IPC_AVDECC_CONTROLLER_SYNC].type = IPC_AVDECC_CONTROLLER_SYNC_TYPE,

	[IPC_CONTROLLED_AVDECC].type = IPC_CONTROLLED_AVDECC_TYPE,
	[IPC_AVDECC_CONTROLLED].type = IPC_AVDECC_CONTROLLED_TYPE,

	[IPC_MEDIA_STACK_MSRP].type = IPC_MEDIA_STACK_MSRP_TYPE,
	[IPC_MSRP_MEDIA_STACK].type = IPC_MSRP_MEDIA_STACK_TYPE,
	[IPC_MSRP_MEDIA_STACK_SYNC].type = IPC_MSRP_MEDIA_STACK_SYNC_TYPE,

	[IPC_MEDIA_STACK_MVRP].type = IPC_MEDIA_STACK_MVRP_TYPE,
	[IPC_MVRP_MEDIA_STACK].type = IPC_MVRP_MEDIA_STACK_TYPE,
	[IPC_MVRP_MEDIA_STACK_SYNC].type = IPC_MVRP_MEDIA_STACK_SYNC_TYPE,

	[IPC_MEDIA_STACK_CLOCK_DOMAIN].type = IPC_MEDIA_STACK_CLOCK_DOMAIN_TYPE,
	[IPC_CLOCK_DOMAIN_MEDIA_STACK].type = IPC_CLOCK_DOMAIN_MEDIA_STACK_TYPE,
	[IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC].type = IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC_TYPE,

	[IPC_MEDIA_STACK_MAAP].type = IPC_MEDIA_STACK_MAAP_TYPE,
	[IPC_MAAP_MEDIA_STACK].type = IPC_MAAP_MEDIA_STACK_TYPE,
	[IPC_MAAP_MEDIA_STACK_SYNC].type = IPC_MAAP_MEDIA_STACK_SYNC_TYPE,

	[IPC_MEDIA_STACK_GPTP].type = IPC_MEDIA_STACK_GPTP_TYPE,
	[IPC_GPTP_MEDIA_STACK].type = IPC_GPTP_MEDIA_STACK_TYPE,
	[IPC_GPTP_MEDIA_STACK_SYNC].type = IPC_GPTP_MEDIA_STACK_SYNC_TYPE,

	[IPC_MEDIA_STACK_AVTP].type = IPC_MEDIA_STACK_AVTP_TYPE,
	[IPC_AVTP_MEDIA_STACK].type = IPC_AVTP_MEDIA_STACK_TYPE,
	[IPC_AVTP_MEDIA_STACK_SYNC].type = IPC_AVTP_MEDIA_STACK_SYNC_TYPE,

	[IPC_AVDECC_MSRP].type = IPC_AVDECC_MSRP_TYPE,

	[IPC_AVTP_STATS].type = IPC_AVTP_STATS_TYPE,

	[IPC_MEDIA_STACK_MAC_SERVICE].type = IPC_MEDIA_STACK_MAC_SERVICE_TYPE,
	[IPC_MAC_SERVICE_MEDIA_STACK].type = IPC_MAC_SERVICE_MEDIA_STACK_TYPE,
	[IPC_MAC_SERVICE_MEDIA_STACK_SYNC].type = IPC_MAC_SERVICE_MEDIA_STACK_SYNC_TYPE,
};

static void __ipc_free(struct ipc_slot *slot, struct ipc_desc *desc);

static void ipc_flush_queue(struct ipc_slot *slot)
{
	struct ipc_desc *desc;

	while (xQueueReceiveFromISR(slot->queue_handle, &desc, NULL) == pdTRUE)
		__ipc_free(slot, desc);
}

static struct ipc_desc *__ipc_alloc(struct ipc_slot *slot, unsigned int size)
{
	struct ipc_desc *desc;

	if (size > IPC_BUF_SIZE)
		return NULL;

	taskENTER_CRITICAL();

	if (!slot)
		goto err_unlock;

	if (slot->count >= IPC_MAX_PENDING)
		goto err_unlock;

	slot->count++;

	taskEXIT_CRITICAL();

	desc = pvPortMalloc(IPC_ALLOC_SIZE(size));
	if (!desc) {
		taskENTER_CRITICAL();

		slot->count--;

		goto err_unlock;
	}

	os_log(LOG_DEBUG, "%p\n", desc);

	return desc;

err_unlock:
	taskEXIT_CRITICAL();

	return NULL;
}

struct ipc_desc *ipc_alloc(struct ipc_tx const *tx, unsigned int size)
{
	return __ipc_alloc(tx->ipc_slot, size);
}

static void __ipc_free(struct ipc_slot *slot, struct ipc_desc *desc)
{
	taskENTER_CRITICAL();

	if (slot && slot->count)
		slot->count--;

	taskEXIT_CRITICAL();

	vPortFree(desc);
}

void ipc_free(void const *ipc, struct ipc_desc *desc)
{
	__ipc_free(((struct ipc_tx *)ipc)->ipc_slot, desc);
}


static int ipc_is_free_slot(struct ipc_channel *ipc, unsigned int i)
{
	if (ipc->slot[i])
		return 0;

	return 1;
}

static int ipc_is_disabled_slot(struct ipc_slot *slot)
{
	return !slot;
}

static int ipc_find_slot(struct ipc_channel *ipc)
{
	int i;

	for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {
		if (ipc_is_free_slot(ipc, i))
			return i;
	}

	return -1;
}

static void ipc_alloc_slot(struct ipc_channel *ipc, unsigned int index, struct ipc_slot *slot)
{
	ipc->slot[index] = slot;

	slot->ipc = ipc;
	slot->index = index;
}

static void ipc_free_slot(struct ipc_slot *slot)
{
	slot->ipc->slot[slot->index] = NULL;
}

static inline unsigned int ipc_hdr_len(void)
{
	struct ipc_desc *tmp = (struct ipc_desc *)0;

	/* Length of all structure members, up to the union */
	return (unsigned long)&tmp->u;
}

static struct ipc_desc *ipc_copy(struct ipc_slot *slot_dst, struct ipc_slot *slot_src, struct ipc_desc *desc_src)
{
	struct ipc_desc *desc_dst;

	if (ipc_is_disabled_slot(slot_dst))
		goto err;

	desc_dst = __ipc_alloc(slot_dst, desc_src->len);
	if (!desc_dst)
		goto err;

	memcpy(desc_dst, desc_src, ipc_hdr_len() + desc_src->len);

	return desc_dst;

err:
	return NULL;
}

static int ipc_rx_default_callback(void *data)
{
	struct ipc_slot *slot = data;
	struct event e;
	BaseType_t wake;

	e.type = EVENT_TYPE_IPC;
	e.data = slot->event_data;
	if (xQueueSendToBackFromISR(slot->event_queue_handle, &e, &wake) != pdTRUE) {
		/* Can not report error to upper layer, since the message was queued */
	}

	slot->callback_enabled = true;

	if (wake)
		return 1;
	else
		return 0;
}

static int ipc_tx_slot(struct ipc_slot *queue_slot, struct ipc_slot *wake_slot, struct ipc_desc *desc)
{
	int rc = 0;

	if (xQueueSendToBackFromISR(queue_slot->queue_handle, &desc, NULL) != pdTRUE)
		goto err;

	if (wake_slot->callback && wake_slot->callback_enabled) {
		wake_slot->callback_enabled = false;
		rc = wake_slot->callback(wake_slot->callback_data);
	}

	return rc;

err:
	return -1;
}

int ipc_rx_init(struct ipc_rx *rx, 
		ipc_id_t id, void (*func)(struct ipc_rx const *, struct ipc_desc *), unsigned long priv)
{
	struct ipc_channel *ipc;
	struct ipc_slot *slot;
	int slot_i;

	if (id >= IPC_ID_MAX)
		goto err;

	ipc = &ipc_channel[id];

	slot = pvPortMalloc(sizeof(struct ipc_slot));
	if (!slot)
		goto err;

	memset(slot, 0, sizeof(struct ipc_slot));

	if (ipc->type != IPC_TYPE_MANY_WRITERS) {

		slot->queue_handle = xQueueCreateStatic(IPC_MAX_PENDING, sizeof(struct ipc_desc *), slot->queue_buffer, &slot->queue);
		if (!slot->queue_handle)
			goto err_create;
	}

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		slot_i = ipc_find_slot(ipc);
		if (slot_i < 0)
			goto err_unlock;

		break;

	case IPC_TYPE_MANY_WRITERS:
	case IPC_TYPE_SINGLE_READER_WRITER:
		slot_i = 0;

		if (!ipc_is_free_slot(ipc, slot_i))
			goto  err_unlock;

		break;

	default:
		goto err_unlock;
		break;
	}

	ipc_alloc_slot(ipc, slot_i, slot);

	if (priv) {
		slot->callback = ipc_rx_default_callback;
		slot->callback_data = slot;
		slot->callback_enabled = true;
		slot->event_queue_handle = (void *)priv;
		slot->event_data = rx;
	}

	rx->ipc_slot = slot;
	rx->func = func;
	rx->priv = priv;

	xSemaphoreGive(ipc->mutex);

	os_log(LOG_INIT, "ipc(%p, %p) success\n", ipc, slot);

	return 0;

err_unlock:
	xSemaphoreGive(ipc->mutex);

err_create:
	vPortFree(slot);

err:
	return -1;
}

int ipc_rx_init_no_notify(struct ipc_rx *rx, ipc_id_t id)
{
	return ipc_rx_init(rx, id, NULL, (unsigned long)NULL);
}

void ipc_rx_exit(struct ipc_rx *rx)
{
	struct ipc_slot *slot = rx->ipc_slot;
	struct ipc_channel *ipc;

	if (!slot)
		goto out;

	ipc = slot->ipc;

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
	case IPC_TYPE_SINGLE_READER_WRITER:
	case IPC_TYPE_MANY_WRITERS:
		break;

	default:
		goto out_unlock;
		break;
	}

	ipc_free_slot(slot);

	rx->ipc_slot = NULL;
	rx->func = NULL;
	rx->priv = 0;

	xSemaphoreGive(ipc->mutex);

	if (ipc->type != IPC_TYPE_MANY_WRITERS)
		ipc_flush_queue(slot);

	vPortFree(slot);

	os_log(LOG_INIT, "ipc(%p, %p) exit\n", ipc, slot);

out_unlock:
	xSemaphoreGive(ipc->mutex);

out:
	return;
}

int ipc_rx_set_callback(struct ipc_rx *rx, int (*callback)(void *), void *data)
{
	struct ipc_slot *slot = rx->ipc_slot;
	struct ipc_channel *ipc;

	if (!slot)
		goto err;

	ipc = slot->ipc;

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	slot->callback = callback;
	slot->callback_data = data;
	if (slot->callback)
		slot->callback_enabled = true;
	else
		slot->callback_enabled = false;

	xSemaphoreGive(ipc->mutex);

	return 0;

err:
	return -1;
}

static bool ipc_slot_has_pending(struct ipc_slot *slot)
{
	struct event e;
	
	return (xQueuePeek(slot->queue_handle, &e, 0) == pdTRUE);
}

int ipc_rx_enable_callback(struct ipc_rx *rx)
{
	struct ipc_slot *slot = rx->ipc_slot;
	struct ipc_channel *ipc;
	int rc = 0;

	if (!slot) {
		rc = -1;
		goto err;
	}

	ipc = slot->ipc;

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	if (!slot->callback) {
		rc = -1;
		goto err_unlock;
	}

	slot->callback_enabled = true;

	if (ipc_slot_has_pending(slot)) {
		slot->callback_enabled = false;
		slot->callback(slot->callback_data);
	}

err_unlock:
	xSemaphoreGive(ipc->mutex);
err:
	return rc;
}

int ipc_tx_init(struct ipc_tx *tx, ipc_id_t id)
{
	struct ipc_channel *ipc;
	struct ipc_slot *slot;
	int slot_i;

	if (id >= IPC_ID_MAX)
		goto err;

	ipc = &ipc_channel[id];

	slot = pvPortMalloc(sizeof(struct ipc_slot));
	if (!slot)
		goto err;

	memset(slot, 0, sizeof(struct ipc_slot));

	if (ipc->type == IPC_TYPE_MANY_WRITERS) {

		slot->queue_handle = xQueueCreateStatic(IPC_MAX_PENDING, sizeof(struct ipc_desc *), slot->queue_buffer, &slot->queue);
		if (!slot->queue_handle)
			goto err_create;
	}

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		slot_i = 0;

		if (!ipc_is_free_slot(ipc, slot_i))
			goto  err_unlock;

		break;

	case IPC_TYPE_SINGLE_READER_WRITER:
		slot_i = 1;

		if (!ipc_is_free_slot(ipc, slot_i))
			goto  err_unlock;

		break;

	case IPC_TYPE_MANY_WRITERS:
		slot_i = ipc_find_slot(ipc);
		if (slot_i < 0)
			goto err_unlock;

		break;

	default:
		goto err_unlock;
		break;
	}

	ipc_alloc_slot(ipc, slot_i, slot);

	tx->ipc_slot = slot;

	xSemaphoreGive(ipc->mutex);

	os_log(LOG_INIT, "ipc(%p, %p) success\n", ipc, slot);

	return 0;

err_unlock:
	xSemaphoreGive(ipc->mutex);

err_create:
	vPortFree(slot);

err:
	return -1;
}

void ipc_tx_exit(struct ipc_tx *tx)
{
	struct ipc_slot *slot = tx->ipc_slot;
	struct ipc_channel *ipc;

	if (!slot)
		goto out;

	ipc = slot->ipc;

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
	case IPC_TYPE_SINGLE_READER_WRITER:
	case IPC_TYPE_MANY_WRITERS:
		break;

	default:
		goto out_unlock;
		break;
	}

	ipc_free_slot(slot);

	tx->ipc_slot = NULL;

	xSemaphoreGive(ipc->mutex);

	if (ipc->type == IPC_TYPE_MANY_WRITERS)
		ipc_flush_queue(slot);

	vPortFree(slot);

	os_log(LOG_INIT, "ipc(%p, %p) exit\n", ipc, slot);

	return;

out_unlock:
	xSemaphoreGive(ipc->mutex);

out:
	return;
}


int ipc_tx(struct ipc_tx const *tx, struct ipc_desc *desc)
{
	struct ipc_slot *slot = tx->ipc_slot;
	struct ipc_channel *ipc;
	struct ipc_slot *rx_slot;
	struct ipc_desc *new_desc;
	unsigned int wakeup = 0;
	int rc;
	int i;

	if (!slot)
		goto err;

	ipc = slot->ipc;

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		if (desc->dst == IPC_DST_ALL) {

			for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {
				rx_slot = ipc->slot[i];

				new_desc = ipc_copy(rx_slot, slot, desc);
				if (!new_desc)
					continue;

				rc = ipc_tx_slot(rx_slot, rx_slot, new_desc);
				if (rc < 0)
					__ipc_free(rx_slot, new_desc);
				else
					wakeup |= rc;
			}

			__ipc_free(slot, desc);
		} else {
			unsigned int dst_index = desc->dst & 0xff;

			/* send message to specific reader */
			if (!dst_index || (dst_index > IPC_MAX_READER_WRITERS))
				goto err_unlock;

			if (ipc->dst_map[dst_index].dst != desc->dst)
				goto err_unlock;

			rx_slot = ipc->dst_map[dst_index].dst_slot;

			new_desc = ipc_copy(rx_slot, slot, desc);
			if (!new_desc)
				goto err_unlock;

			rc = ipc_tx_slot(rx_slot, rx_slot, new_desc);
			if (rc < 0) {
				__ipc_free(rx_slot, new_desc);
				goto err_unlock;
			}

			wakeup |= rc;

			__ipc_free(slot, desc);
		}

		break;

	case IPC_TYPE_SINGLE_READER_WRITER:
		rx_slot = ipc->slot[0];

		new_desc = ipc_copy(rx_slot, slot, desc);
		if (!new_desc)
			goto err_unlock;

		rc = ipc_tx_slot(rx_slot, rx_slot, new_desc);
		if (rc < 0) {
			__ipc_free(rx_slot, new_desc);
			goto err_unlock;
		}

		wakeup |= rc;

		__ipc_free(slot, desc);

		break;

	case IPC_TYPE_MANY_WRITERS:

		if (ipc_is_disabled_slot(ipc->slot[0]))
			goto err_unlock;

		rc = ipc_tx_slot(slot, ipc->slot[0], desc);
		if (rc < 0)
			goto err_unlock;

		wakeup |= rc;

		break;

	default:
		goto err_unlock;
		break;
	}

	xSemaphoreGive(ipc->mutex);

	if (wakeup)
		taskYIELD();

	return 0;

err_unlock:
	xSemaphoreGive(ipc->mutex);

err:
	return -1;
}


static unsigned int ipc_slot_address(struct ipc_slot *slot)
{
	struct ipc_channel *ipc = slot->ipc;

	return (slot->index | ((ipc - &ipc_channel[0]) << 8));
}

struct ipc_desc * __ipc_rx(struct ipc_rx const *rx)
{
	struct ipc_slot *slot = rx->ipc_slot;
	struct ipc_channel *ipc;
	struct ipc_slot *tx_slot;
	struct ipc_desc *desc;
	int i;

	ipc = slot->ipc;

	xSemaphoreTake(ipc->mutex, portMAX_DELAY);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
	case IPC_TYPE_SINGLE_READER_WRITER:

		if (xQueueReceiveFromISR(slot->queue_handle, &desc, NULL) != pdTRUE)
			goto err_unlock;

		desc->src = 0;

		break;

	case IPC_TYPE_MANY_WRITERS:
		for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {
			struct ipc_desc *tx_desc;

			ipc->last++;
			if (ipc->last > IPC_MAX_READER_WRITERS)
				ipc->last = 1;

			tx_slot = ipc->slot[ipc->last];

			if (ipc_is_disabled_slot(tx_slot))
				continue;

			if (xQueueReceiveFromISR(tx_slot->queue_handle, &tx_desc, NULL) != pdTRUE)
				continue;

			desc = ipc_copy(slot, tx_slot, tx_desc);
			if (!desc) {
				__ipc_free(tx_slot, tx_desc);

				continue;
			}

			desc->src = ipc_slot_address(tx_slot);

			__ipc_free(tx_slot, tx_desc);

			goto out_unlock;
		}

		goto err_unlock;

		break;

	default:
		goto err_unlock;
		break;
	}

out_unlock:
	xSemaphoreGive(ipc->mutex);

	return desc;

err_unlock:
	xSemaphoreGive(ipc->mutex);

	return NULL;
}

void ipc_rx(struct ipc_rx const *rx)
{
	struct ipc_desc *desc;

	while (1) {
		desc = __ipc_rx(rx);
		if (!desc)
			break;

		rx->func(rx, desc);
	}
}

int ipc_tx_connect(struct ipc_tx *tx, struct ipc_rx *rx)
{
	struct ipc_slot *slot = tx->ipc_slot;
	struct ipc_channel *tx_ipc;
	struct ipc_channel *rx_ipc;
	struct ipc_slot *rx_slot;

	if (!slot)
		goto err;

	tx_ipc = slot->ipc;

	if (tx_ipc->type != IPC_TYPE_MANY_WRITERS)
		goto err;

	rx_slot = rx->ipc_slot;
	if (!rx_slot)
		goto err;

	rx_ipc = rx_slot->ipc;

	if (rx_ipc->type != IPC_TYPE_MANY_READERS)
		goto err;

	xSemaphoreTake(rx_ipc->mutex, portMAX_DELAY);

	rx_ipc->dst_map[slot->index].dst_slot = rx_slot;
	rx_ipc->dst_map[slot->index].dst = ipc_slot_address(slot);

	xSemaphoreGive(rx_ipc->mutex);

	return 0;

err:
	return -1;
}

__init static void ipc_channel_init(struct ipc_channel *ipc)
{
	ipc->mutex = xSemaphoreCreateMutexStatic(&ipc->mutex_buffer);
	ipc->last = 1;
}

__init int ipc_init(void)
{
	int i;

	for (i = 0; i < IPC_ID_MAX; i++)
		ipc_channel_init(&ipc_channel[i]);

	return 0;
}

__exit void ipc_exit(void)
{

}
