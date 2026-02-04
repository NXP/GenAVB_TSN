/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC common code
 @details Handles all AVDECC entities
*/

#include "config.h"
#include <string.h>

#include "os/stdlib.h"
#include "os/string.h"

#include "common/log.h"
#include "common/avtp.h"
#include "common/ether.h"
#include "common/srp.h"

#include "genavb/aem.h"
#include "genavb/qos.h"
#include "genavb/managed_objects.h"

#include "avdecc.h"
#include "avdecc_ieee.h"

struct __attribute__ ((packed)) genavb_avdecc_managed_objects_default_ds_request {
	avb_u16 node_id;
	avb_u16 node_len;
	struct {
		avb_u16 instance_key_id;
		avb_u16 instance_key_len;
		avb_u16 instance_key_value;
		struct {
			avb_u16 container_id;
			avb_u16 container_len;
			struct {
				avb_u16 clk_id_leaf_id;
				avb_u16 clk_id_leaf_len;
				avb_u16 priority1_leaf_id;
				avb_u16 priority1_leaf_len;
				avb_u16 clk_class_leaf_id;
				avb_u16 clk_class_leaf_len;
				avb_u16 offset_scaled_log_variance_leaf_id;
				avb_u16 offset_scaled_log_variance_leaf_len;
				avb_u16 clk_accuracy_leaf_id;
				avb_u16 clk_accuracy_leaf_len;
				avb_u16 priority2_leaf_id;
				avb_u16 priority2_leaf_len;
			} default_ds;
		} entry;
	} instance;
};

struct __attribute__ ((packed)) genavb_avdecc_managed_objects_default_ds_response {
	avb_u16 node_id;
	avb_u16 node_len;
	avb_u16 node_status;
	struct {
		avb_u16 instance_key_id;
		avb_u16 instance_key_len;
		avb_u16 instance_key_status;
		avb_u16 instance_key_value;
		struct {
			avb_u16 container_id;
			avb_u16 container_len;
			avb_u16 container_status;
			struct __attribute__ ((packed)) {
				avb_u16 clk_id_leaf_id;
				avb_u16 clk_id_leaf_len;
				avb_u16 clk_id_leaf_status;
				avb_u64 clk_id_leaf_value;
				avb_u16 priority1_leaf_id;
				avb_u16 priority1_leaf_len;
				avb_u16 priority1_leaf_status;
				avb_u8 priority1_leaf_value;
				avb_u16 clk_class_leaf_id;
				avb_u16 clk_class_leaf_len;
				avb_u16 clk_class_leaf_status;
				avb_u8 clk_class_leaf_value;
				avb_u16 offset_scaled_log_variance_leaf_id;
				avb_u16 offset_scaled_log_variance_leaf_len;
				avb_u16 offset_scaled_log_variance_leaf_status;
				avb_u16 offset_scaled_log_variance_leaf_value;
				avb_u16 clk_accuracy_leaf_id;
				avb_u16 clk_accuracy_leaf_len;
				avb_u16 clk_accuracy_leaf_status;
				avb_u8 clk_accuracy_leaf_value;
				avb_u16 priority2_leaf_id;
				avb_u16 priority2_leaf_len;
				avb_u16 priority2_leaf_status;
				avb_u8 priority2_leaf_value;
			} default_ds;
		} entry;
	} instance;
};

static const char *talker_stream_status2string(genavb_talker_stream_status_t status)
{
	switch (status) {
	case2str(NO_LISTENER);
	case2str(FAILED_LISTENER);
	case2str(ACTIVE_AND_FAILED_LISTENERS);
	case2str(ACTIVE_LISTENER);
	default:
		return (char *) "Unknown talker stream status";
	}
}

static const char *talker_stream_declaration_type2string(genavb_talker_stream_declaration_type_t talker_declaration_type)
{
	switch (talker_declaration_type) {
	case2str(NO_TALKER_DECLARATION);
	case2str(TALKER_ADVERTISE);
	case2str(TALKER_FAILED);
	default:
		return (char *) "Unknown talker declaration type";
	}
}

static const char *listener_stream_status2string(genavb_listener_stream_status_t status)
{
	switch (status) {
	case2str(NO_TALKER);
	case2str(ACTIVE);
	case2str(FAILED);
	default:
		return (char *) "Unknown listener stream status";
	}
}

static const char *listener_stream_declaration_type2string(genavb_listener_stream_declaration_type_t listener_declaration_type)
{
	switch (listener_declaration_type) {
	case2str(NO_LISTENER_DECLARATION);
	case2str(LISTENER_READY);
	case2str(LISTENER_READY_FAILED);
	case2str(LISTENER_FAILED);
	default:
		return (char *) "Unknown listener declaration type";
	}
}

/** gPTP IPC, sends a GENAVB_MSG_MANAGED_GET message to get the default_parameter_data_set container
 * \return none
 * \param port
 */
static void avdecc_ipc_managed_get_default_ds(struct avdecc_port *port)
{
	struct ipc_desc *desc;
	struct genavb_avdecc_managed_objects_default_ds_request *request;
	int rc;

	desc = ipc_alloc(&port->ipc_tx_gptp, sizeof(struct genavb_msg_managed_get));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MANAGED_GET;
		desc->len = sizeof(struct genavb_msg_managed_get);
		desc->flags = 0;

		os_memset(&desc->u.managed_get, 0, sizeof(struct genavb_msg_managed_get));

		request = (struct genavb_avdecc_managed_objects_default_ds_request *)&desc->u.managed_get;

		request->node_id = GPTP_NODE_INSTANCE_LIST;
		request->node_len = sizeof(request->instance);

		request->instance.instance_key_id = GPTP_INSTANCE_INSTANCE_INDEX;
		request->instance.instance_key_len = sizeof(request->instance.instance_key_value);
		request->instance.instance_key_value = CFG_DEFAULT_GPTP_DOMAIN;

		request->instance.entry.container_id = GPTP_INSTANCE_DEFAULT_DS;
		request->instance.entry.container_len = sizeof(request->instance.entry.default_ds);

		request->instance.entry.default_ds.clk_id_leaf_id = GPTP_DEFAULT_DS_CLOCK_IDENTITY;
		request->instance.entry.default_ds.priority1_leaf_id = GPTP_DEFAULT_DS_PRIORITY1;
		request->instance.entry.default_ds.clk_class_leaf_id = GPTP_DEFAULT_DS_CLOCK_CLASS;
		request->instance.entry.default_ds.offset_scaled_log_variance_leaf_id = GPTP_DEFAULT_DS_OFFSET_SCALED_LOG_VARIANCE;
		request->instance.entry.default_ds.clk_accuracy_leaf_id = GPTP_DEFAULT_DS_CLOCK_ACCURACY;
		request->instance.entry.default_ds.priority2_leaf_id = GPTP_DEFAULT_DS_PRIORITY2;

		rc = ipc_tx(&port->ipc_tx_gptp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&port->ipc_tx_gptp, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

/** gPTP IPC response parser, parse a GENAVB_MSG_MANAGED_GET message that contains the default_parameter_data_set data
 * \return none
 * \param entity
 * \param port_id
 * \param managed_object_resp, the response of a managed_get request on default_parameter_data_set container
 */
static void avdecc_ipc_managed_parse_default_ds_response(struct entity *entity, unsigned int port_id, struct genavb_avdecc_managed_objects_default_ds_response *response)
{
	struct avb_interface_descriptor *avb_itf;
	unsigned int node_len_expected, leaf_len_expected;

	if (response->node_status != 0) {
		os_log(LOG_ERR, "entity(%p): Managed object get status %u failed\n", entity, response->node_status);
		return;
	}

	node_len_expected = sizeof(response->node_status) + sizeof(response->instance);
	if (response->node_len != node_len_expected) {
		os_log(LOG_ERR, "entity(%p): Managed object get length failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->node_status, node_len_expected, response->node_len);
		goto exit;
	}

	avb_itf = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_itf) {
		os_log(LOG_ERR, "entity(%p): Unsupported avb interface (%u)\n", entity, port_id);
		goto exit;
	}

	leaf_len_expected = sizeof(response->instance.entry.default_ds.clk_id_leaf_status) + sizeof(response->instance.entry.default_ds.clk_id_leaf_value);
	if (response->instance.entry.default_ds.clk_id_leaf_status == 0 && response->instance.entry.default_ds.clk_id_leaf_len == leaf_len_expected) {
		avb_itf->clock_identity = get_64(&response->instance.entry.default_ds.clk_id_leaf_value);

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->instance.entry.default_ds.clk_id_leaf_status, leaf_len_expected, response->instance.entry.default_ds.clk_id_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(response->instance.entry.default_ds.priority1_leaf_status) + sizeof(response->instance.entry.default_ds.priority1_leaf_value);
	if (response->instance.entry.default_ds.priority1_leaf_status == 0 && response->instance.entry.default_ds.priority1_leaf_len == leaf_len_expected) {
		avb_itf->priority1 = response->instance.entry.default_ds.priority1_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get priority1 failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->instance.entry.default_ds.priority1_leaf_status, leaf_len_expected, response->instance.entry.default_ds.priority1_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(response->instance.entry.default_ds.clk_class_leaf_status) + sizeof(response->instance.entry.default_ds.clk_class_leaf_value);
	if (response->instance.entry.default_ds.clk_class_leaf_status == 0 && response->instance.entry.default_ds.clk_class_leaf_len == leaf_len_expected) {
		avb_itf->clock_class = response->instance.entry.default_ds.clk_class_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock class failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->instance.entry.default_ds.clk_class_leaf_status, leaf_len_expected, response->instance.entry.default_ds.clk_class_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(response->instance.entry.default_ds.offset_scaled_log_variance_leaf_status) + sizeof(response->instance.entry.default_ds.offset_scaled_log_variance_leaf_value);
	if (response->instance.entry.default_ds.offset_scaled_log_variance_leaf_status == 0 && response->instance.entry.default_ds.offset_scaled_log_variance_leaf_len == leaf_len_expected) {
		avb_itf->offset_scaled_log_variance = response->instance.entry.default_ds.offset_scaled_log_variance_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get offset scaled log variance failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->instance.entry.default_ds.offset_scaled_log_variance_leaf_status, leaf_len_expected, response->instance.entry.default_ds.offset_scaled_log_variance_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(response->instance.entry.default_ds.clk_accuracy_leaf_status) + sizeof(response->instance.entry.default_ds.clk_accuracy_leaf_value);
	if (response->instance.entry.default_ds.clk_accuracy_leaf_status == 0 && response->instance.entry.default_ds.clk_accuracy_leaf_len == leaf_len_expected) {
		avb_itf->clock_accuracy = response->instance.entry.default_ds.clk_accuracy_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock accuracy failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->instance.entry.default_ds.clk_accuracy_leaf_status, leaf_len_expected, response->instance.entry.default_ds.clk_accuracy_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(response->instance.entry.default_ds.priority2_leaf_status) + sizeof(response->instance.entry.default_ds.priority2_leaf_value);
	if (response->instance.entry.default_ds.priority2_leaf_status == 0 && response->instance.entry.default_ds.priority2_leaf_len == leaf_len_expected) {
		avb_itf->priority2 = response->instance.entry.default_ds.priority2_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get priority2 failed: status (%u), len : expected(%u) got(%u)\n",
				entity, response->instance.entry.default_ds.priority2_leaf_status, leaf_len_expected, response->instance.entry.default_ds.priority2_leaf_len);
		goto exit;
	}

exit:
	return;
}

/** gPTP IPC response, receive handler for GENAVB_MSG_MANAGED_GET messages
 * \return none
 * \param entity
 * \param port_id
 * \param managed_get_response, the response of a managed_get request
 */
static void avdecc_ipc_managed_get_response(struct entity *entity, unsigned int port_id, struct genavb_msg_managed_get_response *managed_get_response)
{
	struct genavb_avdecc_managed_objects_default_ds_response *managed_object_resp = (struct genavb_avdecc_managed_objects_default_ds_response *)managed_get_response;

	switch (managed_object_resp->instance.entry.container_id) {
	case GPTP_INSTANCE_DEFAULT_DS:
		avdecc_ipc_managed_parse_default_ds_response(entity, port_id, managed_object_resp);
		break;

	default:
		os_log(LOG_ERR, "Unsupported managed container id %u\n", managed_object_resp->instance.entry.container_id);
		break;
	}
}

void avdecc_inflight_restart(struct inflight_ctx *entry)
{
	timer_stop(&entry->timeout);
	timer_start(&entry->timeout, entry->timeout_ms);
}

/** Inflight timeout callback.
 * \return	none
 * \param	data	timer private data
 */
void avdecc_inflight_timeout(void *data)
{
	struct inflight_ctx *entry = data;
	int rc;

	rc = entry->cb(entry);

	os_log(LOG_DEBUG, "Inflight (%p) timeout, cb returned %d\n", entry, rc);

	if (rc == AVDECC_INFLIGHT_TIMER_STOP) {
		list_del(&entry->list);
		list_add(&entry->entity->free_inflight, &entry->list);
		timer_stop(&entry->timeout);
	}
	else if (rc == AVDECC_INFLIGHT_TIMER_RESTART)
		avdecc_inflight_restart(entry);
	else
		os_log(LOG_CRIT, "callback returned invalid return code...\n");
}

/** Gets a free inflight context.
 * \return	pointer to a free inflight context, NULL if not available
 * \param	entity		pointer to the entity context to get an inflight entry from.
 */
struct inflight_ctx *avdecc_inflight_get(struct entity *entity)
{
	struct inflight_ctx *entry = NULL;
	struct list_head *list_entry;

	if (!list_empty(&entity->free_inflight)) {
		list_entry = list_first(&entity->free_inflight);
		entry = container_of(list_entry, struct inflight_ctx, list);
		list_del(list_entry);
	}
	else
		os_log(LOG_ERR, "entity(%p) No more inflight entries available\n", entity);

	return entry;
}

/** Enqueues the provided inflight context and starts its associated timeout.
 * It is used to save a command PDU and to manage the retransmission.
 * \return	0
 * \param	inflight_head	pointer to the list head
 * \param	entry		pointer to the inflight context
 * \param	timeout		timeout in ms
 */
int avdecc_inflight_start(struct list_head *inflight_head, struct inflight_ctx *entry, unsigned int timeout)
{

	entry->list_head = inflight_head;
	list_add(inflight_head, &entry->list);
	entry->timeout_ms = timeout;
	timer_start(&entry->timeout, timeout);

	os_log(LOG_DEBUG, "Started inflight (%p) with sequence id %d, timeout %d ms\n", entry, entry->data.sequence_id, timeout);

	return 0;
}


/**
 * Puts an inflight entry back to the free list.
 * The entry must not be in use: must be called after calling ::avdecc_inflight_get but before calling ::avdecc_inflight_start.
 * \param 	entity	Entity context the entry should be returned to.
 * \param	entry	Inflight entry to return to the free inflight list.
 */
void avdecc_inflight_abort(struct entity *entity, struct inflight_ctx *entry)
{
	list_add(&entity->free_inflight, &entry->list);
}



struct inflight_ctx *avdecc_inflight_find(struct list_head *inflight_head, u16 sequence_id)
{
	struct list_head *list_entry;
	struct inflight_ctx *entry;

	list_entry = list_first(inflight_head);

	while (list_entry != inflight_head) {
		entry = container_of(list_entry, struct inflight_ctx, list);

		if (sequence_id == entry->data.sequence_id)
			return entry;

		list_entry = list_next(list_entry);
	}

	return NULL;
}

/**
 * Checks the entity lock to see if the controller is authorized or not.
 * \return false if the controller is authorized (either entity is not locked or
 * it's the controller owning the lock), or true if entity is locked by another controller
 * \param entity	pointer to the entity structure to check
 * \param controller_id	controller entity ID trying to alter the entity state.
 */
bool avdecc_entity_is_locked(struct entity *entity, u64 controller_id)
{
       struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
                                                                                     AEM_DESC_TYPE_ENTITY, 0, NULL);
       if ((entity_dynamic->lock_status == LOCKED)
               && (entity_dynamic->locking_controller_id != controller_id)) {

               return true;
       }

       return false;
}

/**
 * Checks the entity acquire status to see if the controller is authorized or not.
 * \return false if the controller is authorized (either entity is not acquired or
 * it's the controller owning the lock), or true if entity is locked by another controller
 * \param entity	pointer to the entity structure to check
 * \param controller_id	controller entity ID trying to alter the entity state.
 */
bool avdecc_entity_is_acquired(struct entity *entity, u64 controller_id)
{
       struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
                                                                                     AEM_DESC_TYPE_ENTITY, 0, NULL);
       if ((entity_dynamic->acquire_status == ACQUIRED)
               && (entity_dynamic->acquiring_controller_id != controller_id)) {

               return true;
       }

       return false;
}

/**
 * Checks if a stream input has a redundant stream already connected/bound
 * \return true if the stream input has a connected redundant stream, false otherwise
 * \param entity			pointer to the entity structure to check.
 * \param listener_unique_id		stream input unique id and descriptor index.
 */
bool avdecc_stream_input_has_redundant_connected(struct entity *entity, u16 listener_unique_id)
{
	struct stream_descriptor *stream_input_desc;
	bool rc = false;
	int i;

	stream_input_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_desc)
		goto exit;

	for (i = 0; i < ntohs(stream_input_desc->number_of_redundant_streams); i++) {
		if (acmp_stream_input_has_valid_stream_id(entity, ntohs(stream_input_desc->redundant_streams[i]))) {
			rc = true;
			goto exit;
		}
	}

exit:
	return rc;
}

/**
 * Checks if a stream input (or one of its redundant streams) is the clock source of its related clock domain.
 * \return true if the stream input or one of its redundant streams is a clock source of its clock domain, false otherwise
 * \param entity			pointer to the entity structure to check.
 * \param listener_unique_id		stream input unique id and descriptor index.
 */
bool avdecc_stream_input_is_clock_source(struct entity *entity, u16 listener_unique_id)
{
	struct clock_domain_descriptor *clock_domain_desc;
	struct clock_source_descriptor *clock_source_desc;
	struct stream_descriptor *stream_input_desc;
	bool rc = false;
	int i;

	stream_input_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_desc)
		goto exit;

	clock_domain_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, ntohs(stream_input_desc->clock_domain_index), NULL);
	if (!clock_domain_desc)
		goto exit;

	clock_source_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_SOURCE, ntohs(clock_domain_desc->clock_source_index), NULL);
	if (!clock_source_desc)
		goto exit;

	if (ntohs(clock_source_desc->clock_source_type) == AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM) {

		u16 stream_source_index = ntohs(clock_source_desc->clock_source_location_index);

		/* Check if the current stream is the clock source */
		if (stream_source_index == listener_unique_id) {
			rc = true;
			goto exit;
		}

		/* Check if one of its redundant streams is the clock source */
		if (STREAM_HAS_REDUNDANT_STREAMS(stream_input_desc)) {
			for (i = 0; i < ntohs(stream_input_desc->number_of_redundant_streams); i++) {
				if (ntohs(stream_input_desc->redundant_streams[i]) == stream_source_index) {
					rc = true;
					goto exit;
				}
			}
		}
	}

exit:
	return rc;
}

