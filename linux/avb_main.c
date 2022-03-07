/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020-2021 NXP
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
 @brief Top AVB linux process
 @details Setups linux AVB process and threads for each AVTP stack components.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/ioctl.h>

#include "common/version.h"
#include "common/log.h"
#include "genavb/helpers.h"
#include "genavb/aem_helpers.h"
#include "genavb/sr_class.h"
#include "cfgfile.h"
#include "os/clock.h"

#include "avb.h"
#include "init.h"
#include "log.h"
#include "net.h"

#if defined(CONFIG_AVDECC)
#include "avdecc/config.h"
#endif

#if defined(CONFIG_AVTP)
void *avtp_thread_main(void *arg);
#endif
#if defined(CONFIG_SRP)
void *srp_thread_main(void *arg);
#endif
#if defined(CONFIG_AVDECC)
void *avdecc_thread_main(void *arg);
#endif
#if defined(CONFIG_MAAP)
void *maap_thread_main(void *arg);
#endif

/* Default configuration files, if none are specified on cmd line */
#define AVB_CONF_FILENAME	"/etc/genavb/genavb-listener.cfg"
#define SRP_CONF_FILENAME		"/etc/genavb/srp.cfg"
#define SRP_BRIDGE_CONF_FILENAME	"/etc/genavb/srp-br.cfg"

#define CHAR_LEN		16

static int terminate = 0;

static void sigterm_hdlr(int signum)
{
	os_log(LOG_INIT, "signum(%d)\n", signum);

	terminate = 1;
}

void print_version(void)
{
	printf("NXP's GenAVB/TSN stack version %s (Built %s %s)\n", GENAVB_VERSION, __DATE__, __TIME__);
}

void print_usage (void)
{
	printf("\nUsage:\n avb [options]\n");
	printf("\nOptions:\n"
		"\t-v                    display program version\n"
		"\t-b                    start in bridge mode\n"
		"\t-f <config file>      avb configuration filename\n"
		"\t-s <config file>      srp configuration filename\n"
		"\t-h                    print this help text\n");
}

#if defined(CONFIG_AVDECC)
static unsigned int parse_list_u16(char *str, u16 *list)
{
	unsigned int i = 0;
	char *string;

	string = strtok(str, ",");
	while (string && (i < ACMP_CFG_MAX_UNIQUE_ID)) {
		errno = 0;
		list[i] = strtoul(string, NULL, 0);
		if (errno != 0)
			break;
		i++;
		string = strtok(NULL, ",");
	}

	return i;
}

static unsigned int parse_list_u64(char *str, u64 *list)
{
	unsigned int i = 0;
	char *string;

	string = strtok(str, ",");
	while (string && (i < ACMP_CFG_MAX_UNIQUE_ID)) {
		errno = 0;
		list[i] = strtoull(string, NULL, 0);
		if (errno != 0)
			break;
		i++;
		string = strtok(NULL, ",");
	}

	return i;
}
#endif

static int log_string2level(const char *s)
{
	int level;

	for (level = LOG_CRIT; level <= LOG_DEBUG; level++) {
		if (!strcasecmp(s, log_lvl_string[level]))
			return level;
	}

	return -1;
}


static sr_class_t str_to_sr_class(const char *s)
{
    char* str = "A";
    char strmax[2] = "\0";
    strmax[0] = 'A' + SR_CLASS_MAX;

	if (!(strcmp(s, str) < 0)     //class requested is not under "A"
	 && !(strcmp(s, strmax) >= 0) //class requested is not above SR_CLASS_MAX
	 && (strlen(s) == 1))         //class requested is valid
		return (sr_class_t)(SR_CLASS_A + s[0] - 'A');
	else
	{
		os_log(LOG_ERR, "Unrecognized requested SR class, NONE selected by default\n");
		return SR_CLASS_NONE;
	}
}

