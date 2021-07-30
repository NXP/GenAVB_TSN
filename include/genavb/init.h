/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    Neither the name of NXP Semiconductors nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 \file init.h
 \brief GenAVB public initialization API
 \details initialization API definition for the GenAVB library

 \copyright Copyright 2018 NXP
*/

#ifndef _GENAVB_PUBLIC_INIT_API_H_
#define _GENAVB_PUBLIC_INIT_API_H_

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "config.h"

#define AVDECC_FAST_CONNECT_MODE		(1 << 0) /* Global fast-connect mode (includes saving a connection state) */
#define AVDECC_FAST_CONNECT_BTB			(1 << 1) /* Global back to back mode, enables connecting to the first talker appearing on the network (demo purpose) */

/* Runtime configurations */
#define ACMP_CFG_MAX_UNIQUE_ID		8

#define AVDECC_WAITMASK_MEDIA_STACK	(1 << 0)
#define AVDECC_WAITMASK_CONTROLLER	(1 << 1)
#define AVDECC_WAITMASK_CONTROLLED	(1 << 2)

/**
 * \cond FREERTOS
 */

/**
 * \ingroup init
 * AVDECC entity configuration
 */
struct avdecc_entity_config {
	unsigned int flags;
	unsigned int channel_waitmask;
	uint64_t entity_id;
	uint64_t association_id;				/* Association ID to use for the entity */
	/* Fast-connect remote information */
	uint64_t talker_entity_id[ACMP_CFG_MAX_UNIQUE_ID];
	unsigned int talker_entity_id_n;
	uint16_t talker_unique_id[ACMP_CFG_MAX_UNIQUE_ID];
	unsigned int talker_unique_id_n;
	uint16_t listener_unique_id[ACMP_CFG_MAX_UNIQUE_ID];
	unsigned int listener_unique_id_n;
	unsigned int valid_time;
	void *aem;
};

/**
 * \ingroup init
 * AVDECC stack component configuration
 */
struct avdecc_config {
	genavb_log_level_t log_level;		/**< AVDECC log level */
	unsigned int enabled;
	unsigned int srp_enabled;
	unsigned int num_entities;
	struct avdecc_entity_config entity_cfg[CFG_AVDECC_NUM_ENTITIES];
};

/**
 * \ingroup init
 * AVTP stack component configuration
 */
struct avtp_config {
	genavb_log_level_t log_level;		/**< AVTP log level */
	unsigned int logical_port_list[CFG_MAX_NUM_PORT];
	unsigned int clock_gptp_list[CFG_MAX_NUM_PORT];
};

/**
 * \ingroup init
 * SRP stack component configuration
 */
struct srp_config {
	genavb_log_level_t log_level;		/**< SRP log level */

	unsigned int is_bridge;

	unsigned int port_max;

	unsigned int management_enabled;

	struct msrp_config {
		unsigned int is_bridge;
		unsigned int flags;
		unsigned int port_max;
		unsigned int logical_port_list[CFG_MAX_NUM_PORT];
		unsigned int enabled;
	} msrp_cfg;

	struct mvrp_config {
		unsigned int is_bridge;
		unsigned int port_max;
		unsigned int logical_port_list[CFG_MAX_NUM_PORT];
	} mvrp_cfg;
};

/**
 * \ingroup init
 * Management stack component configuration
 */
struct management_config {
	genavb_log_level_t log_level;		/**< Management log level */
	unsigned int is_bridge;
	unsigned int port_max;
	unsigned int logical_port_list[CFG_MAX_NUM_PORT];
};

/**
 * \ingroup init
 * GPTP port configuration
 */
struct fgptp_port_config {
	uint8_t portRole;
	uint8_t ptpPortEnabled;
	int rxDelayCompensation;
	int txDelayCompensation;
};

/**
 * \ingroup init
 * GPTP domain configuration
 */
struct fgptp_domain_config {
	/* General params */
	int domain_number;

	unsigned int clock_target;
	unsigned int clock_source;

	/* Grandmaster params */
	uint8_t gmCapable;	/* set to 1 if this device is grandmaster capable */
	uint8_t priority1;
	uint8_t priority2;
	uint8_t clockClass;
	uint8_t clockAccuracy;
	uint16_t offsetScaledLogVariance;
};

/**
 * \ingroup init
 * GPTP stack component configuration
 */
struct fgptp_config {
	/* General params */
	genavb_log_level_t log_level;		/**< gPTP log level */

	unsigned int is_bridge;

	unsigned int profile; /* gptp profile (standard, automotive) */

	unsigned int domain_max;

	unsigned int port_max;

	unsigned int logical_port_list[CFG_MAX_NUM_PORT];

	unsigned int management_enabled;

	unsigned int clock_local;

	uint64_t gm_id; /* grand master id used in case of static slave configuration or resulting from bmca */
	uint64_t neighborPropDelayThreshold; /*expressed in ns. The propagation time threshold, above which a port is not considered capable of participating in the IEEE 802.1AS protocol.*/
	unsigned int rsync; /* set to 1 to enable rsync feature on slave side*/
	unsigned int rsync_interval; /* defines rsync packet interval in ms*/
	unsigned int statsInterval; /* defines the interval in second between statistics output */

	/* Automotive profile params */
	uint8_t neighborPropDelay_mode;					/* set to 1 if predefined pdelay mechanism is used. 0 means relying on standard pdelay exchange */
	double initial_neighborPropDelay[CFG_MAX_NUM_PORT];	/* expressed in ns. per port predefined value, but filled using single value from the default configuration file */
	double neighborPropDelay_sensitivity;			/* expressed in ns. global to all ports. defined the amount of ns between to pdelay measurement that would trigger a new indication to the host main code */

	/* Timing params */
	int8_t initialLogPdelayReqInterval;
	int8_t initialLogSyncInterval;
	int8_t initialLogAnnounceInterval;
	/*Avnu AutoCDSFunctionalSpec-1_4 - 6.2.1.5 / 6.2.1.6 */
	int8_t operLogPdelayReqInterval; /* A device moves to this value on all slave ports once the measured values have stabilized*/
	int8_t operLogSyncInterval; /*operLogSyncInterval is the Sync interval that a device moves to and signals on a slave port once it has achieved synchronization*/

	void (*sync_indication)();
	void (*gm_indication)();
	void (*pdelay_indication)();

	/* per port settings */
	struct fgptp_port_config port_cfg[CFG_MAX_NUM_PORT];

	/* per domain settings */
	struct fgptp_domain_config domain_cfg[CFG_MAX_GPTP_DOMAINS];

	/* IEEE 802.1AS-2011 operation */
	unsigned int force_2011;	/* set to 1 if the stask operates per IEEE 802.1AS-2011 standard (no multi domains support) */
};

/**
 * \endcond
 */

/** Initialize the GenAVB library.
 * \ingroup init
 * \return		::GENAVB_SUCCESS or negative error code. In case of success genavb argument is updated with library handle
 * \param genavb	pointer to GenAVB library handle pointer
 * \param flags		bitmap of configuration options. For future expansion, for now always pass 0.
 */
int genavb_init(struct genavb_handle **genavb, unsigned int flags);


/** Exit the GenAVB library.
 * \ingroup init
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param genavb	pointer to GenAVB library handle structure
 */
int genavb_exit(struct genavb_handle *genavb);

/* OS specific headers */
#include "os/init.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_INIT_API_H_ */