/* Checks if a stream's unique id appears in the array of redundant streams of a given stream_descriptor.
 * \return true if the unique id refers to a stream that is redundant to the given stream_descriptor, false otherwise
 * \param stream_desc			pointer to the given stream_descriptor.
 * \param redundant_stream_unique_id	stream unique id to check. (in network order)
 */
bool avdecc_is_redundant_stream_of(struct stream_descriptor *stream_desc, u16 redundant_stream_unique_id)
{
	unsigned int num_of_redundant_streams;
	bool rc = false;
	int i;

	if (!stream_desc)
		goto out;

	num_of_redundant_streams = ntohs(stream_desc->number_of_redundant_streams);
	for (i = 0; i < num_of_redundant_streams; i++) {
		if (stream_desc->redundant_streams[i] == redundant_stream_unique_id) {
			rc = true;
			goto out;
		}
	}

out:
	return rc;
}

/**
 * Find an inflight entry matching the provided sequence ID and controller entity ID.
 * \return pointer to found inflight entry or NULL.
 * \param inflight_head	Inflight list to search.
 * \param sequence_id	sequence ID to match.
 * \param controller_id	controller entity ID to match.
 */
struct inflight_ctx *aem_inflight_find_controller(struct list_head *inflight_head, u16 sequence_id, u64 controller_id)
{
	struct list_head *list_entry;
	struct inflight_ctx *entry;

	list_entry = list_first(inflight_head);

	while (list_entry != inflight_head) {
		entry = container_of(list_entry, struct inflight_ctx, list);

		if ((sequence_id == entry->data.sequence_id) && (controller_id == entry->data.pdu.aem.controller_entity_id))
			return entry;

		list_entry = list_next(list_entry);
	}

	return NULL;
}


void avdecc_inflight_remove(struct entity *entity, struct inflight_ctx *entry)
{
	if (entry) {
		timer_stop(&entry->timeout);
		list_del(&entry->list);
		list_add(&entity->free_inflight, &entry->list);
		os_log(LOG_DEBUG, "Removed inflight (%p)\n", entry);
	}
}

/**
 * Removes a PDU from the inflight list, based on the PDU sequence ID.
 * \return	* -1 if no inflight PDU matched the sequence ID.
 * 			* 0 if a PDU was found.
 * \param	entity			Pointer to entity context.
 * \param 	inflight_head	Inflight list to search/update.
 * \param	sequence_id		Sequence ID to be searched and removed from the list.
 * \param	orig_seq_id		On successful return, will contain the sequence ID of the original PDU that triggered the command.
 * \param 	priv0			On successful return, will contain the inflight first entry private data.
 * \param 	priv1			On successful return, will contain the inflight second entry private data.
  */
int avdecc_inflight_cancel(struct entity *entity, struct list_head *inflight_head, u16 sequence_id, u16 *orig_seq_id, void **priv0, void **priv1)
{
	struct inflight_ctx *entry;

	entry = avdecc_inflight_find(inflight_head, sequence_id);
	if (entry) {
		if (orig_seq_id)
			*orig_seq_id = entry->data.orig_seq_id;
		if (priv0)
			*priv0 = (void *)entry->data.priv[0];
		if (priv1)
			*priv1 = (void *)entry->data.priv[1];

		avdecc_inflight_remove(entity, entry);
		os_log(LOG_DEBUG, "Cancelled inflight (%p)\n", entry);

		return 0;
	}

	return -1;
}

__init static unsigned int avdecc_inflight_data_size(struct avdecc_entity_config *cfg)
{
	return cfg->max_inflights * sizeof(struct inflight_ctx);
}

__init static int avdecc_inflight_init(struct entity *entity, void *data, struct avdecc_entity_config *cfg)
{
	int i;

	entity->inflight_storage = (struct inflight_ctx *)data;
	entity->max_inflights = cfg->max_inflights;

	list_head_init(&entity->free_inflight);

	for (i = 0; i < entity->max_inflights; i++) {
		list_add(&entity->free_inflight, &entity->inflight_storage[i].list);
		entity->inflight_storage[i].timeout.data = &entity->inflight_storage[i];
		entity->inflight_storage[i].timeout.func = &avdecc_inflight_timeout;
		if (timer_create(entity->avdecc->timer_ctx, &entity->inflight_storage[i].timeout, 0, AVDECC_CFG_INFLIGHT_TIMER_RESOLUTION) < 0) {
			os_log(LOG_ERR, "entity(%p) timer_create failed\n", entity);
			goto err;
		}
		entity->inflight_storage[i].entity = entity;
	}

	os_log(LOG_INIT, "avdecc(%p) entity(%p) %d inflight commands max\n", entity->avdecc, entity, entity->max_inflights);

	return 0;

err:
	while(i--)
		timer_destroy(&entity->inflight_storage[i].timeout);

	return -1;
}

__exit static int avdecc_inflight_exit(struct entity *entity)
{
	int i;

	for (i = 0; i < entity->max_inflights; i++)
		timer_destroy(&entity->inflight_storage[i].timeout);

	return 0;
}

static void avdecc_entity_lock_timeout(void *data)
{
	struct entity *entity = (struct entity *)data;
	struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										AEM_DESC_TYPE_ENTITY, 0, NULL);

	entity_dynamic->lock_status = UNLOCKED;

	/* Send unsolicited notification (notify that the entity is now unlocked)
	* Per AVNU.IO.CONTROL 7.5.2.
	*/
	if (entity->milan_mode)
		aecp_aem_send_async_unsolicited_notification(&entity->aecp, AECP_AEM_CMD_LOCK_ENTITY, AEM_DESC_TYPE_ENTITY, 0);
}

__init static bool avdecc_entity_check(struct entity *entity)
{
	u16 avb_itf_index, num_of_formats, control_value_type, clock_source_type, memory_obj_type;
	unsigned int num_interfaces, num_stream_input, num_stream_output, num_control;
	unsigned int num_clock_domain, num_of_redundant_streams, num_memory_object;
	struct memory_object_descriptor *memory_object_desc;
	struct clock_domain_descriptor *clock_domain_desc;
	struct clock_source_descriptor *clock_source_desc;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct control_descriptor *control_desc;
	struct stream_descriptor *stream_desc;
	int i, j;

	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	if ((num_interfaces > avdecc->port_max) || (num_interfaces > AEM_NUM_AVB_INTERFACES_MAX)) {
		os_log(LOG_ERR, "entity(%p) unsupported num interfaces %u, max ports %u, max avb interfaces %u\n",
			entity, num_interfaces, avdecc->port_max, AEM_NUM_AVB_INTERFACES_MAX);
		goto err;
	}

	for (i = 0; i < num_interfaces; i++) {
		if (!avdecc->port[i].initialized) {
			os_log(LOG_ERR, "entity(%p) unsupported avb interface index(%u)\n", entity, i);
			goto err;
		}
	}

	num_stream_input = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT);
	for (i = 0; i < num_stream_input; i++) {
		stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream input index(%u)\n", entity, i);
			goto err;
		}

		avb_itf_index = ntohs(stream_desc->avb_interface_index);
		if (avb_itf_index >= avdecc->port_max || !avdecc->port[avb_itf_index].initialized) {
			os_log(LOG_ERR, "entity(%p) unsupported avb interface index(%u) for stream input %u\n",
				entity, avb_itf_index, i);
			goto err;
		}

		num_of_formats = ntohs(stream_desc->number_of_formats);
		if (num_of_formats > AEM_NUM_FORMATS_MAX) {
			os_log(LOG_ERR, "entity(%p) number of formats %u exceeding max %u for stream input %u\n",
				entity, num_of_formats, AEM_NUM_FORMATS_MAX, i);
			goto err;
		}

		num_of_redundant_streams = ntohs(stream_desc->number_of_redundant_streams);
		if (num_of_redundant_streams > CFG_AVTP_REDUNDANT_STREAMS_MAX) {
			os_log(LOG_ERR, "entity(%p) number of redundant streams %u exceeding max %u for stream input %u\n",
				entity, num_of_redundant_streams, CFG_AVTP_REDUNDANT_STREAMS_MAX, i);
			goto err;
		}
	}

	num_stream_output = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT);
	for (i = 0; i < num_stream_output; i++) {
		stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (!stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream output index(%u)\n", entity, i);
			goto err;
		}

		avb_itf_index = ntohs(stream_desc->avb_interface_index);
		if (avb_itf_index >= avdecc->port_max || !avdecc->port[avb_itf_index].initialized) {
			os_log(LOG_ERR, "entity(%p) unsupported avb interface index(%u) for stream output %u\n",
				entity, avb_itf_index, i);
			goto err;
		}

		num_of_formats = ntohs(stream_desc->number_of_formats);
		if (num_of_formats > AEM_NUM_FORMATS_MAX) {
			os_log(LOG_ERR, "entity(%p) number of formats %u exceeding max %u for stream output %u\n",
				entity, num_of_formats, AEM_NUM_FORMATS_MAX, i);
			goto err;
		}

		num_of_redundant_streams = ntohs(stream_desc->number_of_redundant_streams);
		if (num_of_redundant_streams > CFG_AVTP_REDUNDANT_STREAMS_MAX) {
			os_log(LOG_ERR, "entity(%p) number of redundant streams %u exceeding max %u for stream output %u\n",
				entity, num_of_redundant_streams, CFG_AVTP_REDUNDANT_STREAMS_MAX, i);
			goto err;
		}
	}

	num_control = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_CONTROL);
	for (i = 0; i < num_control; i++) {
		control_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CONTROL, i, NULL);
		if (!control_desc) {
			os_log(LOG_ERR, "entity(%p) invalid control index(%u)\n", entity, i);
			goto err;
		}

		control_value_type = AEM_CONTROL_GET_VALUE_TYPE(ntohs(control_desc->control_value_type));

		switch (control_value_type) {
		case AEM_CONTROL_LINEAR_UINT8:
			if (ntohs(control_desc->number_of_values) > AEM_NUM_VALUE_DETAILS_LINEAR_U8_MAX) {
				os_log(LOG_ERR, "entity(%p) control descriptor(%d) control_value_type(%u): number_of_values(%u) exceeding max(%u)\n",
							entity, i, control_value_type, ntohs(control_desc->number_of_values), AEM_NUM_VALUE_DETAILS_LINEAR_U8_MAX);
				goto err;
			}
			break;

		case AEM_CONTROL_UTF8:
			if (ntohs(control_desc->number_of_values) > AEM_NUM_VALUE_DETAILS_LINEAR_UTF8_MAX) {
				os_log(LOG_ERR, "entity(%p) control descriptor(%d) control_value_type(%u): number_of_values(%u) exceeding max(%u)\n",
							entity, i, control_value_type, ntohs(control_desc->number_of_values), AEM_NUM_VALUE_DETAILS_LINEAR_UTF8_MAX);
				goto err;
			}
			break;

		default:
			os_log(LOG_ERR, "entity(%p) control descriptor(%d) unsupported control_value_type(%u)\n", entity, i, control_value_type);
			goto err;
		}
	}

	num_clock_domain = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN);
	for (i = 0; i < num_clock_domain; i++) {
		clock_domain_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, i, NULL);
		if (!clock_domain_desc) {
			os_log(LOG_ERR, "entity(%p) invalid clock domain index(%u)\n", entity, i);
			goto err;
		}

		for (j = 0; j < ntohs(clock_domain_desc->clock_sources_count); j++) {
			clock_source_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_SOURCE, ntohs(clock_domain_desc->clock_sources[j]), NULL);
			if (!clock_source_desc) {
				os_log(LOG_ERR, "entity(%p) clock source(%u) referenced by clock domain(%u) does not exist\n",
					entity, ntohs(clock_domain_desc->clock_sources[j]), i);
				goto err;
			}

			clock_source_type = ntohs(clock_source_desc->clock_source_type);

			if ((clock_source_type != AEM_CLOCK_SOURCE_TYPE_INTERNAL) && (clock_source_type != AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM)) {
				os_log(LOG_ERR, "entity(%p) clock source(%u) of clock domain(%u) has an unsupported clock source type(%u)\n",
					entity, ntohs(clock_domain_desc->clock_sources[j]), i, clock_source_type);
				goto err;
			}

			if (clock_source_type == AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM) {
				stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, ntohs(clock_source_desc->clock_source_location_index), NULL);
				if (!stream_desc) {
					os_log(LOG_ERR, "entity(%p) stream input(%u) referenced by clock source(%u) does not exist\n",
						entity, ntohs(clock_source_desc->clock_source_location_index), ntohs(clock_domain_desc->clock_sources[j]));
					goto err;
				}
			}
		}
	}

	num_memory_object = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_MEMORY_OBJECT);
	if (num_memory_object > AEM_NUM_MEMORY_OBJECTS_MAX) {
		os_log(LOG_ERR, "entity(%p) unsupported num memory objects %u, max memory objects %u\n",
			entity, num_memory_object, AEM_NUM_MEMORY_OBJECTS_MAX);
		goto err;
	}

	for (i = 0; i < num_memory_object; i++) {
		memory_object_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_MEMORY_OBJECT, i, NULL);
		if (!memory_object_desc) {
			os_log(LOG_ERR, "entity(%p) invalid memory object index(%u)\n", entity, i);
			goto err;
		}

		memory_obj_type = ntohs(memory_object_desc->memory_object_type);

		if ((memory_obj_type != AEM_MEMORY_OBJECT_PNG_MANUFACTURER) &&
		    (memory_obj_type != AEM_MEMORY_OBJECT_PNG_ENTITY) &&
		    (memory_obj_type != AEM_MEMORY_OBJECT_PNG_GENERIC)) {
			os_log(LOG_ERR, "entity(%p) memory object(%u) unsupported type(%u), only PNG types are supported\n",
				entity, i, memory_obj_type);
			goto err;
		}
	}

	return true;

err:
	return false;
}

__init static int avdecc_dynamic_states_init(struct entity *entity, struct avdecc_entity_config *cfg)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct clock_domain_dynamic_desc *clock_domain_dynamic;
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	struct entity_dynamic_desc *entity_dynamic;
	struct avdecc_ctx *avdecc = entity->avdecc;
	unsigned int num_desc;
	int i;

	entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
	if (!entity_dynamic)
		goto err_entity;

	entity_dynamic->lock_status = UNLOCKED;
	entity_dynamic->acquire_status = RELEASED;

	entity_dynamic->lock_timer.func = &avdecc_entity_lock_timeout;
	entity_dynamic->lock_timer.data = entity;

	if (timer_create(avdecc->timer_ctx, &entity_dynamic->lock_timer, 0, AVDECC_CFG_ENTITY_LOCK_TIMER_GRANULARITY_MS) < 0) {
		os_log(LOG_ERR, "entity(%p) timer_create failed\n", entity);
		goto err_timer;
	}

	num_desc = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	for (i = 0; i < num_desc; i++) {
		avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);
		if (!avb_itf_dynamic)
			goto err_desc;

		avb_itf_dynamic->entity = entity;
		avb_itf_dynamic->interface_index = i;

		os_memset(&avb_itf_dynamic->diagnostic_counters, 0, sizeof(avb_itf_dynamic->diagnostic_counters));

		avb_itf_dynamic->operational_state = false;
		avb_itf_dynamic->as_capable = false;

		avb_itf_dynamic->gptp_grandmaster_id = 0;

		avb_itf_dynamic->propagation_delay = 0;

		avb_itf_dynamic->max_ptlv_entries = cfg->max_ptlv_entries;
		avb_itf_dynamic->num_ptlv_entries = 0;
	}

	num_desc = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT);
	for (i = 0; i < num_desc; i++) {
		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_input_dynamic)
			goto err_desc;

		stream_input_dynamic->entity = entity;
		stream_input_dynamic->unique_id = i;

		stream_input_dynamic->set_id = REDUNDANT_SET_ID_INVALID;

		os_memset(&stream_input_dynamic->diagnostic_counters, 0, sizeof(stream_input_dynamic->diagnostic_counters));
	}

	num_desc = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT);
	for (i = 0; i < num_desc; i++) {
		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (!stream_output_dynamic)
			goto err_desc;

		stream_output_dynamic->entity = entity;
		stream_output_dynamic->unique_id = i;

		stream_output_dynamic->set_id = REDUNDANT_SET_ID_INVALID;

		os_memset(&stream_output_dynamic->diagnostic_counters, 0, sizeof(stream_output_dynamic->diagnostic_counters));
	}

	num_desc = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN);
	for (i = 0; i < num_desc; i++) {
		clock_domain_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, i, NULL);
		if (!clock_domain_dynamic)
			goto err_desc;

		clock_domain_dynamic->entity = entity;
		clock_domain_dynamic->unique_id = i;

		clock_domain_dynamic->reserved = 0;

		clock_domain_dynamic->default_mcr_prio = AEM_DEFAULT_MCR_PRIO;
		clock_domain_dynamic->user_mcr_prio = clock_domain_dynamic->default_mcr_prio;

		os_memset(clock_domain_dynamic->media_clock_domain_name, 0, AEM_STR_LEN_MAX);
		os_memcpy(clock_domain_dynamic->media_clock_domain_name, AEM_DEFAULT_MEDIA_CLOCK_DOMAIN_NAME, strlen(AEM_DEFAULT_MEDIA_CLOCK_DOMAIN_NAME));

		os_memset(&clock_domain_dynamic->diagnostic_counters, 0, sizeof(clock_domain_dynamic->diagnostic_counters));
	}

	return 0;