static int process_sr_class_config(char* stringvalue, struct _SECTIONENTRY *configtree, uint8_t *_sr_class)
{
	int i, n;
	uint8_t sr_class[CFG_SR_CLASS_MAX] = {
		[0 ... CFG_SR_CLASS_MAX - 1] = SR_CLASS_NONE
	};
	char item[CFG_SR_CLASS_MAX][CFG_STRING_LIST_MAX_LEN];

	n = cfg_get_string_list(configtree, "AVB_GENERAL", "sr_class_enabled", "A, B", item, CFG_SR_CLASS_MAX);
	if (n < 0) {
		os_log(LOG_ERR, "Failed to read sr_class_enabled value\n");
		goto err;
	}

	for (i = 0; i < n; i++)
		sr_class[i] = str_to_sr_class(item[i]);

	memcpy(_sr_class, &sr_class, sizeof(sr_class));

	return 0;

err:
	return -1;
}

static int process_section_general(struct _SECTIONENTRY *configtree, struct avb_ctx *avb)
{
	int level;
	int i, rc = 0;
	int nb_cmp;
	char log_item[max_COMPONENT_ID][CFG_STRING_LIST_MAX_LEN];
	char stringvalue[CFG_STRING_MAX_LEN] = "";


	/* loglevel */
	if (cfg_get_string(configtree, "AVB_GENERAL", "log_level", "info", stringvalue)) {
		rc = -1;
		goto exit;
	}

	level = log_string2level(stringvalue);
	if (level < 0) {
		printf("Error setting log level (%s)\n", stringvalue);
		rc = -1;
		goto exit;
	}

	printf("AVB cfg file: setting log level to %s (%d)\n", stringvalue, level);

	log_level_set(os_COMPONENT_ID, level);
	log_level_set(common_COMPONENT_ID, level);

	avb->avtp_cfg.log_level = level;
	avb->avdecc_cfg.log_level = level;
	avb->maap_cfg.log_level = level;

	/* log_monotonic */
	if (cfg_get_string(configtree, "AVB_GENERAL", "log_monotonic", "disabled", stringvalue)) {
		rc = -1;
		goto exit;
	}

	if (!strcmp(stringvalue, "enabled"))
		log_enable_monotonic();


	/* enable sr_class */
	if ((rc = process_sr_class_config(stringvalue, configtree, avb->sr_class)) < 0)
		goto exit;

	/* disable log for some components */
	nb_cmp = cfg_get_string_list(configtree, "AVB_GENERAL", "disable_component_log", "none", log_item, max_COMPONENT_ID);
	if (nb_cmp < 0) {
		rc = -1;
		goto exit;
	}

	/* parse list of components to disable trace for ... */
	for (i = 0; i < nb_cmp; i++) {
		if (strcasestr(log_item[i], "avtp"))
			avb->avtp_cfg.log_level = LOG_CRIT;
		else if (strcasestr(log_item[i], "avdecc"))
			avb->avdecc_cfg.log_level = LOG_CRIT;
		else if (strcasestr(log_item[i], "common"))
			log_level_set(common_COMPONENT_ID, LOG_CRIT);
		else if (strcasestr(log_item[i], "os"))
			log_level_set(os_COMPONENT_ID, LOG_CRIT);
	}

exit:
	return rc;
}


static int process_section_avtp(struct _SECTIONENTRY *configtree, struct avtp_config *avtp_cfg)
{
#if 0
	char stringvalue[CFG_STRING_MAX_LEN] = "";

	if (cfg_get_string(configtree, "AVB_AVTP", "string_option", "0", stringvalue))
		goto exit;

	return 0;

exit:
	return -1;
#endif
	return 0;
}


