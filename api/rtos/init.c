/*
 * Copyright 2018-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file init.c
 \brief GenAVB public API initialization for rtos
 \details API definition for the GenAVB library
*/
#include <string.h>

#include "api/config.h"

#include "api/init.h"

#include "common/log.h"
#include "common/version.h"
#include "common/fqtss.h"

#include "rtos/stats_task.h"
#include "rtos/net_task.h"
#include "rtos/ipc.h"
#include "rtos/media_queue.h"
#include "rtos/media_clock.h"
#include "rtos/net_port.h"
#include "rtos/net_port_netc_1588.h"
#include "rtos/hr_timer.h"
#include "rtos/ftm.h"
#include "rtos/gpt.h"
#include "rtos/tpm.h"
#include "rtos/stm.h"
#include "rtos/fqtss.h"
#include "rtos/clock.h"
#include "rtos/net_bridge.h"
#include "rtos/msgintr.h"

extern void os_random_init(void);

#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
extern void *management_task_init(struct management_config *cfg);
extern void management_task_exit(void *handle);
extern struct management_config management_default_config;
static struct management_config *management_current_config = &management_default_config;
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
extern void *gptp_task_init(struct fgptp_config *cfg);
extern void gptp_task_exit(void *handle);
extern struct fgptp_config gptp_default_config;
static struct fgptp_config *gptp_current_config = &gptp_default_config;
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
extern void *srp_task_init(struct srp_config *cfg);
extern void srp_task_exit(void *handle);
extern struct srp_config srp_default_config;
static struct srp_config *srp_current_config = &srp_default_config;
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
void streaming_exit(struct genavb_handle *genavb);
extern void *avtp_task_init(struct avtp_config *cfg);
extern void avtp_task_exit(void *task);
extern struct avtp_config avtp_default_config;
static struct avtp_config *avtp_current_config = &avtp_default_config;
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
extern void *avdecc_task_init(struct avdecc_config *cfg);
extern void avdecc_task_exit(void *handle);
extern struct avdecc_config avdecc_default_config;
static struct avdecc_config *avdecc_current_config = &avdecc_default_config;
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
extern void *maap_task_init(struct maap_config *cfg);
extern void maap_task_exit(void *handle);
extern struct maap_config maap_default_config;
static struct maap_config *maap_current_config = &maap_default_config;
#endif
#ifdef CONFIG_GENAVB_TSN_HSR
extern void *hsr_task_init(struct hsr_config *cfg);
extern void hsr_task_exit(void *handle);
extern struct hsr_config hsr_default_config;
static struct hsr_config *hsr_current_config = &hsr_default_config;
#endif

__init void genavb_get_default_config(struct genavb_config *config)
{
#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
	memcpy(&config->management_config, &management_default_config, sizeof(struct management_config));
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
	memcpy(&config->fgptp_config, &gptp_default_config, sizeof(struct fgptp_config));
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
	memcpy(&config->srp_config, &srp_default_config, sizeof(struct srp_config));
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	memcpy(&config->avtp_config, &avtp_default_config, sizeof(struct avtp_config));
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
	memcpy(&config->avdecc_config, &avdecc_default_config, sizeof(struct avdecc_config));
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
	memcpy(&config->maap_config, &maap_default_config, sizeof(struct maap_config));
#endif
#ifdef CONFIG_GENAVB_TSN_HSR
	memcpy(&config->hsr_config, &hsr_default_config, sizeof(struct hsr_config));
#endif
}

__init void genavb_set_config(struct genavb_config *config)
{
#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
	management_current_config = &config->management_config;
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
	gptp_current_config = &config->fgptp_config;
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
	srp_current_config = &config->srp_config;
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	avtp_current_config = &config->avtp_config;
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
	avdecc_current_config = &config->avdecc_config;
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
	maap_current_config = &config->maap_config;
#endif
#ifdef CONFIG_GENAVB_TSN_HSR
	hsr_current_config = &config->hsr_config;
#endif
}

__init static int osal_init(void)
{
	if (stats_task_init() < 0)
		goto err_stats;

	if (mclock_init() < 0)
		goto err_mclock;

	if (media_queue_init() < 0)
		goto err_media_queue;

	if (hw_timer_init() < 0)
		goto err_hw_timer;

	if (msgintr_init() < 0)
		goto err_msgintr;

	if (netc_1588_init() < 0)
		goto err_netc_1588;

	if (gpt_driver_init() < 0)
		goto err_gpt;

	if (tpm_driver_init() < 0)
		goto err_tpm;

	if (stm_driver_init() < 0)
		goto err_stm;

	if (ftm_driver_init() < 0)
		goto err_ftm;

	if (port_init() < 0)
		goto err_port;

	if (bridge_init() < 0)
		goto err_bridge;

	if (os_clock_init() < 0)
		goto err_clock;

	if (hr_timer_init() < 0)
		goto err_timer;

	if (net_task_init() < 0)
		goto err_net;

	if (port_post_init() < 0)
		goto err_port_post;

	if (ipc_init() < 0)
		goto err_ipc;

	if (fqtss_init() < 0)
		goto err_fqtss;

	/*
	 * random global init.
	 */
	os_random_init();

	return 0;

err_fqtss:
	ipc_exit();

err_ipc:
	port_pre_exit();

err_port_post:
	net_task_exit();

err_net:
	hr_timer_exit();

err_timer:
	os_clock_exit();

err_clock:
	bridge_exit();

err_bridge:
	port_exit();

err_port:
	ftm_driver_exit();

err_ftm:
	stm_driver_exit();

err_stm:
	tpm_driver_exit();

err_tpm:
	gpt_driver_exit();

err_gpt:
	netc_1588_exit();

err_netc_1588:
	msgintr_exit();

err_msgintr:
	hw_timer_exit();

err_hw_timer:
	media_queue_exit();

err_media_queue:
	mclock_exit();

err_mclock:
	stats_task_exit();

err_stats:

	return -1;
}