err_desc:
	timer_destroy(&entity_dynamic->lock_timer);

err_timer:
err_entity:
	return -1;
}

/**
 * Checks if two stream descriptors (of the same type) support the same set of formats, in the same order.
 * \return			true if they do, false otherwise.
 * \param stream_desc_src	Pointer to a reference stream descriptor context.
 * \param stream_desc_dst	Pointer to a stream descriptor context.
 */
__init static bool avdecc_are_streams_supporting_same_formats(struct stream_descriptor *stream_desc_src, struct stream_descriptor *stream_desc_dst)
{
	bool rc = true;

	if (stream_desc_src->number_of_formats != stream_desc_dst->number_of_formats) {
		rc = false;
		goto out;
	}

	rc = (os_memcmp(stream_desc_src->formats, stream_desc_dst->formats,
		sizeof(u64) * ntohs(stream_desc_src->number_of_formats)) == 0);

out:
	return rc;
}

/** Initialize the redundant sets for Listener (Input) streams
 * by attributing each set an ID and an index to each stream included in the set.
 * As per Milan v1.2 section 8.
 * \return	-1 if the redundant sets couldn't be set up, 0 otherwise.
 * \param	entity		Pointer to entity context.
 */
__init static int avdecc_dynamic_redundant_listener_set_init(struct entity *entity)
{
	struct stream_input_dynamic_desc *redundant_input_desc;
	struct stream_input_dynamic_desc *stream_input_desc;
	unsigned int set_id = LISTENER_SET_ID_BASE;
	struct stream_descriptor *redundant_desc;
	struct stream_descriptor *stream_desc;
	unsigned int set_index = 0;
	u16 redundant_desc_index;
	u16 num_redundant_desc;
	unsigned int num_desc;
	bool valid_set = false;
	bool new_set = false;
	int i, j, k;

	/* Start with non redundant streams, for consistency they're also assigned to a redundant set with set_id = listener base + stream's unique_id */
	num_desc = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT);
	for (i = 0; i < num_desc; i++) {
		stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream input index(%u)\n", entity, i);
			goto err;
		}

		/* Non redundant streams */
		if (!STREAM_HAS_REDUNDANT_STREAMS(stream_desc)) {
			stream_input_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
			if (!stream_input_desc) {
				os_log(LOG_ERR, "entity(%p) invalid stream input index(%u)\n", entity, i);
				goto err;
			}

			/* Start set_id for actual redundant sets at listener base + last non redundant stream's unique id + 1 */
			set_id = LISTENER_SET_ID_BASE + stream_input_desc->unique_id;

			stream_input_desc->set_id = set_id;
			stream_input_desc->set_index = 0;

			os_log(LOG_INFO, "entity(%p) stream input(%u): redundant set id(%u), redundant stream index(%u)\n",
				entity, i, stream_input_desc->set_id, stream_input_desc->set_index);

			/* Next set_id usable by actual redundant sets */
			set_id++;
		}
	}

	/* Assign a proper set_id to the actual redundant sets containing redundant streams */
	for (i = 0; i < num_desc; i++) {
		stream_input_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		stream_desc = (struct stream_descriptor *)aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_input_desc || !stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream input index(%u)\n", entity, i);
			goto err;
		}

		/* Skip non redundant streams. They already have their set_id */
		if (!STREAM_HAS_REDUNDANT_STREAMS(stream_desc))
			continue;

		/* By default a redundant stream has an invalid redundant set id, in other words,
		 * The redundant stream isn't part of a redundant set yet.
		 */
		new_set = (stream_input_desc->set_id == REDUNDANT_SET_ID_INVALID);

		if (new_set) {
			set_index = 0;

			stream_input_desc->set_id = set_id;
			stream_input_desc->set_index = set_index;

			os_log(LOG_INFO, "entity(%p) stream input(%u): redundant set id(%u), redundant stream index(%u)\n",
				entity, i, stream_input_desc->set_id, stream_input_desc->set_index);

			set_index++;
		}

		num_redundant_desc = ntohs(stream_desc->number_of_redundant_streams);

		for (j = 0; j < num_redundant_desc; j++) {
			redundant_desc_index = ntohs(stream_desc->redundant_streams[j]);

			redundant_input_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, redundant_desc_index, NULL);
			redundant_desc = (struct stream_descriptor *)aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, redundant_desc_index, NULL);
			if (!redundant_input_desc || !redundant_desc) {
				os_log(LOG_ERR, "entity(%p) invalid redundant(%u, %u) stream input index(%u)\n",
					entity, i, j, redundant_desc_index);
				goto err;
			}

			/* On the first pass for a new redundant set :
			 * Propagate the current redundant set's id and set the index of the redundant stream accordingly,
			 * Check that all streams of the redundant set have coherent common fields.
			 */
			if (new_set) {
				/* Set the ID and indexes on redundant streams that are part of the new redundant set.
				 * If the redundant stream's set_id isn't INVALID at this point,
				 * that means that it has previously been referenced in another redundant set.
				 */
				if (redundant_input_desc->set_id == REDUNDANT_SET_ID_INVALID) {
					redundant_input_desc->set_id = set_id;
					redundant_input_desc->set_index = set_index;

					os_log(LOG_INFO, "entity(%p) stream input(%u): redundant set id(%u), redundant stream index(%u)\n",
						entity, redundant_desc_index, redundant_input_desc->set_id, redundant_input_desc->set_index);

					set_index++;

				} else {
					os_log(LOG_ERR, "entity(%p) redundant stream input(%u) is already referenced in another redundant set(%u), expected(%u)\n",
						entity, redundant_desc_index, redundant_input_desc->set_id, stream_input_desc->set_id);
					goto err;
				}

				/* Check that the redundant streams reference the same number of streams,
				 * As all streams, of a given redundant set, should reference each other.
				 */
				if (redundant_desc->number_of_redundant_streams != stream_desc->number_of_redundant_streams) {
					os_log(LOG_ERR, "entity(%p) stream input(%u) and redundant stream input(%u) reference a different number of redundant streams(%u), expected(%u)\n",
						entity, i, redundant_desc_index, redundant_desc->number_of_redundant_streams,
						stream_desc->number_of_redundant_streams);
					goto err;
				}

				/* The streams of a redundant set should have the same format.
				 * And SET_STREAM_FORMAT commands should be applied to all streams of a redundant set.
				 * As per Milan v1.2 section 8.3.2.3.1
				 */
				if (!cmp_64(&stream_desc->current_format, &redundant_desc->current_format)) {
					os_log(LOG_ERR, "entity(%p) stream input(%u)'s and redundant stream input(%u)'s format don't match\n",
						entity, i, redundant_desc_index);
					goto err;
				}

				/* Check also that the streams from the same redundant set support the same set of formats */
				if (!avdecc_are_streams_supporting_same_formats(stream_desc, redundant_desc)) {
					os_log(LOG_ERR, "entity(%p) stream input(%u)'s and redundant stream input(%u)'s supported formats array don't match\n",
						entity, i, redundant_desc_index);
					goto err;
				}

				/* The streams of a redundant set should share the same clock domain.
				 * And setting one of these streams input as the clock source should have the same effect, regarless of which stream, in the set, it is.
				 * As per Milan v1.2 section 8.3.2.3.3
				 */
				if (stream_desc->clock_domain_index != redundant_desc->clock_domain_index) {
					os_log(LOG_ERR, "entity(%p) stream input(%u) and redundant stream input(%u) don't share the same clock domain, index(%u) != index(%u)\n",
						entity, i, redundant_desc_index, ntohs(stream_desc->clock_domain_index), ntohs(redundant_desc->clock_domain_index));
					goto err;
				}

				/* The streams of a redundant set should have the same msrp_accumulated_latency.
				 * For stream inputs, it comes from the reported SRP talker attributes.
				 * Talkers should be the ones maintaining msrp_accumulated_latency (presentation_time_offset) coherency.
				 * It is init to max_transit_time for SR class A in acmp_milan_init(),
				 * and SET_STREAM_INFO commands, through which msrp_accumulated_latency can be changed,
				 * should be aligned with Milan v1.2 section 8.3.2.3.2
				 */

			} else { /* !new_set */
				/* Check that the redundant streams aren't referenced in more than one / in another redundant set. */
				if (redundant_input_desc->set_id != stream_input_desc->set_id) {
					os_log(LOG_ERR, "entity(%p) stream input(%u) has redundant stream input(%u) in a different redundant set(%u), expected(%u)\n",
						entity, i, redundant_desc_index, redundant_input_desc->set_id, stream_input_desc->set_id);
					goto err;
				}
			}

			/* All streams in a given redundant set should reference each others.
			 * Check that the redundant stream reference the initial stream back.
			 */
			for (k = 0; k < ntohs(redundant_desc->number_of_redundant_streams); k++) {
				if (ntohs(redundant_desc->redundant_streams[k]) == i) {
					valid_set = true;
					break;
				}
			}

			if (!valid_set) {
				os_log(LOG_ERR, "entity(%p) redundant stream input(%u) does not reference stream input(%u) back\n",
					entity, redundant_desc_index, i);
				goto err;
			}

			entity->has_redundant_streams = true;

			valid_set = false;
		}

		if (new_set)
			set_id++;
	}

	return 0;

err:
	return -1;
}

/** Initialize the redundant sets for Talker (Output) streams
 * by attributing each set an ID and an index to each stream included in the set.
 * As per Milan v1.2 section 8.
 * \return	-1 if the redundant sets couldn't be set up, 0 otherwise.
 * \param	entity		Pointer to entity context.
 */
__init static int avdecc_dynamic_redundant_talker_set_init(struct entity *entity)
{
	struct stream_output_dynamic_desc *redundant_output_desc;
	struct stream_output_dynamic_desc *stream_output_desc;
	struct stream_descriptor *redundant_desc;
	unsigned int set_id = TALKER_SET_ID_BASE;
	struct stream_descriptor *stream_desc;
	unsigned int set_index = 0;
	u16 redundant_desc_index;
	u16 num_redundant_desc;
	unsigned int num_desc;
	bool valid_set = false;
	bool new_set = false;
	int i, j, k;

	/* Start with non redundant streams, for consistency they're also assigned to a redundant set with set_id = talker base + stream's unique_id */
	num_desc = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT);
	for (i = 0; i < num_desc; i++) {
		stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (!stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream output index(%u)\n", entity, i);
			goto err;
		}

		/* Non redundant streams */
		if (!STREAM_HAS_REDUNDANT_STREAMS(stream_desc)) {
			stream_output_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
			if (!stream_output_desc) {
				os_log(LOG_ERR, "entity(%p) invalid stream output index(%u)\n", entity, i);
				goto err;
			}

			/* Start set_id for actual redundant sets at talker base + last non redundant stream's unique id + 1 */
			set_id = TALKER_SET_ID_BASE + stream_output_desc->unique_id;

			stream_output_desc->set_id = set_id;
			stream_output_desc->set_index = 0;

			os_log(LOG_INFO, "entity(%p) stream output(%u): redundant set id(%u), redundant stream index(%u)\n",
				entity, i, stream_output_desc->set_id, stream_output_desc->set_index);

			/* Next set_id usable by actual redundant sets */
			set_id++;
		}
	}

	/* Assign a proper set_id to the actual redundant sets containing redundant streams */
	for (i = 0; i < num_desc; i++) {
		stream_output_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		stream_desc = (struct stream_descriptor *)aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (!stream_output_desc || !stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream output index(%u)\n", entity, i);
			goto err;
		}

		/* Skip non redundant streams. They already have their set_id */
		if (!STREAM_HAS_REDUNDANT_STREAMS(stream_desc))
			continue;

		/* By default a redundant stream has an invalid redundant set id, in other words,
		 * The redundant stream isn't part of a redundant set yet.
		 */
		new_set = (stream_output_desc->set_id == REDUNDANT_SET_ID_INVALID);

		if (new_set) {
			set_index = 0;

			stream_output_desc->set_id = set_id;
			stream_output_desc->set_index = set_index;

			os_log(LOG_INFO, "entity(%p) stream output(%u): redundant set id(%u), redundant stream index(%u)\n",
				entity, i, stream_output_desc->set_id, stream_output_desc->set_index);

			set_index++;
		}

		num_redundant_desc = ntohs(stream_desc->number_of_redundant_streams);

		for (j = 0; j < num_redundant_desc; j++) {
			redundant_desc_index = ntohs(stream_desc->redundant_streams[j]);

			redundant_output_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, redundant_desc_index, NULL);
			redundant_desc = (struct stream_descriptor *)aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, redundant_desc_index, NULL);
			if (!redundant_output_desc || !redundant_desc) {
				os_log(LOG_ERR, "entity(%p) invalid redundant(%u, %u) stream output index(%u)\n",
					entity, i, j, redundant_desc_index);
				goto err;
			}

			/* On the first pass for a new redundant set :
			 * Propagate the current redundant set's id and set the index of the redundant stream accordingly,
			 * Check that all streams of the redundant set have coherent common fields.
			 */
			if (new_set) {
				/* Set the ID and indexes on redundant streams that are part of the new redundant set.
				 * If the redundant stream's set_id isn't INVALID at this point,
				 * that means that it has previously been referenced in another redundant set.
				 */
				if (redundant_output_desc->set_id == REDUNDANT_SET_ID_INVALID) {
					redundant_output_desc->set_id = set_id;
					redundant_output_desc->set_index = set_index;

					os_log(LOG_INFO, "entity(%p) stream output(%u): redundant set id(%u), redundant stream index(%u)\n",
						entity, redundant_desc_index, redundant_output_desc->set_id, redundant_output_desc->set_index);

					set_index++;
				} else {
					os_log(LOG_ERR, "entity(%p) redundant stream output(%u) is already referenced in another redundant set(%u), expected(%u)\n",
						entity, redundant_desc_index, redundant_output_desc->set_id, stream_output_desc->set_id);
					goto err;
				}

				/* Check that the redundant streams reference the same number of streams,
				 * As all streams, of a given redundant set, should reference each other.
				 */
				if (redundant_desc->number_of_redundant_streams != stream_desc->number_of_redundant_streams) {
					os_log(LOG_ERR, "entity(%p) stream output(%u) and redundant stream output(%u) reference a different number of redundant streams(%u), expected(%u)\n",
						entity, i, redundant_desc_index, redundant_desc->number_of_redundant_streams,
						stream_desc->number_of_redundant_streams);
					goto err;
				}

				/* The streams of a redundant set should have the same format.
				 * And SET_STREAM_FORMAT commands should be applied to all streams of a redundant set.
				 * As per Milan v1.2 section 8.3.2.3.1
				 */
				if (!cmp_64(&stream_desc->current_format, &redundant_desc->current_format)) {
					os_log(LOG_ERR, "entity(%p) stream output(%u)'s and redundant stream output(%u)'s format don't match\n",
						entity, i, redundant_desc_index);
					goto err;
				}

				/* Check also that the streams from the same redundant set support the same set of formats */
				if (!avdecc_are_streams_supporting_same_formats(stream_desc, redundant_desc)) {
					os_log(LOG_ERR, "entity(%p) stream output(%u)'s and redundant stream output(%u)'s supported formats array don't match\n",
						entity, i, redundant_desc_index);
					goto err;
				}

				/* The streams of a redundant set should share the same clock domain.
				 * As per Milan v1.2 section 8.3.2.3.3
				 */
				if (stream_desc->clock_domain_index != redundant_desc->clock_domain_index) {
					os_log(LOG_ERR, "entity(%p) stream output(%u) and redundant stream output(%u) don't share the same clock domain, index(%u) != index(%u)\n",
						entity, i, redundant_desc_index, ntohs(stream_desc->clock_domain_index), ntohs(redundant_desc->clock_domain_index));
					goto err;
				}

				/* The streams of a redundant set should have the same msrp_accumulated_latency (presentation_time_offset).
				 * For stream outputs, it is init to max_transit_time for SR class A in acmp_milan_init(),
				 * and SET_STREAM_INFO commands, through which msrp_accumulated_latency can be changed,
				 * should be aligned with Milan v1.2 section 8.3.2.3.2
				 */

			} else { /* !new_set */
				/* Check that the redundant streams aren't referenced in more than one / in another redundant set. */
				if (redundant_output_desc->set_id != stream_output_desc->set_id) {
					os_log(LOG_ERR, "entity(%p) stream output(%u) has redundant stream output(%u) in a different redundant set(%u), expected(%u)\n",
						entity, i, redundant_desc_index, redundant_output_desc->set_id, stream_output_desc->set_id);
					goto err;
				}
			}

			/* All streams in a given redundant set should reference each others.
			 * Check that the redundant stream reference the initial stream back.
			 */
			for (k = 0; k < ntohs(redundant_desc->number_of_redundant_streams); k++) {
				if (ntohs(redundant_desc->redundant_streams[k]) == i) {
					valid_set = true;
					break;
				}
			}

			if (!valid_set) {
				os_log(LOG_ERR, "entity(%p) redundant stream output(%u) does not reference stream output(%u) back\n",
					entity, redundant_desc_index, i);
				goto err;
			}

			entity->has_redundant_streams = true;

			valid_set = false;
		}

		if (new_set)
			set_id++;
	}

	return 0;