#if defined(CONFIG_AVDECC)
static int process_section_avdecc(struct _SECTIONENTRY *configtree, struct avdecc_config *avdecc_cfg)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	char section_name[CFG_STRING_MAX_LEN] = "";
	int nb_entity = 0;
	int entity_index = 0;
	int rc = 0;
	unsigned int i;
	u64 association_id = 0;
	struct avdecc_entity_config *entity_cfg;

	entity_cfg = &avdecc_cfg->entity_cfg[0];

	if (cfg_get_string(configtree, "AVB_AVDECC", "enabled", "1", stringvalue))
		goto exit;

	if (strcmp(stringvalue, "1")) {
		avdecc_cfg->enabled = false;
		goto out;
	}

	avdecc_cfg->enabled = true;

	if (cfg_get_string(configtree, "AVB_AVDECC", "milan_mode", "0", stringvalue))
		goto exit;

	if (!strcmp(stringvalue, "0"))
		avdecc_cfg->milan_mode = false;
	else
		avdecc_cfg->milan_mode = true;

	if (cfg_get_uint(configtree, "AVB_AVDECC", "max_entities_discovery",
				CFG_ADP_DEFAULT_NUM_ENTITIES_DISCOVERY, CFG_ADP_MIN_NUM_ENTITIES_DISCOVERY, CFG_ADP_MAX_NUM_ENTITIES_DISCOVERY, &avdecc_cfg->max_entities_discovery))
		goto exit;

	if (avdecc_cfg->milan_mode) {
		/* Milan mode forces SRP enabled */
		avdecc_cfg->srp_enabled = true;
	} else {
		/* Configuration options available only in non-milan mode */

		if (cfg_get_string(configtree, "AVB_AVDECC", "srp_enabled", "1", stringvalue))
			goto exit;

		if (!strcmp(stringvalue, "0"))
			avdecc_cfg->srp_enabled = false;
		else
			avdecc_cfg->srp_enabled = true;

		/* fast_connect_btb */
		if (cfg_get_string(configtree, "AVB_AVDECC", "btb_demo_mode", "0", stringvalue))
			goto exit;

		if (!strcmp(stringvalue, "1")) {
			entity_cfg->flags |= AVDECC_FAST_CONNECT_BTB;
			entity_cfg->flags |= AVDECC_FAST_CONNECT_MODE; /* btb demo mode relies on fast connect, so enable it too */
		} else if (strcmp(stringvalue, "0"))
			printf("AVB cfg file: Warning, unknow value for btb_demo_mode (%s). Ignoring.\n", stringvalue);


		/* enable fast-connect mode */
		if (cfg_get_string(configtree, "AVB_AVDECC", "fast_connect", "0", stringvalue))
			goto exit;

		if (!strcmp(stringvalue, "1"))
			entity_cfg->flags |= AVDECC_FAST_CONNECT_MODE;
		else if (strcmp(stringvalue, "0"))
			printf("AVB cfg file: Warning, unknow value for avdecc_fast_connect (%s). Ignoring.\n", stringvalue);
	}

	for (entity_index = 0; entity_index < CFG_AVDECC_NUM_ENTITIES; entity_index++) {
		snprintf(section_name, 256, "AVB_AVDECC_ENTITY_%d", entity_index + 1);
		//printf("Processing section %s\n", section_name);

		rc = cfg_get_string(configtree, section_name, "entity_file", "none", stringvalue);

		if (rc == 0 && strcmp(stringvalue, "none")) {
			/* aem file name - load aem file */
			entity_cfg = &avdecc_cfg->entity_cfg[nb_entity];

			entity_cfg->aem = aem_entity_load_from_file(stringvalue);
			if (!entity_cfg->aem)
				goto err;

			nb_entity++;

			if (cfg_get_u64(configtree, section_name, "entity_id", 0, 0, UINT64_MAX, &entity_cfg->entity_id))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_listener_streams", CFG_ACMP_DEFAULT_NUM_LISTENER_STREAMS, CFG_ACMP_MIN_NUM_LISTENER_STREAMS, CFG_ACMP_MAX_NUM_LISTENER_STREAMS, &entity_cfg->max_listener_streams))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_talker_streams", CFG_ACMP_DEFAULT_NUM_TALKER_STREAMS, CFG_ACMP_MIN_NUM_TALKER_STREAMS, CFG_ACMP_MAX_NUM_TALKER_STREAMS, &entity_cfg->max_talker_streams))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_inflights", CFG_AVDECC_DEFAULT_NUM_INFLIGHTS, CFG_AVDECC_MIN_NUM_INFLIGHTS, CFG_AVDECC_MAX_NUM_INFLIGHTS, &entity_cfg->max_inflights))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_unsolicited_registratons", CFG_AECP_DEFAULT_NUM_UNSOLICITED, CFG_AECP_MIN_NUM_UNSOLICITED, CFG_AECP_MAX_NUM_UNSOLICITED, &entity_cfg->max_unsolicited_registrations))
				goto err;

			if (cfg_get_uint(configtree, section_name, "channel_waitmask", 0, 0, 7, &entity_cfg->channel_waitmask))
				goto err;

			if (avdecc_cfg->milan_mode) {
				/* Force milan fixed valid time */
				entity_cfg->valid_time = CFG_ADP_MILAN_VALID_TIME;
			} else {
				/* Configuration options available only in non-milan mode */

				if (cfg_get_uint(configtree, section_name, "valid_time", CFG_ADP_DEFAULT_VALID_TIME, CFG_ADP_MIN_VALID_TIME, CFG_ADP_MAX_VALID_TIME, &entity_cfg->valid_time))
					goto err;

				if (cfg_get_uint(configtree, section_name, "max_listener_pairs", CFG_ACMP_DEFAULT_NUM_LISTENER_PAIRS, CFG_ACMP_MIN_NUM_LISTENER_PAIRS, CFG_ACMP_MAX_NUM_LISTENER_PAIRS, &entity_cfg->max_listener_pairs))
					goto err;

				/* talker entity ID list */
				if (cfg_get_string(configtree, section_name, "talker_entity_id_list", "none", stringvalue))
					goto err;

				if (strcmp(stringvalue, "none"))
					entity_cfg->talker_entity_id_n = parse_list_u64(stringvalue, entity_cfg->talker_entity_id);

				/* talker unique ID list */
				if (cfg_get_string(configtree, section_name, "talker_unique_id_list", "none", stringvalue))
					goto err;

				if (strcmp(stringvalue, "none"))
					entity_cfg->talker_unique_id_n = parse_list_u16(stringvalue, entity_cfg->talker_unique_id);

				/* listener unique ID list */
				if (cfg_get_string(configtree, section_name, "listener_unique_id_list", "none", stringvalue))
					goto err;

				if (strcmp(stringvalue, "none"))
					entity_cfg->listener_unique_id_n = parse_list_u16(stringvalue, entity_cfg->listener_unique_id);
			}
		}
	}


	avdecc_cfg->num_entities = nb_entity;

	printf("AVB cfg file: loaded %d AEM entities.\n", nb_entity);


	/* association ID */
	if (cfg_get_u64(configtree, "AVB_AVDECC", "association_id", 0, 0, UINT64_MAX, &association_id))
		goto err;

	printf("Advertising association ID 0x%" PRIx64 " for all entities.\n", association_id);

	for (i = 0; i < avdecc_cfg->num_entities; i++)
		avdecc_cfg->entity_cfg[i].association_id = association_id;