__exit static void osal_exit(void)
{
	fqtss_exit();
	ipc_exit();
	port_pre_exit();
	net_task_exit();
	hr_timer_exit();
	os_clock_exit();
	bridge_exit();
	port_exit();
	ftm_driver_exit();
	stm_driver_exit();
	tpm_driver_exit();
	gpt_driver_exit();
	netc_1588_exit();
	msgintr_exit();
	hw_timer_exit();
	media_queue_exit();
	mclock_exit();
	stats_task_exit();
}

__init int genavb_init(struct genavb_handle **genavb, unsigned int flags)
{
	struct genavb_handle *stack_handle;
	int rc = -GENAVB_ERR_NO_MEMORY;

	log_level_set(api_COMPONENT_ID, LOG_INIT);
	log_level_set(common_COMPONENT_ID, LOG_INIT);
	log_level_set(os_COMPONENT_ID, LOG_INFO);

	os_log(LOG_INIT, "NXP's GenAVB/TSN stack version %s\n", GENAVB_VERSION);

	stack_handle = rtos_malloc(sizeof(struct genavb_handle));
	if (!stack_handle) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto err_handle;
	}

	memset(stack_handle, 0, sizeof(struct genavb_handle));

	stack_handle->flags = flags;
	stack_handle->flags &= ~AVTP_INITIALIZED;

	if (osal_init() < 0)
		goto err_osal;

#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
	stack_handle->management_handle = management_task_init(management_current_config);
	if (!stack_handle->management_handle)
		goto err_management;
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
	stack_handle->gptp_handle = gptp_task_init(gptp_current_config);
	if (!stack_handle->gptp_handle)
		goto err_gptp;
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
	if (common_fqtss_init() < 0)
		goto err_fqtss_init;

	stack_handle->srp_handle = srp_task_init(srp_current_config);
	if (!stack_handle->srp_handle)
		goto err_srp;
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	stack_handle->avtp_handle = avtp_task_init(avtp_current_config);
	if (!stack_handle->avtp_handle)
		goto err_avtp;
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
	stack_handle->avdecc_handle = avdecc_task_init(avdecc_current_config);
	if (!stack_handle->avdecc_handle)
		goto err_avdecc;
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
	stack_handle->maap_handle = maap_task_init(maap_current_config);
	if (!stack_handle->maap_handle)
		goto err_maap;
#endif
#ifdef CONFIG_GENAVB_TSN_HSR
	stack_handle->hsr_handle = hsr_task_init(hsr_current_config);
	if (!stack_handle->hsr_handle)
		goto err_hsr;
#endif

	*genavb = stack_handle;

	return GENAVB_SUCCESS;

#ifdef CONFIG_GENAVB_TSN_HSR
err_hsr:
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
	maap_task_exit(stack_handle->maap_handle);
err_maap:
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
	avdecc_task_exit(stack_handle->avdecc_handle);
err_avdecc:
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	avtp_task_exit(stack_handle->avtp_handle);
err_avtp:
#endif
#if defined(CONFIG_GENAVB_TSN_SRP) && (defined(CONFIG_GENAVB_TSN_AVTP) || defined(CONFIG_GENAVB_TSN_AVDECC) || defined(CONFIG_GENAVB_TSN_MAAP) || defined(CONFIG_GENAVB_TSN_HSR))
	srp_task_exit(stack_handle->srp_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
err_srp:
	common_fqtss_exit();
err_fqtss_init:
#endif
#if defined(CONFIG_GENAVB_TSN_GPTP) && (defined(CONFIG_GENAVB_TSN_SRP) || defined(CONFIG_GENAVB_TSN_AVTP) || defined(CONFIG_GENAVB_TSN_AVDECC) || defined(CONFIG_GENAVB_TSN_MAAP) || defined(CONFIG_GENAVB_TSN_HSR))
	gptp_task_exit(stack_handle->gptp_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
err_gptp:
#endif
#if ((defined(CONFIG_GENAVB_TSN_MANAGEMENT) && defined(CONFIG_GENAVB_TSN_GPTP)) || defined(CONFIG_GENAVB_TSN_SRP) || defined(CONFIG_GENAVB_TSN_AVTP) || defined(CONFIG_GENAVB_TSN_AVDECC) || defined(CONFIG_GENAVB_TSN_MAAP) || defined(CONFIG_GENAVB_TSN_HSR))
	management_task_exit(stack_handle->management_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
err_management:
#endif
	osal_exit();

err_osal:
	rtos_free(stack_handle);

err_handle:
	*genavb = NULL;

	return rc;
}

__exit int genavb_exit(struct genavb_handle *genavb)
{
	if (!genavb)
		goto err;

#ifdef CONFIG_GENAVB_TSN_HSR
	hsr_task_exit(genavb->hsr_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	streaming_exit(genavb);
#endif
#ifdef CONFIG_GENAVB_TSN_AVDECC
	avdecc_task_exit(genavb->avdecc_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_AVTP
	avtp_task_exit(genavb->avtp_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_SRP
	srp_task_exit(genavb->srp_handle);

	common_fqtss_exit();
#endif
#ifdef CONFIG_GENAVB_TSN_GPTP
	gptp_task_exit(genavb->gptp_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_MANAGEMENT
	management_task_exit(genavb->management_handle);
#endif
#ifdef CONFIG_GENAVB_TSN_MAAP
	maap_task_exit(genavb->maap_handle);
#endif

	osal_exit();
	rtos_free(genavb);

	return GENAVB_SUCCESS;

err:
	return -GENAVB_ERR_INVALID;
}