err:
	return -1;
}

__init static int avdecc_entity_mc_init(struct entity *entity)
{
	unsigned char mc_addr[6] = MC_ADDR_AVDECC_ADP_ACMP;
	struct avdecc_ctx *avdecc = entity->avdecc;
	int num_interfaces, i, j;

	/* dynamic settings */
	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	for (i = 0; i < num_interfaces; i++) {

		if (net_add_multi(&avdecc->port[i].net_rx, avdecc_port_to_logical(entity->avdecc, i), mc_addr) < 0) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) entity(%p) cannot add multicast address\n", avdecc, i, entity);
			goto err;
		}
	}
	return 0;

err:
	for (j = 0; j < i; j++)
		net_del_multi(&avdecc->port[j].net_rx, avdecc_port_to_logical(entity->avdecc, i), mc_addr);

	return -1;
}

__exit static void avdecc_entity_mc_exit(struct entity *entity)
{
	unsigned char mc_addr[6] = MC_ADDR_AVDECC_ADP_ACMP;
	struct avdecc_ctx *avdecc = entity->avdecc;
	int num_interfaces, i;

	/* dynamic settings */
	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	for (i = 0; i < num_interfaces; i++)
		net_del_multi(&avdecc->port[i].net_rx, avdecc_port_to_logical(entity->avdecc, i), mc_addr);
}

__exit static void avdecc_dynamic_states_exit(struct entity *entity)
{
	struct entity_dynamic_desc *entity_dynamic;

	entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
						AEM_DESC_TYPE_ENTITY, 0, NULL);

	timer_destroy(&entity_dynamic->lock_timer);

}

__init static struct entity *avdecc_entity_alloc(struct avdecc_entity_config *cfg)
{
	struct entity *entity;
	unsigned int size;

	size = sizeof(struct entity);
	size += avdecc_inflight_data_size(cfg);
	size += aecp_data_size(cfg);
	size += acmp_data_size(cfg);

	entity = os_malloc(size);
	if (!entity)
		goto err;

	os_memset(entity, 0, size);

	return entity;

err:
	return NULL;
}

/* Common AVDECC code entry points */
__init static struct entity *avdecc_entity_init(struct avdecc_ctx *avdecc, int entity_num, struct avdecc_entity_config *cfg)
{
	struct entity *entity;
	u8 *avdecc_data, *acmp_data, *aecp_data;

	/* Pass the milan mode to the entity cfg. */
	cfg->milan_mode = avdecc->milan_mode;

	entity = avdecc_entity_alloc(cfg);
	if (!entity)
		goto err_malloc;

	entity->flags = cfg->flags;
	entity->channel_waitmask = cfg->channel_waitmask;
	entity->valid_time = cfg->valid_time;

	entity->milan_mode = avdecc->milan_mode;
	entity->has_redundant_streams = false; /* false by default, set to true when setting redundant sets' id if at least one redundant set is valid. */
	entity->started = false;
	entity->avdecc = avdecc;

	if (cfg->aem)
		entity->aem_descs = cfg->aem;
	else
		entity->aem_descs = aem_entity_static_init();

	aem_init(entity->aem_descs, cfg, entity_num);

	entity->aem_dynamic_descs = aem_dynamic_descs_init(entity->aem_descs, cfg);
	if (!entity->aem_dynamic_descs) {
		os_log(LOG_CRIT, "Cannot init dynamic descriptors for entity (%d)\n", entity_num);
		goto err_dynamic_desc_init;
	}

	entity->desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
	entity->dynamic_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
	if (!entity->desc || !entity->dynamic_desc) {
		os_log(LOG_CRIT, "Cannot find entity (%d)\n", entity_num);
		goto err_entity_desc;
	}

	entity->index = entity_num;
	entity->system_unique_id = htonl(0);
	entity->channel_openmask = 0;

	if (!avdecc_entity_check(entity))
		goto err_entity_check;

	if (avdecc_dynamic_states_init(entity, cfg) < 0)
		goto err_dynamic_states_init;

	if (avdecc_dynamic_redundant_listener_set_init(entity) < 0)
		goto err_dynamic_redundant_set_init;

	if (avdecc_dynamic_redundant_talker_set_init(entity) < 0)
		goto err_dynamic_redundant_set_init;

	avdecc_data = (u8 *)(entity + 1);
	if (avdecc_inflight_init(entity, avdecc_data, cfg) < 0)
		goto err_inflight;

	if (adp_init(&entity->adp) < 0)
		goto err_adp;

	aecp_data = avdecc_data + avdecc_inflight_data_size(cfg);
	if (aecp_init(&entity->aecp, aecp_data, cfg) < 0)
		goto err_aecp;

	acmp_data = (acmp_data_size(cfg)) ? (aecp_data + aecp_data_size(cfg)) : NULL;
	if (acmp_init(&entity->acmp, acmp_data, cfg) < 0)
		goto err_acmp;

	if (avdecc_entity_mc_init(entity) < 0)
		goto err_multi;

	os_log(LOG_INIT, "entity(%d) (%p) desc(%p), id : %016"PRIx64", name : %s\n",
		entity_num, entity, entity->desc, ntohll(entity->desc->entity_id), entity->desc->entity_name);

	return entity;

err_multi:
	acmp_exit(&entity->acmp);
err_acmp:
	aecp_exit(&entity->aecp);
err_aecp:
	adp_exit(&entity->adp);
err_adp:
	avdecc_inflight_exit(entity);
err_inflight:
err_dynamic_redundant_set_init:
	avdecc_dynamic_states_exit(entity);
err_dynamic_states_init:
err_entity_check:
err_entity_desc:
err_dynamic_desc_init:
	os_free(entity);
err_malloc:
	return NULL;
}

__exit static void avdecc_entity_exit(struct entity *entity)
{
	avdecc_dynamic_states_exit(entity);
	avdecc_entity_mc_exit(entity);
	acmp_exit(&entity->acmp);
	aecp_exit(&entity->aecp);
	adp_exit(&entity->adp);
	avdecc_inflight_exit(entity);
	os_free(entity);
}

/** Fills a buffer (e.g: from a aecp_aem_read_desc_rsp_pdu) with a descriptor's data.
 * Some descriptors use fixed size arrays for theirs attributes, but only the right size
 * (based on the number of elements in each fixed size array) should be copied into the buffer.
 * \return 	length of the copied descriptor's data.
 * \param	buf		pointer to the buffer where the descriptor should be copied to.
 * \param	desc		pointer to the AEM descriptor.
 * \param	desc_type	type of the AEM descriptor.
 * \param	desc_len	length of the AEM descriptor, that can be retieve through aem_get_descriptor().
 */
u16 avdecc_desc_to_network(u8 *buf, struct aem_descriptor_common *desc, u16 desc_type, u16 desc_len)
{
	unsigned int size = 0;
	u16 len = 0;

	switch (desc_type) {
	case AEM_DESC_TYPE_STREAM_INPUT:
	case AEM_DESC_TYPE_STREAM_OUTPUT:
	{
		/* The stack is using fixed size arrays for: formats and redundant_streams in the stream descriptor.
		 * Make sure to copy the right size of the arrays based on the number_of_formats and number_of_redundant_streams
		 * fields.
		 */
		struct stream_descriptor *stream_desc = (struct stream_descriptor *)desc;
		size = offsetof(struct stream_descriptor, formats);

		/* Copy all fields up to the formats */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy formats fields */
		size = sizeof(u64) * ntohs(stream_desc->number_of_formats);
		os_memcpy(buf + len, stream_desc->formats, size);
		len += size;

		/* Copy redundant_streams fields */
		size = sizeof(u16) * ntohs(stream_desc->number_of_redundant_streams);
		os_memcpy(buf + len, stream_desc->redundant_streams, size);
		len += size;

		break;
	}
	case AEM_DESC_TYPE_VIDEO_CLUSTER:
	{
		/* The stack is using fixed size arrays for:
		 * format_specifics, sampling_rates, aspect_ratios, sizes and color_spaces in the video cluster descriptor.
		 * Make sure to copy the right size of the arrays based on the supported_xxx_count fields,
		 * xxx being the attributes the stack is using fixed arrays for.
		 */
		struct video_cluster_descriptor *video_cluster_desc = (struct video_cluster_descriptor *)desc;
		size = offsetof(struct video_cluster_descriptor, supported_format_specifics);

		/* Copy all fields up to the supported_format_specifics */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy supported_format_specifics fields */
		size = sizeof(u32) * ntohs(video_cluster_desc->supported_format_specifics_count);
		os_memcpy(buf + len, video_cluster_desc->supported_format_specifics, size);
		len += size;

		/* Copy supported_sampling_rates fields */
		size = sizeof(u32) * ntohs(video_cluster_desc->supported_sampling_rates_count);
		os_memcpy(buf + len, video_cluster_desc->supported_sampling_rates, size);
		len += size;

		/* Copy supported_aspect_ratios fields */
		size = sizeof(u16) * ntohs(video_cluster_desc->supported_aspect_ratios_count);
		os_memcpy(buf + len, video_cluster_desc->supported_aspect_ratios, size);
		len += size;

		/* Copy supported_sizes fields */
		size = sizeof(u32) * ntohs(video_cluster_desc->supported_sizes_count);
		os_memcpy(buf + len, video_cluster_desc->supported_sizes, size);
		len += size;

		/* Copy supported_color_spaces fields */
		size = sizeof(u16) * ntohs(video_cluster_desc->supported_color_spaces_count);
		os_memcpy(buf + len, video_cluster_desc->supported_color_spaces, size);
		len += size;

		break;
	}
	case AEM_DESC_TYPE_CONFIGURATION:
	{
		/* The stack is using fixed size arrays for: descriptors_counts in the configuration descriptor.
		 * Make sure to copy the right size of the arrays based on the descriptor_counts_count x 2 field.
		 * As it stores pairs of (descriptor type, number of descriptors of said type).
		 */
		struct configuration_descriptor *config_desc = (struct configuration_descriptor *)desc;
		size = offsetof(struct configuration_descriptor, descriptors_counts);

		/* Copy all fields up to the descriptors_counts */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy descriptors_counts fields */
		size = sizeof(u16) * ntohs(config_desc->descriptor_counts_count) * 2;
		os_memcpy(buf + len, config_desc->descriptors_counts, size);
		len += size;

		break;
	}
	case AEM_DESC_TYPE_CLOCK_DOMAIN:
	{
		/* The stack is using fixed size arrays for: clock_sources in the clock_domain descriptor.
		 * Make sure to copy the right size of the arrays based on the clock_sources_count field.
		 */
		struct clock_domain_descriptor *clock_domain_desc = (struct clock_domain_descriptor *)desc;
		size = offsetof(struct clock_domain_descriptor, clock_sources);

		/* Copy all fields up to the clock_sources */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy clock_sources fields */
		size = sizeof(u16) * ntohs(clock_domain_desc->clock_sources_count);
		os_memcpy(buf + len, clock_domain_desc->clock_sources, size);
		len += size;

		break;
	}
	case AEM_DESC_TYPE_AUDIO_UNIT:
	{
		/* The stack is using fixed size arrays for: sampling_rates in the audio_unit descriptor.
		 * Make sure to copy the right size of the arrays based on the sampling_rates_count field.
		 */
		struct audio_unit_descriptor *audio_unit_desc = (struct audio_unit_descriptor *)desc;
		size = offsetof(struct audio_unit_descriptor, sampling_rates);

		/* Copy all fields up to the sampling_rates */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy sampling_rates fields */
		size = sizeof(u32) * ntohs(audio_unit_desc->sampling_rates_count);
		os_memcpy(buf + len, audio_unit_desc->sampling_rates, size);
		len += size;

		break;
	}
	case AEM_DESC_TYPE_AUDIO_MAP:
	{
		/* The stack is using fixed size arrays for: mappings in the audio_map descriptor.
		 * Make sure to copy the right size of the arrays based on the number_of_mappings field.
		 */
		struct audio_map_descriptor *audio_map_desc = (struct audio_map_descriptor *)desc;
		size = offsetof(struct audio_map_descriptor, mappings);

		/* Copy all fields up to the mappings */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy mappings fields */
		size = sizeof(struct audio_mapping) * ntohs(audio_map_desc->number_of_mappings);
		os_memcpy(buf + len, audio_map_desc->mappings, size);
		len += size;

		break;
	}
	case AEM_DESC_TYPE_VIDEO_MAP:
	{
		/* The stack is using fixed size arrays for: mappings in the video_map descriptor.
		 * Make sure to copy the right size of the arrays based on the number_of_mappings field.
		 */
		struct video_map_descriptor *video_map_desc = (struct video_map_descriptor *)desc;
		size = offsetof(struct video_map_descriptor, mappings);

		/* Copy all fields up to the mappings */
		os_memcpy(buf, desc, size);
		len += size;

		/* Copy mappings fields */
		size = sizeof(struct video_mapping) * ntohs(video_map_desc->number_of_mappings);
		os_memcpy(buf + len, video_map_desc->mappings, size);
		len += size;

		break;
	}
	default:
	{
		os_memcpy(buf, desc, desc_len);
		len += desc_len;

		break;
	}
	}

	return len;
}

/** Main AVDECC network receive function.
 * Receives all AVDECC frames and routes them to the different sub-stack
 * components.
 * \return 	none
 * \param 	rx 	pointer to network receive context
 * \param 	desc 	pointer to network receive descriptor
 */
void avdecc_net_rx(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct eth_hdr *eth = (struct eth_hdr *)((char *)desc + desc->l2_offset);
	struct avtp_ctrl_hdr *avtp = (struct avtp_ctrl_hdr *)((char *)desc + desc->l3_offset);
	struct avdecc_port *port = container_of(rx, struct avdecc_port, net_rx);

	switch (avtp->subtype) {
	case AVTP_SUBTYPE_ADP:
		adp_net_rx(port, (struct adp_pdu *)(avtp + 1), avtp->control_data, AVTP_GET_STATUS(avtp), eth->src);
		break;
	case AVTP_SUBTYPE_AECP:
		aecp_net_rx(port, (struct aecp_pdu *)(avtp + 1), avtp->control_data, AVTP_GET_STATUS(avtp), AVTP_GET_CTRL_DATA_LEN(avtp) + 8, eth->src); /* Add 8 to account for the entity id field */
		break;
	case AVTP_SUBTYPE_ACMP:
		acmp_net_rx(port, (struct acmp_pdu *)(avtp + 1), avtp->control_data, AVTP_GET_STATUS(avtp));
		break;
	default :
		os_log(LOG_ERR, "unknown subtype %x\n", avtp->subtype);
		break;
	}

	net_rx_free(desc);
}

/** AVDECC sends inital IPC messages to upper layer to configure the clock domains according to the AEM description of the entity.
 * \return 	none
 * \param 	avdecc 	pointer to AVDECC context
 * \param 	entity 	pointer to Entity context
 */