out:
	return 0;

err:
	/* free memory allocated from aem_entity_load_from_file() */
	for (i = 0; i < avdecc_cfg->num_entities; i++) {
		if (avdecc_cfg->entity_cfg[i].aem)
			free(avdecc_cfg->entity_cfg[i].aem);
	}

exit:
	return -1;
}
#endif

static int process_avb_config(struct avb_ctx *avb, const char *filename)
{
	struct _SECTIONENTRY *configtree;

	/**************************************************************/
	/* open cfg file, store inputs in a tree, then process inputs */
	/**************************************************************/
	printf("AVB: Using configuration file: %s\n", filename);

	/* read all sections and all key/value pairs from config file, and store them in chained list */
	configtree = cfg_read(filename);
	if (!configtree)
		goto err_open;

	/********************************/
	/* fetch values from configtree */
	/********************************/

	if (process_section_general(configtree, avb))
		goto err_parse;

	if (process_section_avtp(configtree, &avb->avtp_cfg))
		goto err_parse;

#if defined(CONFIG_AVDECC)
	if (process_section_avdecc(configtree, &avb->avdecc_cfg))
		goto err_parse;
#else
	avb->avdecc_cfg.enabled = false;
#endif

	/* finished parsing the configuration tree, so free memory */
	cfg_free_configtree(configtree);

	return 0;

err_parse:
	printf("AVB cfg file: Error while parsing config file\n");

	cfg_free_configtree(configtree);
err_open:
	return -1;
}

static int process_section_msrp(struct _SECTIONENTRY *configtree, struct msrp_config *cfg)
{
	if (cfg_get_uint(configtree, "MSRP", "enabled", 1, 0, 1, &cfg->enabled))
		goto exit;

	cfg->flags = 0;

	return 0;

exit:
	return -1;
}