static void avdecc_ipc_send_default_clock_sources(struct avdecc_ctx *avdecc, struct entity *entity)
{
	struct stream_input_dynamic_desc *stream_input_dynamic_desc;
	struct clock_domain_descriptor *clock_domain_desc;
	struct clock_source_descriptor *clock_source_desc;
	genavb_clock_source_type_t source_type;
	u16 set_id = REDUNDANT_SET_ID_INVALID;
	u16 clock_source_location_index;
	unsigned int num_clock_domain;
	u16 clock_source_type;
	int i;

	if (entity_ready(entity)) {
		num_clock_domain = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN);

		for (i = 0; i < num_clock_domain; i++) {
			clock_domain_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, i, NULL);
			if (!clock_domain_desc) {
				os_log(LOG_ERR, "avdecc(%p) clock_domain(%u) does not exist.\n", avdecc, i);
				continue;
			}

			clock_source_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_SOURCE, ntohs(clock_domain_desc->clock_source_index), NULL);
			if (!clock_source_desc) {
				os_log(LOG_ERR, "avdecc(%p) clock_source(%u) referenced by clock_domain(%u) does not exist.\n",
					avdecc, ntohs(clock_domain_desc->clock_source_index), i);
				continue;
			}

			clock_source_type = ntohs(clock_source_desc->clock_source_type);
			clock_source_location_index = ntohs(clock_source_desc->clock_source_location_index);

			if ((clock_source_type != AEM_CLOCK_SOURCE_TYPE_INTERNAL) && (clock_source_type != AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM)) {
				os_log(LOG_ERR, "avdecc(%p) clock_source_type(%u) of clock_source(%u) for clock_domain(%u) is not supported.\n",
					avdecc, clock_source_type, ntohs(clock_domain_desc->clock_source_index), i);
				continue;
			}

			if (clock_source_type == AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM) {
				stream_input_dynamic_desc = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, clock_source_location_index, NULL);
				if (!stream_input_dynamic_desc) {
					os_log(LOG_ERR, "avdecc(%p) stream_input(%u) acting as clock_source(%u) for clock_domain(%u) does not exist.\n",
						avdecc, clock_source_location_index, ntohs(clock_domain_desc->clock_source_index), i);
					continue;
				}

				set_id = stream_input_dynamic_desc->set_id;
				source_type = GENAVB_CLOCK_SOURCE_TYPE_INPUT_SET;
			} else {
				source_type = GENAVB_CLOCK_SOURCE_TYPE_INTERNAL;
			}

			avdecc_ipc_set_clock_source(avdecc, &avdecc->ipc_tx_media_stack, i,
						    source_type, clock_source_location_index, set_id);
		}
	}
}

static void avdecc_local_entity_updated(struct avdecc_ctx *avdecc, struct entity *entity)
{
	struct adp_discovery_ctx *disc;
	unsigned int num_interfaces;
	int i, port;

	if (entity_ready(entity)) {
		adp_update(&entity->adp);

		if (!avdecc->milan_mode) {

			num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

			for (port = 0; port < num_interfaces; port++) {
				disc = &avdecc->port[port].discovery;

				for (i = 0; i < disc->max_entities_discovery; i++) {
					if (disc->entities[i].in_use)
						avdecc_ieee_try_fast_connect(entity, &disc->entities[i], port);
				}
			}
		}

		if (!entity->started) {

			/* Send the default clock sources message for entities that has opened
			 * the MEDIA_STACK channel
			 */
			if ((entity->channel_openmask & AVDECC_WAITMASK_MEDIA_STACK))
				avdecc_ipc_send_default_clock_sources(avdecc, entity);

			if (avdecc->milan_mode) {
				/* Start Listener sinks that received saved binding parameters. */
				acmp_milan_listener_sink_start_with_binding_params(entity);

				acmp_milan_talkers_maap_start(entity);
			}

			entity->started = true;
		}
	}
}

/** AVDECC IPC receive callback.
 * IPC received from MAAP
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_maap(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct entity *entity;
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_maap);
	avb_u16 status, port_id;
	avb_u8 *base_address;
	avb_u16 count;
	avb_u32 range_id;

	os_log(LOG_DEBUG, "Received MAAP IPC type: %d\n", desc->type);

	switch (desc->type) {
	case GENAVB_MSG_MAAP_STATUS:
		if (desc->len != sizeof(struct genavb_maap_status)) {
			os_log(LOG_ERR, "MAAP status message failed\n");
			break;
		}

		status = desc->u.maap_status.status;
		count = desc->u.maap_status.count;
		base_address = desc->u.maap_status.base_address;
		port_id = desc->u.maap_status.port_id;
		range_id = desc->u.maap_status.range_id;

		os_log(LOG_INFO, "avdecc(%p) port(%u) range(%u) status(%u) base_address (%02x:%02x:%02x:%02x:%02x:%02x) count (%u)\n",
			avdecc, port_id, range_id, status, base_address[0], base_address[1], base_address[2], base_address[3], base_address[4],
			base_address[5], count);

		if (avdecc->milan_mode) {
			entity = avdecc_get_local_talker(avdecc, port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local talker entity supporting this port.\n",
					avdecc, port_id);
				goto exit;
			}

			if (status == MAAP_STATUS_SUCCESS)
				acmp_milan_talker_maap_valid(entity, port_id, range_id, base_address, count);
			else if (status == MAAP_STATUS_CONFLICT)
				acmp_milan_talker_maap_conflict(entity, port_id, range_id, base_address, count);
		}

		break;

	case GENAVB_MSG_MAAP_CREATE_RANGE_RESPONSE:
		status = desc->u.maap_create_response.status;
		range_id = desc->u.maap_create_response.range_id;
		port_id = desc->u.maap_create_response.port_id;

		if (status != MAAP_RESPONSE_SUCCESS) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) range(%u) error on range creation\n", avdecc, port_id, range_id);
			goto exit;
		}

		os_log(LOG_DEBUG, "avdecc(%p) port(%u) range(%u) range creation success\n", avdecc, port_id, range_id);

		break;

	case GENAVB_MSG_MAAP_DELETE_RANGE_RESPONSE:
		status = desc->u.maap_delete_response.status;
		range_id = desc->u.maap_delete_response.range_id;
		port_id = desc->u.maap_delete_response.port_id;

		if (status != MAAP_RESPONSE_SUCCESS) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) range(%u) error on range deletion\n", avdecc, port_id, range_id);
			goto exit;
		}

		os_log(LOG_DEBUG, "avdecc(%p) port(%u) range(%u) range deletion success\n", avdecc, port_id, range_id);

		break;

	default:
		break;
	}

exit:
	ipc_free(rx, desc);
}

/** AVDECC IPC receive callback.
 * IPC received from gPTP application.
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_gptp(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_port *port = container_of(rx, struct avdecc_port, ipc_rx_gptp);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	struct avb_interface_descriptor *avb_itf;
	int i;

	os_log(LOG_DEBUG, "Received IPC type: %d\n", desc->type);

	switch (desc->type) {
	case GENAVB_MSG_GM_STATUS:
		if ((desc->len < (sizeof(struct genavb_msg_gm_status))) || (desc->len != (sizeof(struct genavb_msg_gm_status) + desc->u.gm_status.num_ptlv * sizeof(struct ptp_clock_identity)))) {
			os_log(LOG_ERR, "avdecc(%p) grandmaster status message failed", avdecc);
			break;
		}

		/* We can receive indication from multiple domains, filter on the supported domain.
		 * FIXME: make the supported domain configurable
		 */
		if (desc->u.gm_status.domain != CFG_DEFAULT_GPTP_DOMAIN)
			break;

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];
			bool path_sequence_changed = false;

			avb_itf = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);
			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);

			if (!avb_itf || !avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			/* update the domain number */
			avb_itf->domain_number = desc->u.gm_status.domain;

			if (avb_itf_dynamic->num_ptlv_entries != desc->u.gm_status.num_ptlv)
				path_sequence_changed = true;

			/* update the path sequence to the grandmaster as per Milan Dicovery Connection Control 6.6.1 */
			if (desc->u.gm_status.num_ptlv <= avb_itf_dynamic->max_ptlv_entries) {
				avb_itf_dynamic->num_ptlv_entries = desc->u.gm_status.num_ptlv;
			} else {
				/* Truncate the path trace to the max supported size. */
				avb_itf_dynamic->num_ptlv_entries = avb_itf_dynamic->max_ptlv_entries;
			}

			if (path_sequence_changed || os_memcmp(&avb_itf_dynamic->path_sequence, &desc->u.gm_status.path_sequence, avb_itf_dynamic->num_ptlv_entries * sizeof(struct ptp_clock_identity))) {
				os_memcpy(&avb_itf_dynamic->path_sequence, &desc->u.gm_status.path_sequence, avb_itf_dynamic->num_ptlv_entries * sizeof(struct ptp_clock_identity));

				path_sequence_changed = true;
			}

			if (!cmp_64(&avb_itf_dynamic->gptp_grandmaster_id, &desc->u.gm_status.gm_id)) {
				os_log(LOG_INFO, "GTP GM id change for local entity(%p) old 0x%"PRIx64" new 0x%"PRIx64"\n",
					entity, ntohll(avb_itf_dynamic->gptp_grandmaster_id), ntohll(desc->u.gm_status.gm_id));

				copy_64(&avb_itf_dynamic->gptp_grandmaster_id, &desc->u.gm_status.gm_id);

				avb_itf_dynamic->diagnostic_counters.gptp_gm_changed++;

				avdecc_local_entity_updated(avdecc, entity);

				if (entity_ready(entity)) {
					if (avdecc->milan_mode)
						adp_milan_advertise_sm(entity, port->port_id, ADP_MILAN_ADV_GM_CHANGE);
					else
						adp_ieee_advertise_interface_sm(entity, port->port_id, ADP_INTERFACE_ADV_EVENT_GM_CHANGE);

					aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id);
				}
			}

			if (path_sequence_changed && entity_ready(entity)) {
				/* Register send of an asynchronous GET_AS_PATH unsolicited notification per MILAN Specification v1.2 5.4.5.2
				 * to notify changes in the path sequence from gPTP
				 */
				aecp_register_get_as_path_asyn_notification(entity, avb_itf_dynamic->interface_index);
			}
		}

		break;

	case GENAVB_MSG_GPTP_PORT_PARAMS:
		if (desc->len != sizeof(struct genavb_msg_gptp_port_params)) {
			os_log(LOG_ERR, "avdecc(%p) gptp port params message failed", avdecc);
			break;
		}

		/* Updates only on the concerned port and filter on the supported domain
		 * FIXME: make the supported domain configurable
		 */
		if (desc->u.gptp_port_params.port_id != port->logical_port || desc->u.gptp_port_params.domain != CFG_DEFAULT_GPTP_DOMAIN)
			break;

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);

			if (!avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			avb_itf_dynamic->as_capable = desc->u.gptp_port_params.as_capable;
			avb_itf_dynamic->propagation_delay = desc->u.gptp_port_params.pdelay;
		}

		break;

	case GENAVB_MSG_MANAGED_GET_RESPONSE:
		if (desc->len > GENAVB_MAX_MANAGED_SIZE) {
			os_log(LOG_ERR, "avdecc(%p) gptp managed get response message failed", avdecc);
			break;
		}

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);

			if (!avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			avdecc_ipc_managed_get_response(entity, port->port_id, &desc->u.managed_get_response);
		}

		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

/** Initializes stream input's media_(un)locked counters.
 * As the stream input is always statically mapped to the same clock domain,
 * align the stream input's and its clock domain's current locked status.
 * \return 	none
 * \param 	entity 			pointer to the Entity context
 * \param 	listener_unique_id 	stream input descriptor's index
 */
void avdecc_init_stream_input_clock_domain_counters(struct entity *entity, u16 listener_unique_id)
{
	struct clock_domain_dynamic_desc *clock_domain_dynamic;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct stream_descriptor *stream_input_desc;

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_dynamic)
		goto out;

	stream_input_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_desc)
		goto out;

	clock_domain_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, ntohs(stream_input_desc->clock_domain_index), NULL);
	if (!clock_domain_dynamic)
		goto out;

	/* Update stream input's media_(un)locked counters to align them with its clock domain's current locked status
	 * Note: avdecc_ipc_rx_clock_domain() guarantees that locked = unlocked or locked = unlocked + 1
	 */
	stream_input_dynamic->diagnostic_counters.media_locked = clock_domain_dynamic->diagnostic_counters.locked - clock_domain_dynamic->diagnostic_counters.unlocked;
	stream_input_dynamic->diagnostic_counters.media_unlocked = 0;

	/* By default the stream starts unlocked (both media_locked and media_unlocked counters reset to 0).
	 * If its clock domain is already locked, the stream should be locked too at init/binding.
	 * So notify the controller that the new stream is in fact already locked.
	 */
	if (stream_input_dynamic->diagnostic_counters.media_locked != 0)
		aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id);

out:
	return;
}

/** AVDECC IPC receive callback.
 * IPC received from clock domain application.
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_clock_domain(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_clock_domain);

	os_log(LOG_DEBUG, "Received IPC type: %d\n", desc->type);

	switch (desc->type) {
	case GENAVB_MSG_CLOCK_DOMAIN_STATUS:
	{
		u16 clock_domain_index = desc->u.clock_domain_status.domain - GENAVB_CLOCK_DOMAIN_0;
		struct stream_input_dynamic_desc *stream_input_dynamic;
		struct clock_domain_dynamic_desc *clock_domain_dynamic;
		struct clock_domain_descriptor *clock_domain_desc;
		struct stream_descriptor *stream_input_desc;
		unsigned int num_stream_input, clock_status;
		bool desc_changed = false;
		int i, j;

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			clock_domain_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, clock_domain_index, NULL);
			clock_domain_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, clock_domain_index, NULL);
			if (!clock_domain_desc || !clock_domain_dynamic)
				continue;

			clock_status = desc->u.clock_domain_status.status;

			if (clock_status == GENAVB_CLOCK_DOMAIN_STATUS_LOCKED) {
				if ((clock_domain_dynamic->diagnostic_counters.locked - clock_domain_dynamic->diagnostic_counters.unlocked) == 0) {
					clock_domain_dynamic->diagnostic_counters.locked += 1;

					desc_changed = true;
				} else {
					os_log(LOG_ERR, "avdecc(%p) received erroneous CLOCK_DOMAIN locked status: locked(%u) != unlocked(%u)\n",
						avdecc, clock_domain_dynamic->diagnostic_counters.locked, clock_domain_dynamic->diagnostic_counters.unlocked);
				}

			} else if (clock_status == GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED) {
				if ((clock_domain_dynamic->diagnostic_counters.locked - clock_domain_dynamic->diagnostic_counters.unlocked) == 1) {
					clock_domain_dynamic->diagnostic_counters.unlocked += 1;

					desc_changed = true;
				} else {
					os_log(LOG_ERR, "avdecc(%p) received erroneous CLOCK_DOMAIN unlocked status: locked(%u) != unlocked(%u) + 1\n",
						avdecc, clock_domain_dynamic->diagnostic_counters.locked, clock_domain_dynamic->diagnostic_counters.unlocked);
				}
			}

			if (desc_changed) {
				aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_CLOCK_DOMAIN, clock_domain_index);

				desc_changed = false;
			}

			num_stream_input = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT);

			for (j = 0; j < num_stream_input; j++) {
				if (!acmp_is_stream_running(entity, AEM_DESC_TYPE_STREAM_INPUT, j)) {
					/* We update stream input's media_(un)locked counters only when the stream is running (bound).
					 * Otherwise we would blindly send unnecessary notifications for inactive streams.
					 * Behavior of media_(un)locked counters' update is implementation defined.
					 */
					continue;
				}

				stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, j, NULL);
				stream_input_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, j, NULL);
				if (!stream_input_dynamic || !stream_input_desc)
					continue;

				if (ntohs(stream_input_desc->clock_domain_index) != clock_domain_index)
					continue;

				if (clock_status == GENAVB_CLOCK_DOMAIN_STATUS_LOCKED) {
					if ((stream_input_dynamic->diagnostic_counters.media_locked - stream_input_dynamic->diagnostic_counters.media_unlocked) == 0) {
						stream_input_dynamic->diagnostic_counters.media_locked += 1;

						desc_changed = true;
					}

				} else if (clock_status == GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED) {
					if ((stream_input_dynamic->diagnostic_counters.media_locked - stream_input_dynamic->diagnostic_counters.media_unlocked) == 1) {
						stream_input_dynamic->diagnostic_counters.media_unlocked += 1;

						desc_changed = true;
					}
				}

				if (desc_changed) {
					aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_STREAM_INPUT, j);

					desc_changed = false;
				}
			}
		}
		break;
	}
	default:
		break;
	}

	ipc_free(rx, desc);
}


/** Finds the corresponding dynamic descriptor of a stream INPUT by its stream id
 * \return 	pointer to the dynamic descriptor, or NULL if it wasn't found
 * \param 	entity 		pointer to entity context
 * \param 	stream_id 	the stream's id (in network order)
 */
static struct stream_input_dynamic_desc *find_stream_input_dynamic_desc_by_stream_id(struct entity *entity, u64 stream_id)
{
	unsigned int num_stream_input = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT);
	struct stream_input_dynamic_desc *stream_input_dynamic;
	int i;

	for (i = 0; i < num_stream_input; i++) {
		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_input_dynamic)
			continue;

		if (cmp_64(&stream_input_dynamic->stream_id, &stream_id))
			goto found;
	}

	return NULL;

found:
	return stream_input_dynamic;
}

/** Finds the corresponding dynamic descriptor of a stream OUTPUT by its stream id
 * \return 	pointer to the dynamic descriptor, or NULL if it wasn't found
 * \param 	entity 		pointer to entity context
 * \param 	stream_id 	the stream's id (in network order)
 */
static struct stream_output_dynamic_desc *find_stream_output_dynamic_desc_by_stream_id(struct entity *entity, u64 stream_id)
{
	unsigned int num_stream_output = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT);
	struct stream_output_dynamic_desc *stream_output_dynamic;
	int i;

	for (i = 0; i < num_stream_output; i++) {
		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (!stream_output_dynamic)
			continue;

		if (cmp_64(&stream_output_dynamic->stream_id, &stream_id))
			goto found;
	}

	return NULL;

found:
	return stream_output_dynamic;
}