static int process_section_srp_general(struct _SECTIONENTRY *configtree, struct srp_config *cfg)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	int level;

	/* loglevel */
	if (cfg_get_string(configtree, "SRP_GENERAL", "log_level", "info", stringvalue)) {
		goto err;
	}

	level = log_string2level(stringvalue);
	if (level < 0) {
		printf("Error setting log level (%s)\n", stringvalue);
		goto err;
	}

	printf("SRP cfg file: setting log level to %s (%d)\n", stringvalue, level);

	cfg->log_level = level;

	return 0;

err:
	return -1;
}

static int process_srp_config(struct srp_config *cfg, const char *filename)
{
	struct _SECTIONENTRY *configtree;

	printf("SRP: Using configuration file: %s\n", filename);

	/* read all sections and all key/value pairs from config file, and store them in chained list */
	configtree = cfg_read(filename);
	if (!configtree)
		goto err_open;

	/********************************/
	/* fetch values from configtree */
	/********************************/

	if (process_section_srp_general(configtree, cfg))
		goto err_parse;

	if (process_section_msrp(configtree, &cfg->msrp_cfg))
		goto err_parse;

	/* finished parsing the configuration tree, so free memory */
	cfg_free_configtree(configtree);

	return 0;

err_parse:
	printf("SRP cfg file: Error while parsing config file\n");

	cfg_free_configtree(configtree);

err_open:
	return -1;
}

static int endpoint_init(struct avb_ctx *avb)
{
#if defined(CONFIG_AVTP) || defined(CONFIG_MAAP) || defined(CONFIG_AVDECC)
	int rc;
#endif

#if defined(CONFIG_AVTP)
	pthread_cond_init(&avb->avtp_cond, NULL);
#endif
#if defined(CONFIG_MAAP)
	pthread_cond_init(&avb->maap_cond, NULL);
#endif
#if defined(CONFIG_AVDECC)
	if (avb->avdecc_cfg.enabled)
		pthread_cond_init(&avb->avdecc_cond, NULL);
#endif

#if defined(CONFIG_AVTP)
	rc = pthread_create(&avb->avtp_thread, NULL, avtp_thread_main, avb);
	if (rc) {
		os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
		goto err_pthread_create_avtp;
	}
#endif
#if defined(CONFIG_MAAP)
	rc = pthread_create(&avb->maap_thread, NULL, maap_thread_main, avb);
	if (rc) {
		os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
		goto err_pthread_create_maap;
	}
#endif

	pthread_mutex_lock(&avb->status_mutex);
#if defined(CONFIG_AVTP)
	while (!avb->avtp_status) pthread_cond_wait(&avb->avtp_cond, &avb->status_mutex);
#endif
#if defined(CONFIG_MAAP)
	while (!avb->maap_status) pthread_cond_wait(&avb->maap_cond, &avb->status_mutex);
#endif
	pthread_mutex_unlock(&avb->status_mutex);

#if defined(CONFIG_AVTP)
	if (avb->avtp_status < 0)
		goto err_thread_init;
#endif

#if defined(CONFIG_MAAP)
	if (avb->maap_status < 0)
		goto err_thread_init;
#endif

#if defined(CONFIG_AVDECC)
	if (avb->avdecc_cfg.enabled) {
		rc = pthread_create(&avb->avdecc_thread, NULL, avdecc_thread_main, avb);
		if (rc) {
			os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
			goto err_pthread_create_avdecc;
		}

		pthread_mutex_lock(&avb->status_mutex);
		while (!avb->avdecc_status) pthread_cond_wait(&avb->avdecc_cond, &avb->status_mutex);
		pthread_mutex_unlock(&avb->status_mutex);

		if (avb->avdecc_status < 0)
			goto err_avdecc_init;
	}
#endif

	return 0;

#if defined(CONFIG_AVDECC)
err_avdecc_init:
	if (avb->avdecc_cfg.enabled) {
		pthread_cancel(avb->avdecc_thread);
		pthread_join(avb->avdecc_thread, NULL);
	}

err_pthread_create_avdecc:
#endif
#if defined(CONFIG_AVTP) || defined(CONFIG_MAAP)
err_thread_init:
#endif
#if defined(CONFIG_MAAP) && defined(CONFIG_AVDECC)
	pthread_cancel(avb->maap_thread);
	pthread_join(avb->maap_thread, NULL);
#endif
#if defined(CONFIG_MAAP)
err_pthread_create_maap:
#endif
#if defined(CONFIG_AVTP) && (defined(CONFIG_MAAP) || defined(CONFIG_AVDECC))
	pthread_cancel(avb->avtp_thread);
	pthread_join(avb->avtp_thread, NULL);
#endif
#if defined(CONFIG_AVTP)
err_pthread_create_avtp:
#endif
	return -1;
}

static void endpoint_exit(struct avb_ctx *avb)
{
#if defined(CONFIG_AVDECC)
	if (avb->avdecc_cfg.enabled) {
		pthread_cancel(avb->avdecc_thread);
		pthread_join(avb->avdecc_thread, NULL);
	}
#endif

#if defined(CONFIG_MAAP)
	pthread_cancel(avb->maap_thread);
#endif
#if defined(CONFIG_AVTP)
	pthread_cancel(avb->avtp_thread);
#endif
#if defined(CONFIG_MAAP)
	pthread_join(avb->maap_thread, NULL);
#endif
#if defined(CONFIG_AVTP)
	pthread_join(avb->avtp_thread, NULL);
#endif
}

int main(int argc, char *argv[])
{
	struct avb_ctx *avb;
	struct os_net_config net_config = { .net_mode = CONFIG_AVB_DEFAULT_NET };
#if defined(CONFIG_SRP)
	pthread_t srp_thread;
#endif
	sigset_t set;
	struct sigaction action;
	int option;
	int i;
	const char *avb_conf_filename;
	const char *srp_conf_filename;
	unsigned int bridge_logical_port_list[CFG_BR_DEFAULT_NUM_PORTS] = CFG_BR_LOGICAL_PORT_LIST;
	unsigned int endpoint_logical_port_list[CFG_EP_DEFAULT_NUM_PORTS] = CFG_EP_LOGICAL_PORT_LIST;
	unsigned int *logical_port_list;
	unsigned int port_max;
	bool is_bridge = false;
	int fd;
	os_clock_id_t clock_log;
	int rc;

	/* Setup standard output in append mode so that log file truncate works correctly */
	fd = fileno(stdout);
	if (fd < 0)
		goto err_fileno;

	if (fcntl(fd, F_SETFL, O_APPEND) < 0)
		goto err_fcntl;

	print_version();

	avb = malloc(sizeof(struct avb_ctx));
	if (!avb) {
		printf("malloc(): failed, aborting.\n");
		goto err_malloc;
	}

	memset(avb, 0, sizeof(struct avb_ctx));

	avb_conf_filename = AVB_CONF_FILENAME;
	srp_conf_filename = NULL;

	while ((option = getopt(argc, argv,"vhbf:s:")) != -1) {
		switch (option) {
		case 'v':
			print_version();
			goto exit;
			break;

		case 'b':
			is_bridge = true;
			break;

		case 'f':
			avb_conf_filename = optarg;
			break;

		case 's':
			srp_conf_filename = optarg;
			break;

		case 'h':
		default:
			print_usage();
			goto exit;
		}
	}

	if (!srp_conf_filename) {
		if (is_bridge)
			srp_conf_filename = SRP_BRIDGE_CONF_FILENAME;
		else
			srp_conf_filename = SRP_CONF_FILENAME;
	}

	if (process_avb_config(avb, avb_conf_filename) < 0) /* Cfg file processing failed, exit */
		goto err_config;

	if (process_srp_config(&avb->srp_cfg, srp_conf_filename) < 0) /* Cfg file processing failed, exit */
		goto err_config;

	if (os_init(&net_config) < 0)
		goto err_osal;

	for (i = 0; i < CFG_MAX_LOGICAL_PORTS; i++)
		if (net_port_sr_config(i, avb->sr_class) < 0)
			goto err_osal;

	if (sr_class_config(avb->sr_class) < 0) {
		os_log(LOG_ERR, "sr_class_config failed\n");
		goto err_osal;
	}

	if (is_bridge) {
		logical_port_list = bridge_logical_port_list;
		port_max = CFG_BR_DEFAULT_NUM_PORTS;
		clock_log = OS_CLOCK_GPTP_BR_0_0;
	} else {
		logical_port_list = endpoint_logical_port_list;
		port_max = CFG_EP_DEFAULT_NUM_PORTS;
		clock_log = OS_CLOCK_GPTP_EP_0_0;
	}

	avb->srp_cfg.is_bridge = is_bridge;

	avb->srp_cfg.port_max = port_max;

#ifdef CONFIG_MANAGEMENT
	avb->srp_cfg.management_enabled = 1;
	avb->avdecc_cfg.management_enabled = true;
#else
	avb->srp_cfg.management_enabled = 0;
	avb->avdecc_cfg.management_enabled = false;
#endif

	avb->srp_cfg.msrp_cfg.is_bridge = is_bridge;
	avb->srp_cfg.msrp_cfg.port_max = port_max;
	avb->srp_cfg.mvrp_cfg.is_bridge = is_bridge;
	avb->srp_cfg.mvrp_cfg.port_max = port_max;

	avb->maap_cfg.port_max = port_max;

	for (i = 0; i < port_max; i++) {
		avb->avtp_cfg.logical_port_list[i] = logical_port_list[i];
		avb->avtp_cfg.clock_gptp_list[i] = logical_port_to_gptp_clock(logical_port_list[i], CFG_DEFAULT_GPTP_DOMAIN);

		avb->srp_cfg.msrp_cfg.logical_port_list[i] = logical_port_list[i];
		avb->srp_cfg.mvrp_cfg.logical_port_list[i] = logical_port_list[i];

		avb->maap_cfg.logical_port_list[i] = logical_port_list[i];
	}

	/* Block most signals for all threads */
	if (sigfillset(&set) < 0)
		os_log(LOG_ERR, "sigfillset(): %s\n", strerror(errno));

	if (sigdelset(&set, SIGILL) < 0)
		os_log(LOG_ERR, "sigdelset(): %s\n", strerror(errno));

	if (sigdelset(&set, SIGFPE) < 0)
		os_log(LOG_ERR, "sigdelset(): %s\n", strerror(errno));

	if (sigdelset(&set, SIGSEGV) < 0)
		os_log(LOG_ERR, "sigdelset(): %s\n", strerror(errno));

	rc = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (rc)
		os_log(LOG_ERR, "pthread_sigmask(): %s\n", strerror(rc));

#if defined(CONFIG_SRP)
	pthread_cond_init(&avb->srp_cond, NULL);

	rc = pthread_create(&srp_thread, NULL, srp_thread_main, avb);
	if (rc) {
		os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
		goto err_pthread_create_srp;
	}

	pthread_mutex_lock(&avb->status_mutex);

	while (!avb->srp_status) pthread_cond_wait(&avb->srp_cond, &avb->status_mutex);

	pthread_mutex_unlock(&avb->status_mutex);

	if (avb->srp_status < 0)
		goto err_thread_init;
#endif

	if (!is_bridge)
		if (endpoint_init(avb) < 0)
			goto err_endpoint_init;

	action.sa_handler = sigterm_hdlr;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		os_log(LOG_ERR, "sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGTERM, &action, NULL) < 0)
		os_log(LOG_ERR, "sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGQUIT, &action, NULL) < 0)
		os_log(LOG_ERR, "sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGINT, &action, NULL) < 0)
		os_log(LOG_ERR, "sigaction(): %s\n", strerror(errno));

	/* Unblock normal process control signals for the main thread only */
	if (sigemptyset(&set) < 0)
		os_log(LOG_ERR, "sigfillset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGINT) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGTERM) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGQUIT) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGTSTP) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	rc = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	if (rc)
		os_log(LOG_ERR, "sigprocmask(): %s\n", strerror(rc));

	while (1) {
		sleep(1);

		log_update_time(clock_log);
		log_update_monotonic();

		if (terminate)
			break;
	}

	if (!is_bridge)
		endpoint_exit(avb);

err_endpoint_init:
#if defined(CONFIG_SRP)
err_thread_init:
	pthread_cancel(srp_thread);
	pthread_join(srp_thread, NULL);

err_pthread_create_srp:
#endif

	os_exit();

	if (avb->avdecc_cfg.enabled) {
		for (i = 0; i< avb->avdecc_cfg.num_entities; i++) {
			if (avb->avdecc_cfg.entity_cfg[i].aem)
				free(avb->avdecc_cfg.entity_cfg[i].aem);
		}
	}

exit:
	free(avb);

	return 0;

err_osal:
err_config:
	free(avb);

err_malloc:
err_fcntl:
err_fileno:
	return -1;
}