/** AVDECC IPC receive callback.
 * IPC received from AVTP
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_avtp(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_avtp);

	os_log(LOG_DEBUG, "Received IPC type: %d\n", desc->type);

	switch (desc->type) {
	case IPC_AVTP_COUNTERS:
	{
		struct ipc_avtp_counters *msg = (struct ipc_avtp_counters *)&desc->u;
		unsigned int listener_count = msg->stream_listener_count;
		unsigned int talker_count = msg->stream_talker_count;
		struct stream_output_dynamic_desc *stream_output_dynamic;
		struct stream_input_dynamic_desc *stream_input_dynamic;
		struct ipc_avtp_listener_counters listener_counters;
		struct ipc_avtp_talker_counters talker_counters;
		u8 *msg_counters = msg->data;
		bool desc_changed = false;
		struct entity *entity;
		u64 stream_id;
		int i, j;

		for (i = 0; i < listener_count; i++) {
			listener_counters = ((struct ipc_avtp_listener_counters *)msg_counters)[i];

			stream_id = listener_counters.stream_id; /* Coming from AVTP in Network Order */

			for (j = 0; j < avdecc->num_entities; j++) {
				entity = avdecc->entities[j];

				if (!entity_ready(entity))
					continue;

				if (!entity_has_listener_stream_sinks(entity))
					continue;

				stream_input_dynamic = find_stream_input_dynamic_desc_by_stream_id(entity, stream_id);

				/* Multiple ready entities can co-exist, in a non guaranteed order, one of them holds the stream we're looking to update.
				 * Or static streams could have been defined on AVTP side that AVDECC isn't aware of.
				 * So, silently continue to the next entity if the stream isn't found in the current entity.
				 */
				if (!stream_input_dynamic)
					continue;

				if ((stream_input_dynamic->diagnostic_counters.frame_rx != listener_counters.counters.rx) ||
				    (stream_input_dynamic->diagnostic_counters.seq_num_mismatch != listener_counters.counters.pkt_lost) ||
				    (stream_input_dynamic->diagnostic_counters.media_reset != listener_counters.counters.mr) ||
				    (stream_input_dynamic->diagnostic_counters.timestamp_uncertain != listener_counters.counters.tu) ||
				    (stream_input_dynamic->diagnostic_counters.timestamp_valid != listener_counters.counters.timestamp_valid) ||
				    (stream_input_dynamic->diagnostic_counters.timestamp_invalid != listener_counters.counters.timestamp_invalid) ||
				    (stream_input_dynamic->diagnostic_counters.unsupported_format != listener_counters.counters.unsupported_format) ||
				    (stream_input_dynamic->diagnostic_counters.early_timestamp != listener_counters.counters.early_timestamp) ||
				    (stream_input_dynamic->diagnostic_counters.late_timestamp != listener_counters.counters.late_timestamp) ||
				    (stream_input_dynamic->diagnostic_counters.stream_interruption != listener_counters.counters.stream_interruption))
					desc_changed = true;

				/* Update stream input's diagnostic counters as par Milan v1.2 section 5.3.8.10 */
				stream_input_dynamic->diagnostic_counters.seq_num_mismatch = listener_counters.counters.pkt_lost;
				stream_input_dynamic->diagnostic_counters.media_reset = listener_counters.counters.mr;
				stream_input_dynamic->diagnostic_counters.timestamp_uncertain = listener_counters.counters.tu;
				stream_input_dynamic->diagnostic_counters.timestamp_valid = listener_counters.counters.timestamp_valid;
				stream_input_dynamic->diagnostic_counters.timestamp_invalid = listener_counters.counters.timestamp_invalid;
				stream_input_dynamic->diagnostic_counters.unsupported_format = listener_counters.counters.unsupported_format;
				stream_input_dynamic->diagnostic_counters.early_timestamp = listener_counters.counters.early_timestamp;
				stream_input_dynamic->diagnostic_counters.late_timestamp = listener_counters.counters.late_timestamp;
				stream_input_dynamic->diagnostic_counters.frame_rx = listener_counters.counters.rx;
				stream_input_dynamic->diagnostic_counters.stream_interruption = listener_counters.counters.stream_interruption;

				if (desc_changed) {
					aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_STREAM_INPUT, stream_input_dynamic->unique_id);

					desc_changed = false;
				}

				/* We don't support multiple local listener entities or multiple listeners for the same stream,
				 * so locally, at most one listener matches the stream id.
				 */
				break;
			}
		}

		for (i = 0; i < talker_count; i++) {
			talker_counters = ((struct ipc_avtp_talker_counters *)msg_counters)[listener_count + i];

			stream_id = talker_counters.stream_id; /* Coming from AVTP in Big Endian/network order */

			for (j = 0; j < avdecc->num_entities; j++) {
				entity = avdecc->entities[j];

				if (!entity_ready(entity))
					continue;

				if (!entity_has_talker_stream_sources(entity))
					continue;

				stream_output_dynamic = find_stream_output_dynamic_desc_by_stream_id(entity, stream_id);

				/* Multiple ready entities can co-exist, in a non guaranteed order, one of them holds the stream we're looking to update.
				 * Or static streams could have been defined on AVTP side that AVDECC isn't aware of.
				 * So, silently continue to the next entity if the stream isn't found in the current entity.
				 */
				if (!stream_output_dynamic)
					continue;

				if ((stream_output_dynamic->diagnostic_counters.frame_tx != talker_counters.counters.tx) ||
				    (stream_output_dynamic->diagnostic_counters.media_reset != talker_counters.counters.mr) ||
				    (stream_output_dynamic->diagnostic_counters.timestamp_valid != talker_counters.counters.timestamp_valid) ||
				    (stream_output_dynamic->diagnostic_counters.timestamp_invalid != talker_counters.counters.timestamp_invalid))
					desc_changed = true;

				/* Update stream output's diagnostic counters as par Milan v1.2 section 5.3.7.7 */
				stream_output_dynamic->diagnostic_counters.frame_tx = talker_counters.counters.tx;
				stream_output_dynamic->diagnostic_counters.media_reset = talker_counters.counters.mr;
				stream_output_dynamic->diagnostic_counters.timestamp_valid = talker_counters.counters.timestamp_valid;
				stream_output_dynamic->diagnostic_counters.timestamp_invalid = talker_counters.counters.timestamp_invalid;

				if (desc_changed) {
					aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_STREAM_OUTPUT, stream_output_dynamic->unique_id);

					desc_changed = false;
				}

				/* Talker stream id is unique to the whole network/entities, so at most one talker entity may match the talker stream id */
				break;
			}
		}

		break;
	}
	default:
		break;
	}

	ipc_free(rx, desc);
}

/** Sends an IPC to AVTP to retrieve the counters/stats it track
 * \return 	none
 * \param 	avdecc 	pointer to AVDECC context
 */
static void avdecc_ipc_avtp_get_counters(struct avdecc_ctx *avdecc)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(&avdecc->ipc_tx_avtp, 0);
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = IPC_AVTP_GET_COUNTERS;
		desc->len = 0;
		desc->flags = 0;

		rc = ipc_tx(&avdecc->ipc_tx_avtp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&avdecc->ipc_tx_avtp, desc);
		}
	} else {
		os_log(LOG_ERR, "ipc_alloc() failed\n");
	}
}

static void avdecc_ipc_gm_get_status(struct avdecc_port *port)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(&port->ipc_tx_gptp, sizeof(struct genavb_msg_gm_status));
	if (desc) {
		desc->type = GENAVB_MSG_GM_GET_STATUS;
		desc->len = sizeof(struct genavb_msg_gm_status);
		desc->flags = 0;

		desc->u.gm_get_status.domain = CFG_DEFAULT_GPTP_DOMAIN;

		rc = ipc_tx(&port->ipc_tx_gptp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&port->ipc_tx_gptp, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void avdecc_ipc_srp_deregister_all(struct avdecc_port *port)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct ipc_desc *desc;
	int rc;

	os_log(LOG_INFO, "avdecc(%p) port(%u) Deregister all SRP stream declarations on port.\n", avdecc, port->port_id);

	desc = ipc_alloc(&port->ipc_tx_srp, 0);
	if (desc) {
		desc->type = GENAVB_MSG_DEREGISTER_ALL;
		desc->len = 0;
		desc->flags = 0;

		rc = ipc_tx(&port->ipc_tx_srp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&port->ipc_tx_srp, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

/** Find a local entity (ready or not) with Controller capability.
 *  \return	pointer to first found local controller entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_controller_any(struct avdecc_ctx *avdecc)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if (avdecc->entities[i]->desc->controller_capabilities & htonl(ADP_CONTROLLER_IMPLEMENTED))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity that is ready, with Controller capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local controller entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_controller(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->controller_capabilities & htonl(ADP_CONTROLLER_IMPLEMENTED))
			&& entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity (ready or not) with Talker or Listener capability.
 *  \return	pointer to first matching local entity (if any), NULL if none was foudn.
 */
struct entity *avdecc_get_local_controlled_any(struct avdecc_ctx *avdecc)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED)) ||
			(avdecc->entities[i]->desc->talker_capabilities & htons(ADP_TALKER_IMPLEMENTED)))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity that is ready, with Listener capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local listener entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_listener(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED))
			&& entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity (ready or not) and with Listener capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local listener entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_listener_any(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED))
			&& avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity that is ready, with Talker capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local talker entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_talker(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->talker_capabilities & htons(ADP_TALKER_IMPLEMENTED))
			&& entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Send an IPC HEARTBEAT message on the provided tx IPC.
 * \ return	0 (or positive) on success, -1 otherwise.
 */
static int avdecc_ipc_send_heartbeat(struct ipc_tx *tx, unsigned int ipc_dst)
{
	struct ipc_desc *tx_desc;
	int rc = 0;

	os_log(LOG_DEBUG, "ipc_tx(%p) Sending IPC heartbeat\n", tx);
	tx_desc = ipc_alloc(tx, sizeof(struct ipc_heartbeat));
	if (tx_desc) {
		tx_desc->dst = ipc_dst;
		tx_desc->type = IPC_HEARTBEAT;
		tx_desc->len = sizeof(struct ipc_heartbeat);
		tx_desc->u.hearbeat.status =  0;


		rc = ipc_tx(tx, tx_desc);
		if (rc < 0) {
			if (rc == -IPC_TX_ERR_QUEUE_FULL)
				os_log(LOG_ERR, "ict_tx(%p) ipc_tx() failed (%d)\n", tx, rc);
			ipc_free(tx, tx_desc);
			rc = -1;
		}
	} else {
		os_log(LOG_ERR, "ipc_tx(%p) ipc_alloc() failed\n", tx);
		rc = -1;
	}

	return rc;
}

/** AVDECC IPC receive callback.
 * IPC received from the controller API.
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_controller(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_controller);
	struct entity *entity;
	struct ipc_tx *ipc = &avdecc->ipc_tx_controller;
	int rc = -1;

	os_log(LOG_DEBUG, "Received IPC type: %d\n", desc->type);

	/* Assume at most one controller entity per endpoint.
	 * FIXME to be replaced by explicit entity selection by app
	 * if/when multiple controller entities per endpoint is implemented.
	 */
	entity = avdecc_get_local_controller_any(avdecc);
	if (!entity) {
		os_log(LOG_ERR, "avdecc(%p) Couldn't find any local controller entities, ignoring message from application.\n", avdecc);
		goto exit;
	}

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc = &avdecc->ipc_tx_controller_sync;

	switch (desc->type) {
	case GENAVB_MSG_ACMP_COMMAND:
		if (entity_ready(entity))
			rc = acmp_ipc_rx(entity, &desc->u.acmp_command, desc->len, ipc, desc->src);
		break;
	case GENAVB_MSG_AECP:
		if (entity_ready(entity))
			rc = aecp_ipc_rx_controller(entity, &desc->u.aecp_msg, desc->len, ipc, desc->src);
		break;
	case GENAVB_MSG_ADP:
		if (entity_ready(entity))
			rc = adp_ipc_rx(entity, &desc->u.adp_msg, desc->len, ipc, desc->src);
		break;

	case IPC_HEARTBEAT:
		os_log(LOG_DEBUG, "Sending HEARTBEAT on ipc_tx(%p)\n", ipc);
		avdecc_ipc_send_heartbeat(ipc, desc->src);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_CONTROLLER)) {
			entity->channel_openmask |= AVDECC_WAITMASK_CONTROLLER;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	default:
		os_log(LOG_ERR, "Received unknown message type %d on a CONTROLLER channel\n", desc->type);
		break;
	}

exit:
	if (rc < 0) {
		// FIXME: If we were unable to send a command that expected a response, send an error response back to the application.
	}

	ipc_free(rx, desc);
}

void avdecc_ipc_rx_controlled(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_controlled);
	struct entity *entity;

	os_log(LOG_DEBUG, "avdecc(%p) Received IPC type: %d\n", avdecc, desc->type);

	/* Assume at most one non-controller entity per endpoint.
	 * FIXME to be replaced by explicit entity selection by app
	 * if/when multiple (non-controller) entities per endpoint is implemented.
	 */
	entity = avdecc_get_local_controlled_any(avdecc);
	if (!entity) {
		os_log(LOG_ERR, "avdecc(%p) Couldn't find any local non-controller entities, ignoring message from application.\n", avdecc);
		goto exit;
	}

	switch (desc->type) {
	case GENAVB_MSG_AECP:
		if (entity_ready(entity))
			aecp_ipc_rx_controlled(entity, &desc->u.aecp_msg, desc->len);
		// TODO else ???
		break;

	case IPC_HEARTBEAT:
		avdecc_ipc_send_heartbeat(&avdecc->ipc_tx_controlled, IPC_DST_ALL);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_CONTROLLED)) {
			entity->channel_openmask |= AVDECC_WAITMASK_CONTROLLED;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	default:
		os_log(LOG_ERR, "Received unknown message type %d on a CONTROLLED channel\n", desc->type);
		break;
	}

exit:
	ipc_free(rx, desc);
}


void avdecc_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_media_stack);
	struct entity *entity;

	os_log(LOG_DEBUG, "avdecc(%p) Received IPC type: %d\n", avdecc, desc->type);

	switch (desc->type) {
	case IPC_HEARTBEAT:
		/* Assume at most one non-controller entity per endpoint.
		 * FIXME to be replaced by explicit entity selection by app
		 * if/when multiple (non-controller) entities per endpoint is implemented.
		 */
		entity = avdecc_get_local_controlled_any(avdecc);
		if (!entity) {
			os_log(LOG_ERR, "avdecc(%p) Couldn't find any local non-controller entities, ignoring message from application.\n", avdecc);
			goto exit;
		}

		avdecc_ipc_send_heartbeat(&avdecc->ipc_tx_media_stack, IPC_DST_ALL);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_MEDIA_STACK)) {
			entity->channel_openmask |= AVDECC_WAITMASK_MEDIA_STACK;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	case GENAVB_MSG_MEDIA_STACK_ENTITY_START:
		if (desc->len != sizeof(struct ipc_media_stack_start)) {
			os_log(LOG_ERR, "avdecc(%p) wrong length received %u expected %zu\n", avdecc, desc->len, sizeof(struct ipc_media_stack_start));
			goto exit;
		}

		if (!desc->u.media_stack_bind.entity_id) {
			/* A zero entity ID will return the first non-controller entity.
			 * Only a single non-controller entity is supported anyway (for now).
			 */
			entity = avdecc_get_local_controlled_any(avdecc);

		} else {
			entity = avdecc_get_entity_raw(avdecc, desc->u.media_stack_start.entity_id);
		}

		if (!entity) {
			os_log(LOG_ERR, "avdecc(%p) Couldn't find any entity(%"PRIu64"), ignoring GENAVB_MSG_MEDIA_STACK_ENTITY_START message from application\n",
				avdecc, ntohll(desc->u.media_stack_bind.entity_id));
			goto exit;
		} else if (entity_ready(entity)) {
			os_log(LOG_ERR, "avdecc(%p) entity(%p): received GENAVB_MSG_MEDIA_STACK_ENTITY_START but entity already started\n",
				avdecc, entity);
			goto exit;
		}

		os_log(LOG_INFO, "avdecc(%p) entity(%p): received GENAVB_MSG_MEDIA_STACK_ENTITY_START\n",
			avdecc, entity);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_MEDIA_STACK_START)) {
			entity->channel_openmask |= AVDECC_WAITMASK_MEDIA_STACK_START;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	case GENAVB_MSG_MEDIA_STACK_BIND:
		if (desc->len != sizeof(struct ipc_media_stack_bind)) {
			os_log(LOG_ERR, "avdecc(%p) wrong length received %u expected %zu\n", avdecc, desc->len, sizeof(struct ipc_media_stack_bind));
			goto exit;
		}

		if (!avdecc->milan_mode) {
			os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_BIND supported only in Milan mode\n", avdecc);
			goto exit;
		}

		entity = avdecc_get_entity_raw(avdecc, htonll(desc->u.media_stack_bind.entity_id));

		if (entity) {
			if (entity_ready(entity)) {
				os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_BIND is not accepted after entity(%p) is started, ignored\n", avdecc, entity);
				goto exit;
			}

			os_log(LOG_INFO, "avdecc(%p) entity(%p): received GENAVB_MSG_MEDIA_STACK_BIND: Controller (%016"PRIx64") bound listener "
					"stream (%016"PRIx64", %u, %s) to talker stream (%016"PRIx64", %u) \n",
					avdecc, entity, desc->u.media_stack_bind.controller_entity_id,
					desc->u.media_stack_bind.entity_id, desc->u.media_stack_bind.listener_stream_index,
					(desc->u.media_stack_bind.started == ACMP_LISTENER_STREAM_STARTED) ? "STARTED" : "STOPPED",
					desc->u.media_stack_bind.talker_entity_id, desc->u.media_stack_bind.talker_stream_index);

			acmp_milan_listener_sink_rcv_binding_params(entity, &desc->u.media_stack_bind);
		} else {
			os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_BIND for unknown entity(%016"PRIx64")\n", avdecc, desc->u.media_stack_bind.entity_id);
			goto exit;
		}

		break;

	case GENAVB_MSG_MEDIA_STACK_PERSISTENT_PARAM:
		if (desc->len != sizeof(struct ipc_media_stack_persistent_param)) {
			os_log(LOG_ERR, "avdecc(%p) wrong length received %u expected %zu\n", avdecc, desc->len, sizeof(struct ipc_media_stack_persistent_param));
			goto exit;
		}

		entity = avdecc_get_entity_raw(avdecc, desc->u.media_stack_persistent_param.entity_id);
		if (entity) {
			if (entity_ready(entity)) {
				os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_PERSISTENT_PARAM is not accepted after entity(%p) is started, ignored\n",
					avdecc, entity);
				goto exit;
			}

			os_log(LOG_INFO, "avdecc(%p): received GENAVB_MSG_MEDIA_STACK_PERSISTENT_PARAM: Entity (%016"PRIx64") "
					"Descriptor Type %u, Descriptor Index %u, Parameter Type %u\n",
					avdecc, ntohll(desc->u.media_stack_persistent_param.entity_id), desc->u.media_stack_persistent_param.desc_type,
					desc->u.media_stack_persistent_param.desc_index, desc->u.media_stack_persistent_param.param_type);

			if (aecp_update_persistent_param(entity, &desc->u.media_stack_persistent_param) < 0) {
				os_log(LOG_ERR, "avdecc(%p) couldn't update persistent parameter on descriptor (%u, %u)\n", avdecc,
					desc->u.media_stack_persistent_param.desc_type, desc->u.media_stack_persistent_param.desc_index);
				goto exit;
			}
		} else {
			os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_PERSISTENT_PARAM for unknown entity(%016"PRIx64")\n",
				avdecc, ntohll(desc->u.media_stack_persistent_param.entity_id));
			goto exit;
		}

		break;

	case GENAVB_MSG_MEDIA_STACK_MEMORY_OBJECT:
		if (desc->len != sizeof(struct ipc_media_stack_memory_object)) {
			os_log(LOG_ERR, "avdecc(%p) wrong length received %u expected %zu\n", avdecc, desc->len, sizeof(struct ipc_media_stack_memory_object));
			goto exit;
		}

		if (!desc->u.media_stack_memory_object.entity_id) {
			/* A zero entity ID will return the first non-controller entity.
			 * Only a single non-controller entity is supported anyway (for now).
			 */
			entity = avdecc_get_local_controlled_any(avdecc);

		} else {
			entity = avdecc_get_entity_raw(avdecc, desc->u.media_stack_memory_object.entity_id);
		}

		if (entity) {
			if (entity_ready(entity)) {
				os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_MEMORY_OBJECT is not accepted after entity(%p) is started, ignored\n",
					avdecc, entity);
				goto exit;
			}

			os_log(LOG_INFO, "avdecc(%p): received GENAVB_MSG_MEDIA_STACK_MEMORY_OBJECT: Entity (%016"PRIx64") "
					"Configuration Index %u, Descriptor Index %u, Type %u, Length %"PRIu64", Max Length %"PRIu64"\n",
					avdecc, ntohll(desc->u.media_stack_memory_object.entity_id), desc->u.media_stack_memory_object.desc_attr.cfg_idx,
					desc->u.media_stack_memory_object.desc_attr.desc_idx, desc->u.media_stack_memory_object.desc_attr.object_type,
					desc->u.media_stack_memory_object.desc_attr.len, desc->u.media_stack_memory_object.desc_attr.max_len);

			if (aecp_update_memory_object(entity, &desc->u.media_stack_memory_object.desc_attr) < 0) {
				os_log(LOG_ERR, "avdecc(%p) couldn't update memory object on descriptor (%u, %u)\n", avdecc,
					desc->u.media_stack_memory_object.desc_attr.object_type, desc->u.media_stack_memory_object.desc_attr.desc_idx);
				goto exit;
			}
		} else {
			os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_MEMORY_OBJECT for unknown entity(%016"PRIx64")\n",
				avdecc, ntohll(desc->u.media_stack_memory_object.entity_id));
			goto exit;
		}
		break;

	default:
		os_log(LOG_ERR, "Received unknown message type %d on a MEDIA_STACK channel\n", desc->type);
		break;
	}

exit:
	ipc_free(rx, desc);
}

void avdecc_ipc_rx_srp(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_port *port = container_of(rx, struct avdecc_port, ipc_rx_srp);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct ipc_msrp_listener_status *listener_status;
	struct ipc_msrp_talker_status *talker_status;
	struct ipc_msrp_listener_declaration_status *listener_declaration_status;
	struct ipc_msrp_talker_declaration_status *talker_declaration_status;
	u64 stream_id;
	struct entity *entity;

	os_log(LOG_DEBUG, "avdecc(%p) port(%u) received IPC type: %d\n", avdecc, port->port_id, desc->type);

	switch (desc->type) {
	case GENAVB_MSG_LISTENER_STATUS:

		listener_status = &desc->u.msrp_listener_status;
		stream_id = get_64(listener_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) Received listener status %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, listener_stream_status2string(listener_status->status), ntohll(stream_id));

		if (avdecc->milan_mode) {
			u16 listener_unique_id;

			entity = avdecc_get_local_listener(avdecc, port->port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local listener entity supporting this port.\n",
					avdecc, port->port_id);
				goto exit;
			}

			if (!acmp_milan_get_listener_unique_id(entity, stream_id, &listener_unique_id))
				acmp_milan_listener_srp_state_sm(entity, listener_unique_id, listener_status);
		}

		break;

	case GENAVB_MSG_TALKER_STATUS:
		talker_status = &desc->u.msrp_talker_status;
		stream_id = get_64(talker_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) Received talker status %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, talker_stream_status2string(talker_status->status), ntohll(stream_id));

		if (avdecc->milan_mode) {
			u16 talker_unique_id;

			entity = avdecc_get_local_talker(avdecc, port->port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local talker entity supporting this port.\n",
					avdecc, port->port_id);
				goto exit;
			}

			if (!acmp_milan_get_talker_unique_id(entity, stream_id, &talker_unique_id))
				acmp_milan_talker_update_status(entity, talker_unique_id, talker_status);
		}
		break;

	case GENAVB_MSG_TALKER_DECLARATION_STATUS:
		talker_declaration_status = &desc->u.msrp_talker_declaration_status;
		stream_id = get_64(talker_declaration_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) talker declaration %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, talker_stream_declaration_type2string(talker_declaration_status->declaration_type), ntohll(stream_id));

		if (avdecc->milan_mode) {
			u16 talker_unique_id;

			entity = avdecc_get_local_talker(avdecc, port->port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local talker entity supporting this port.\n",
					avdecc, port->port_id);
				goto exit;
			}

			if (!acmp_milan_get_talker_unique_id(entity, stream_id, &talker_unique_id))
				acmp_milan_talker_update_declaration(entity, talker_unique_id, talker_declaration_status);
		}
		break;

	case GENAVB_MSG_LISTENER_DECLARATION_STATUS:
		listener_declaration_status = &desc->u.msrp_listener_declaration_status;
		stream_id = get_64(listener_declaration_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) listener declaration %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, listener_stream_declaration_type2string(listener_declaration_status->declaration_type), ntohll(stream_id));

		break;

	case GENAVB_MSG_TALKER_RESPONSE:
	case GENAVB_MSG_LISTENER_RESPONSE:
		break;

	default:
		os_log(LOG_ERR, "avdecc(%p) port(%u) received unknown message type %d on MSRP channel\n", avdecc, port->port_id, desc->type);
		break;
	}

exit:
	ipc_free(rx, desc);
}

static void avdecc_ipc_get_mac_status(struct avdecc_ctx *avdecc, struct ipc_tx *ipc, unsigned int port_id)
{
	struct ipc_desc *desc;
	struct ipc_mac_service_get_status *get_status;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_mac_service_get_status));
	if (desc) {
		desc->type = IPC_MAC_SERVICE_GET_STATUS;
		desc->len = sizeof(struct ipc_mac_service_get_status);
		desc->flags = 0;

		get_status = &desc->u.mac_service_get_status;

		get_status->port_id = port_id;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void avdecc_ipc_rx_mac_service(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_port *port = container_of(rx, struct avdecc_port, ipc_rx_mac_service);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct ipc_mac_service_status *status;
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	int i;

	switch (desc->type) {
	case IPC_MAC_SERVICE_STATUS:
		status = &desc->u.mac_service_status;

		os_log(LOG_DEBUG, "IPC_MAC_SERVICE_STATUS: logical_port(%u) operational(%u): received on avdecc port (%u)\n",
			status->port_id, status->operational, port->port_id);

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);
			if (!avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			if (!avb_itf_dynamic->operational_state && status->operational) {
				avb_itf_dynamic->diagnostic_counters.link_up++;

				if (entity_ready(entity)) {
					if (avdecc->milan_mode)
						adp_milan_advertise_sm(entity, port->port_id, ADP_MILAN_ADV_LINK_UP);
					else
						adp_ieee_advertise_interface_sm(entity, port->port_id, ADP_INTERFACE_ADV_EVENT_LINK_UP);

					aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id);
				}

			} else if (avb_itf_dynamic->operational_state && !status->operational) {
				avb_itf_dynamic->diagnostic_counters.link_down++;

				if (entity_ready(entity)) {
					if (avdecc->milan_mode)
						adp_milan_advertise_sm(entity, port->port_id, ADP_MILAN_ADV_LINK_DOWN);
					else
						adp_ieee_advertise_interface_sm(entity, port->port_id, ADP_INTERFACE_ADV_EVENT_LINK_DOWN);

					aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id);
				}
			}

			/* Save the current state */
			avb_itf_dynamic->operational_state = (status->operational) ? true : false;
		}

		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

/** Sends an AVDECC IPC message to the apps to handle a clock source change for a given clock domain.
 * \param	avdecc		pointer to the AVDECC context
 * \param	ipc		pointer to the IPC channel
 * \param	domain		clock domain id and descriptor index
 * \param	source_type	clock source type (GENAVB_CLOCK_SOURCE_TYPE_INTERNAL or GENAVB_CLOCK_SOURCE_TYPE_INPUT_SET)
 * \param	local_id	clock source id in case of an INTERNAL clock source
 * \param	set_id		set id in case of a SET_INPUT clock source
 */
void avdecc_ipc_set_clock_source(struct avdecc_ctx *avdecc, struct ipc_tx *ipc, u16 domain, genavb_clock_source_type_t source_type, u16 local_id, u16 set_id)
{
	struct ipc_clock_domain_set_source *set_clock_source;
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_clock_domain_set_source));
	if (desc) {
		desc->type = GENAVB_MSG_MEDIA_SET_CLOCK_SOURCE;
		desc->len = sizeof(struct ipc_clock_domain_set_source);
		desc->flags = 0;

		set_clock_source = &desc->u.clock_domain_set_source;

		os_memset(set_clock_source, 0, sizeof(struct ipc_clock_domain_set_source));

		set_clock_source->domain = domain + GENAVB_CLOCK_DOMAIN_0;
		set_clock_source->source_type = source_type;

		if (source_type == GENAVB_CLOCK_SOURCE_TYPE_INTERNAL) {
			/* FIXME: Only PTP_CLK is supported as internal clock for now. */
			set_clock_source->local_id = GENAVB_CLOCK_SOURCE_PTP_CLK;

		} else if (source_type == GENAVB_CLOCK_SOURCE_TYPE_INPUT_SET) {
			set_clock_source->set_id = set_id;

		} else {
			os_log(LOG_ERR, "Unsupported clock source type(%d)\n", source_type);
			ipc_free(ipc, desc);
		}

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(ipc, desc);
		}
	} else {
		os_log(LOG_ERR, "ipc_alloc() failed\n");
	}
}

/** Sends an AVDECC packet back to the stack.
 * Takes ownership of the TX descriptor in all cases.
 * \param	avdecc		pointer to the AVDECC context
 * \param	rx_desc		RX descriptor containing the packet to send
 * \param	local_mac	Pointer to local MAC address to set as source
 */
static void avdecc_net_tx_loopback(struct avdecc_port *port, struct net_rx_desc *rx_desc, u8 *local_mac)
{
	struct eth_hdr *eth = (struct eth_hdr *)NET_DATA_START(rx_desc);

	/* Make sure any response will come back locally as well. */
	os_memcpy(eth->src, local_mac, 6);

	rx_desc->l3_offset = rx_desc->l2_offset + sizeof(struct eth_hdr);

	avdecc_net_rx(&port->net_rx, rx_desc);
}

/** Sends an AVDECC packet on the network.
 * Takes ownership of the TX descriptor in all cases.
 * \return	0 on success, -1 in case of failure
 * \param	port		pointer to the AVDECC port on which the packet will be sent
 * \param	desc		TX descriptor containing the packet to send
 */
int avdecc_net_tx(struct avdecc_port *port, struct net_tx_desc *desc)
{
	struct net_tx_desc *tx_desc;
	struct eth_hdr *eth = (struct eth_hdr *)NET_DATA_START(desc);
	int rc = 0;

	os_log(LOG_DEBUG, "port (%u), l2_offset %x, len %d, flags %d\n",
		port->port_id, desc->l2_offset, desc->len, desc->flags);

	if (MAC_IS_MCAST(eth->dst)) {
		if (!(desc->flags & NET_TX_FLAGS_LOOPBACK_ONLY)) {
			tx_desc = net_tx_clone(&port->net_tx, desc);
			if (!tx_desc) {
				os_log(LOG_ERR, "port(%u) cannot alloc tx descriptor\n", port->port_id);
			} else {
				if (net_tx(&port->net_tx, tx_desc) < 0) {
					os_log(LOG_ERR, "port(%u) cannot transmit packet\n", port->port_id);
					net_tx_free(tx_desc);
				}
			}
		}

		avdecc_net_tx_loopback(port, (struct net_rx_desc *)desc, port->local_physical_mac);

	} else {
		if (os_memcmp(eth->dst, port->local_physical_mac, 6) == 0) {
			avdecc_net_tx_loopback(port, (struct net_rx_desc *)desc, port->local_physical_mac);
		} else {
			if (!(desc->flags & NET_TX_FLAGS_LOOPBACK_ONLY)) {
				if (net_tx(&port->net_tx, desc) < 0) {
					os_log(LOG_ERR, "port(%u) cannot transmit packet\n", port->port_id);
					net_tx_free(desc);
					rc = -1;
				}
			} else {
				/* Locally visible controllers don't send packets on the network.
				 * So free the unused net buffer, as no one takes ownership of it in this case.
				 */
				net_tx_free(desc);
			}
		}
	}

	if (!rc)
		os_log(LOG_DEBUG, "Success \n");

	return rc;
}

size_t avdecc_add_common_header(void *buf, u8 subtype, u8 msg_type, u16 length, u8 status)
{
	struct avtp_ctrl_hdr *avtp_ctrl = (struct avtp_ctrl_hdr *)buf;

	avtp_ctrl->subtype = subtype;
	avtp_ctrl->version = 0;
	avtp_ctrl->sv = 0;
	avtp_ctrl->control_data = msg_type;
	AVTP_SET_CTRL_DATA_STATUS(avtp_ctrl, status, length);

	return sizeof(struct avtp_ctrl_hdr);
}

/**
 * Get the avdecc port for a specific logical port number
 * \return			0 on success and the interface index set in the interface_index pointer, -1 otherwise.
 * \return			The avdecc port with the corresponding logical port id on success, NULL otherwise
 * \param avdecc		pointer to avdecc context
 * \param logical_port		logical port number
 */
struct avdecc_port *logical_to_avdecc_port(struct avdecc_ctx *avdecc, unsigned int logical_port)
{
	int i;

	for (i = 0; i < avdecc->port_max; i++) {
		if (logical_port == avdecc->port[i].logical_port) {
			if (avdecc->port[i].initialized)
				return &avdecc->port[i];
			else
				return NULL;
		}
	}

	return NULL;
}

/**
 * Get the logical port for a specific avdecc port (avb interface index)
 * \return			logical port id
 * \param avdecc		pointer to avdecc context
 * \param port_id		avdecc port id/interface index
 */
unsigned int avdecc_port_to_logical(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	return avdecc->port[port_id].logical_port;
}

/** Find the local entity matching the provided entity ID and return it if it exists.
 *  \return	pointer to the entity (if found), NULL otherwise.
 */
struct entity *avdecc_get_entity_raw(struct avdecc_ctx *avdecc, u64 entity_id)
{
	int i;

	for (i = 0; i < avdecc->num_entities; i++)
		if (avdecc->entities[i]->desc->entity_id == entity_id)
			return avdecc->entities[i];

	return NULL;
}

/** Find the local entity matching the provided entity ID and return it if it is ready.
 *  \return	pointer to the entity (if found and ready), NULL otherwise.
 */
struct entity *avdecc_get_entity(struct avdecc_ctx *avdecc, u64 entity_id)
{
	struct entity *entity;

	entity = avdecc_get_entity_raw(avdecc, entity_id);

	if (entity && entity_ready(entity))
		return entity;

	return NULL;
}

/** Checks if the provided interface index is valid for the AVDECC entity.
 * \return		True if the interface index is enabled, False otherwise
 * \param entity	Pointer to entity
 * \param port_id	avdecc port id/interface index
 */
bool avdecc_entity_port_valid(struct entity *entity, unsigned int port_id)
{
	if (aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL))
		return true;
	else
		return false;
}

__init static int avdecc_port_ipc_init(struct avdecc_port *port, unsigned long priv)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	if (avdecc->use_gptp_bridge_stack) {
		/* No gptp endpoint stack is available: use the bridge gPTP stack for GM status events and managed objects fetch. */
		if (ipc_rx_init(&port->ipc_rx_gptp, IPC_GPTP_BRIDGE_MEDIA_STACK,
				&avdecc_ipc_rx_gptp, priv) < 0)
			goto err_ipc_rx_gptp;

		if (ipc_tx_init(&port->ipc_tx_gptp, IPC_MEDIA_STACK_GPTP_BRIDGE) < 0)
			goto err_ipc_tx_gptp;
	} else {
		if (ipc_rx_init(&port->ipc_rx_gptp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_GPTP_MEDIA_STACK : IPC_GPTP_1_MEDIA_STACK,
				&avdecc_ipc_rx_gptp, priv) < 0)
			goto err_ipc_rx_gptp;

		if (ipc_tx_init(&port->ipc_tx_gptp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_GPTP : IPC_MEDIA_STACK_GPTP_1) < 0)
			goto err_ipc_tx_gptp;
	}

	if (ipc_tx_connect(&port->ipc_tx_gptp, &port->ipc_rx_gptp) < 0)
		goto err_ipc_tx_gptp_connect;

	if (avdecc->srp_enabled) {
		if (ipc_tx_init(&port->ipc_tx_srp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_MSRP : IPC_MEDIA_STACK_MSRP_1) < 0)
			goto err_ipc_tx_srp;

		if (ipc_rx_init(&port->ipc_rx_srp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MSRP_MEDIA_STACK : IPC_MSRP_1_MEDIA_STACK, &avdecc_ipc_rx_srp, priv) < 0)
			goto err_ipc_rx_srp;

		if (ipc_tx_connect(&port->ipc_tx_srp, &port->ipc_rx_srp) < 0)
			goto err_ipc_tx_srp_connect;
	}

	if (avdecc->management_enabled) {
		if (ipc_rx_init(&port->ipc_rx_mac_service, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MAC_SERVICE_MEDIA_STACK : IPC_MAC_SERVICE_1_MEDIA_STACK,
					&avdecc_ipc_rx_mac_service, priv) < 0)
			goto err_ipc_rx_mac_service;

		if (ipc_tx_init(&port->ipc_tx_mac_service, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_MAC_SERVICE : IPC_MEDIA_STACK_MAC_SERVICE_1) < 0)
			goto err_ipc_tx_mac_service;

		if (ipc_tx_connect(&port->ipc_tx_mac_service, &port->ipc_rx_mac_service) < 0)
			goto err_ipc_tx_mac_connect;

		avdecc_ipc_get_mac_status(avdecc, &port->ipc_tx_mac_service, port->port_id);
	}

	return 0;

err_ipc_tx_mac_connect:
	if (avdecc->management_enabled)
		ipc_tx_exit(&port->ipc_tx_mac_service);
err_ipc_tx_mac_service:
	if (avdecc->management_enabled)
		ipc_rx_exit(&port->ipc_rx_mac_service);
err_ipc_rx_mac_service:
err_ipc_tx_srp_connect:
	if (avdecc->srp_enabled)
		ipc_rx_exit(&port->ipc_rx_srp);

err_ipc_rx_srp:
	if (avdecc->srp_enabled)
		ipc_tx_exit(&port->ipc_tx_srp);

err_ipc_tx_srp:
err_ipc_tx_gptp_connect:
	ipc_tx_exit(&port->ipc_tx_gptp);

err_ipc_tx_gptp:
	ipc_rx_exit(&port->ipc_rx_gptp);

err_ipc_rx_gptp:
	return -1;
}

void avdecc_counters_polling_timer_handler(void *data)
{
	struct avdecc_ctx *avdecc = (struct avdecc_ctx *)data;

	avdecc_ipc_avtp_get_counters(avdecc);

	timer_restart(&avdecc->counters_polling_timeout, AVTP_COUNTERS_POLLING_TIMEOUT);
}

__init static int avdecc_timers_init(struct avdecc_ctx *avdecc)
{
	/* AVTP counters polling timer */
	avdecc->counters_polling_timeout.func = &avdecc_counters_polling_timer_handler;
	avdecc->counters_polling_timeout.data = avdecc;
	if (timer_create(avdecc->timer_ctx, &avdecc->counters_polling_timeout, 0, AVTP_COUNTERS_POLLING_TIMER_GRANULARITY) < 0) {
		os_log(LOG_ERR, "avdecc(%p), AVTP counters_polling_timeout timer failed\n", avdecc);
		goto err_timer_avtp_counters;
	}

	timer_start(&avdecc->counters_polling_timeout, AVTP_COUNTERS_POLLING_TIMEOUT);

	return 0;

err_timer_avtp_counters:
	return -1;
}

__init static int avdecc_ports_init(struct avdecc_ctx *avdecc, struct avdecc_config *cfg, unsigned long priv)
{
	unsigned int i;
	struct net_address addr;
	unsigned int logical_port;

	for (i = 0; i < avdecc->port_max; i++) {
		logical_port = cfg->logical_port_list[i];

		avdecc->port[i].port_id = i;
		avdecc->port[i].logical_port = logical_port;
		avdecc->port[i].initialized = false;

		if (net_get_local_addr(logical_port, avdecc->port[i].local_physical_mac) < 0) {
			os_log(LOG_ERR, "avdecc(%p) could not get local physical mac address on port(%u)\n", avdecc, i);
			goto err_local_addr;
		}

		/**
		 * Network Rx/Tx
		 */
		addr.ptype = PTYPE_AVTP;
		addr.port = logical_port;
		addr.priority = AVDECC_DEFAULT_PRIORITY;
		addr.u.avtp.subtype = AVTP_SUBTYPE_AVDECC;

		if (net_rx_init(&avdecc->port[i].net_rx, &addr, &avdecc_net_rx, priv) < 0)
			goto err_net_rx_init;

		if (net_tx_init(&avdecc->port[i].net_tx, &addr) < 0)
			goto err_net_tx_init;

		if (avdecc_port_ipc_init(&avdecc->port[i], priv) < 0)
			goto err_ipc_init;

		avdecc->port[i].initialized = true;

		os_log(LOG_INIT, "avdecc(%p) - port(%u)(%p) / max %d done\n",
			avdecc, avdecc->port[i].port_id, &avdecc->port[i], avdecc->port_max);

		continue;

	err_ipc_init:
		net_tx_exit(&avdecc->port[i].net_tx);
	err_net_tx_init:
		net_rx_exit(&avdecc->port[i].net_rx);

	err_net_rx_init:
	err_local_addr:
		continue;
	}

	return 0;
}

__init static struct avdecc_ctx *avdecc_alloc(unsigned int timer_n, unsigned int n_ports, unsigned int max_entities_discovery)
{
	struct avdecc_ctx *avdecc;
	unsigned int size;

	size = sizeof(struct avdecc_ctx) + n_ports * sizeof(struct avdecc_port);
	size += timer_pool_size(timer_n);
	size += adp_discovery_data_size(max_entities_discovery) * n_ports;

	avdecc = os_malloc(size);
	if (!avdecc)
		goto err;

	os_memset(avdecc, 0, size);

	avdecc->timer_ctx = (struct timer_ctx *)((u8 *)(avdecc + 1) + n_ports * sizeof(struct avdecc_port));
	avdecc->adp_discovery_data = (void *)((u8 *)avdecc->timer_ctx + timer_pool_size(timer_n));
	avdecc->port_max = n_ports;

	return avdecc;

err:
	return NULL;
}

__exit static void avdecc_port_ipc_exit(struct avdecc_port *port)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	if (avdecc->management_enabled) {
		ipc_tx_exit(&port->ipc_tx_mac_service);
		ipc_rx_exit(&port->ipc_rx_mac_service);
	}

	if (avdecc->srp_enabled) {
		ipc_rx_exit(&port->ipc_rx_srp);
		ipc_tx_exit(&port->ipc_tx_srp);
	}

	ipc_tx_exit(&port->ipc_tx_gptp);

	ipc_rx_exit(&port->ipc_rx_gptp);
}

__exit static void avdecc_ports_exit(struct avdecc_ctx *avdecc)
{
	unsigned int i;

	for (i = 0; i < avdecc->port_max; i++) {
		if (!avdecc->port[i].initialized)
			continue;

		net_tx_exit(&avdecc->port[i].net_tx);
		net_rx_exit(&avdecc->port[i].net_rx);

		avdecc_port_ipc_exit(&avdecc->port[i]);

		avdecc->port[i].initialized = false;
	}

	os_log(LOG_INIT, "done\n");
}

__init void *avdecc_init(struct avdecc_config *cfg, unsigned long priv)
{
	struct avdecc_ctx *avdecc;
	unsigned int timer_n;
	u8 *adp_discovery_data;
	int i = 0, j, k = 0;

	if (cfg->num_entities == 0)
		cfg->num_entities = 1;

	timer_n = cfg->port_max * cfg->num_entities * CFG_AVDECC_MAX_TIMERS_PER_ENTITY;

	avdecc = avdecc_alloc(timer_n, cfg->port_max, cfg->max_entities_discovery);
	if (!avdecc)
		goto err_malloc;

	log_level_set(avdecc_COMPONENT_ID, cfg->log_level);

	avdecc->srp_enabled = cfg->srp_enabled;
	avdecc->management_enabled = cfg->management_enabled;
	avdecc->milan_mode = cfg->milan_mode;
	avdecc->use_gptp_bridge_stack = cfg->use_gptp_bridge_stack;

	if (avdecc_ports_init(avdecc, cfg, priv) < 0)
		goto err_ports;

	/**
	 * IPC
	 */
	if (ipc_rx_init(&avdecc->ipc_rx_controller, IPC_CONTROLLER_AVDECC, &avdecc_ipc_rx_controller, priv) < 0)
		goto err_ipc_rx_controller;

	if (ipc_rx_init(&avdecc->ipc_rx_controlled, IPC_CONTROLLED_AVDECC, &avdecc_ipc_rx_controlled, priv) < 0)
		goto err_ipc_rx_controlled;

	if (ipc_rx_init(&avdecc->ipc_rx_media_stack, IPC_MEDIA_STACK_AVDECC, &avdecc_ipc_rx_media_stack, priv) < 0)
		goto err_ipc_rx_media_stack;

	if (ipc_tx_init(&avdecc->ipc_tx_media_stack, IPC_AVDECC_MEDIA_STACK) < 0)
		goto err_ipc_tx_media_stack;

	if (ipc_tx_init(&avdecc->ipc_tx_maap, IPC_MEDIA_STACK_MAAP) < 0)
		goto err_ipc_tx_maap;

	if (ipc_rx_init(&avdecc->ipc_rx_maap, IPC_MAAP_MEDIA_STACK, &avdecc_ipc_rx_maap, priv) < 0)
		goto err_ipc_rx_maap;

	if (ipc_tx_connect(&avdecc->ipc_tx_maap, &avdecc->ipc_rx_maap) < 0)
		goto err_ipc_tx_maap_connect;

	if (ipc_tx_init(&avdecc->ipc_tx_controlled, IPC_AVDECC_CONTROLLED) < 0)
		goto err_ipc_tx_controlled;

	if (ipc_tx_init(&avdecc->ipc_tx_controller, IPC_AVDECC_CONTROLLER) < 0)
		goto err_ipc_tx_controller;

	if (ipc_tx_init(&avdecc->ipc_tx_controller_sync, IPC_AVDECC_CONTROLLER_SYNC) < 0)
		goto err_ipc_tx_controller_sync;

	if (ipc_tx_init(&avdecc->ipc_tx_avtp, IPC_AVDECC_AVTP) < 0)
		goto err_ipc_tx_avtp;

	if (ipc_rx_init(&avdecc->ipc_rx_avtp, IPC_AVTP_AVDECC, &avdecc_ipc_rx_avtp, priv) < 0)
		goto err_ipc_rx_avtp;

	if (ipc_rx_init(&avdecc->ipc_rx_clock_domain, IPC_CLOCK_DOMAIN_MEDIA_STACK, &avdecc_ipc_rx_clock_domain, priv) < 0)
		goto err_ipc_rx_clock_domain;

	/**
	 * Timers
	 */
	if (timer_pool_init(avdecc->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	avdecc->num_entities = cfg->num_entities;

	for (i = 0; i < avdecc->num_entities; i++) {
		avdecc->entities[i] = avdecc_entity_init(avdecc, i, &cfg->entity_cfg[i]);
		if (!avdecc->entities[i])
			goto err_entity_init;
	}

	for (k = 0; k < avdecc->port_max; k++) {
		adp_discovery_data = (u8 *)avdecc->adp_discovery_data + k * adp_discovery_data_size(cfg->max_entities_discovery);

		if (adp_discovery_init(&avdecc->port[k].discovery, adp_discovery_data, cfg) < 0)
			goto err_discovery_init;
	}

	for (j = 0; j < avdecc->port_max; j++) {
		avdecc_ipc_managed_get_default_ds(&avdecc->port[j]);

		avdecc_ipc_gm_get_status(&avdecc->port[j]);
	}

	if (avdecc->srp_enabled) {
		for (j = 0; j < avdecc->port_max; j++)
			avdecc_ipc_srp_deregister_all(&avdecc->port[j]);
	}

	if (avdecc_timers_init(avdecc) < 0)
		goto err_timer_init;

	os_log(LOG_INIT, "avdecc(%p) done, loaded %d entities.\n", avdecc, avdecc->num_entities);

	return avdecc;

err_timer_init:
err_discovery_init:
	for (j = 0; j < k; j++)
		adp_discovery_exit(&avdecc->port[j].discovery);

err_entity_init:
	for (j = 0; j < i; j++)
		avdecc_entity_exit(avdecc->entities[j]);

	timer_pool_exit(avdecc->timer_ctx);

err_timer_pool_init:
	ipc_rx_exit(&avdecc->ipc_rx_clock_domain);

err_ipc_rx_clock_domain:
	ipc_rx_exit(&avdecc->ipc_rx_avtp);

err_ipc_rx_avtp:
	ipc_tx_exit(&avdecc->ipc_tx_avtp);

err_ipc_tx_avtp:
	ipc_tx_exit(&avdecc->ipc_tx_controller_sync);

err_ipc_tx_controller_sync:
	ipc_tx_exit(&avdecc->ipc_tx_controller);

err_ipc_tx_controller:
	ipc_tx_exit(&avdecc->ipc_tx_controlled);

err_ipc_tx_controlled:
err_ipc_tx_maap_connect:
	ipc_rx_exit(&avdecc->ipc_rx_maap);
err_ipc_rx_maap:
	ipc_tx_exit(&avdecc->ipc_tx_maap);

err_ipc_tx_maap:
	ipc_tx_exit(&avdecc->ipc_tx_media_stack);

err_ipc_tx_media_stack:
	ipc_rx_exit(&avdecc->ipc_rx_media_stack);

err_ipc_rx_media_stack:
	ipc_rx_exit(&avdecc->ipc_rx_controlled);

err_ipc_rx_controlled:
	ipc_rx_exit(&avdecc->ipc_rx_controller);

err_ipc_rx_controller:
	avdecc_ports_exit(avdecc);

err_ports:
	os_free(avdecc);

err_malloc:
	return NULL;
}

__exit int avdecc_exit(void *avdecc_h)
{
	struct avdecc_ctx *avdecc = (struct avdecc_ctx *)avdecc_h;
	int i;

	timer_destroy(&avdecc->counters_polling_timeout);

	/* De-init entities in reverse order while decrementing the num_entities
	 * counter to protect against access to the freed entity (on avdecc_net_tx_loopback())
	 */
	while (avdecc->num_entities-- > 0) {
		avdecc_entity_exit(avdecc->entities[avdecc->num_entities]);
	}

	/* avdecc_entity_exit() can generate PDUs (in loopback), so de-init the
	 * discovery context after that.
	 */
	for (i = 0; i < avdecc->port_max; i++) {
		if (!avdecc->port[i].initialized)
			continue;

		adp_discovery_exit(&avdecc->port[i].discovery);
	}

	timer_pool_exit(avdecc->timer_ctx);

	ipc_tx_exit(&avdecc->ipc_tx_controller_sync);

	ipc_tx_exit(&avdecc->ipc_tx_controller);

	ipc_tx_exit(&avdecc->ipc_tx_controlled);

	ipc_rx_exit(&avdecc->ipc_rx_maap);

	ipc_tx_exit(&avdecc->ipc_tx_maap);

	ipc_tx_exit(&avdecc->ipc_tx_media_stack);

	ipc_rx_exit(&avdecc->ipc_rx_media_stack);

	ipc_rx_exit(&avdecc->ipc_rx_controlled);

	ipc_rx_exit(&avdecc->ipc_rx_controller);

	ipc_rx_exit(&avdecc->ipc_rx_avtp);

	ipc_tx_exit(&avdecc->ipc_tx_avtp);

	ipc_rx_exit(&avdecc->ipc_rx_clock_domain);

	avdecc_ports_exit(avdecc);

	os_log(LOG_INIT, "avdecc(%p)\n", avdecc);

	os_free(avdecc_h);

	return 0;
}
