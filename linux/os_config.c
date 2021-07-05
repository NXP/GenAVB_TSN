/*
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

#include "cfgfile.h"
#include "os_config.h"

#define CONF_FILE_NAME "/etc/genavb/system.cfg"

#define LOGICAL_PORT_ENDPOINT_DEFAULT	"eth0, eth1"
#define LOGICAL_PORT_BRIDGE_DEFAULT	"SJA1105P_p0, SJA1105P_p1, SJA1105P_p2, SJA1105P_p3, SJA1105P_p4*"
#define LOGICAL_PORT_BRIDGE_DEFAULT_INTERFACE	"br0"

#define CLOCK_ENDPOINT_GPTP_0_DEFAULT		"/dev/ptp0"	/* domain 0 */
#define CLOCK_ENDPOINT_GPTP_1_DEFAULT		"sw_clock"	/* domain 1 */
#define CLOCK_ENDPOINT_LOCAL_DEFAULT		"/dev/ptp0"

#define CLOCK_BRIDGE_GPTP_0_DEFAULT		"sw_clock"		/* domain 0 */
#define CLOCK_BRIDGE_GPTP_1_DEFAULT		"sw_clock"		/* domain 1 */
#define CLOCK_BRIDGE_LOCAL_DEFAULT		"/dev/ptp1"

static int process_section_logical_port(struct _SECTIONENTRY *configtree, struct os_logical_port_config *config)
{
	if (cfg_get_string_list(configtree, "LOGICAL_PORT", "endpoint", LOGICAL_PORT_ENDPOINT_DEFAULT, config->endpoint, CFG_MAX_ENDPOINTS) < 0)
		goto err;

	if (cfg_get_string_list(configtree, "LOGICAL_PORT", "bridge", LOGICAL_PORT_BRIDGE_DEFAULT_INTERFACE, config->bridge, CFG_MAX_BRIDGES) < 0)
		goto err;

	if (cfg_get_string_list(configtree, "LOGICAL_PORT", "bridge_0", LOGICAL_PORT_BRIDGE_DEFAULT, config->bridge_ports[0], CFG_MAX_NUM_PORT) < 0)
		goto err;

	return 0;

err:
	return -1;
}

static int process_section_clock(struct _SECTIONENTRY *configtree, struct os_clock_config *config)
{
	if (cfg_get_string_list(configtree, "CLOCK", "endpoint_gptp_0", CLOCK_ENDPOINT_GPTP_0_DEFAULT, config->endpoint_gptp[0], CFG_MAX_ENDPOINTS) < 0)
		goto err;

	if (cfg_get_string_list(configtree, "CLOCK", "bridge_gptp_0", CLOCK_BRIDGE_GPTP_0_DEFAULT, config->bridge_gptp[0], CFG_MAX_BRIDGES) < 0)
		goto err;

#if CFG_MAX_GPTP_DOMAINS > 1
	if (cfg_get_string_list(configtree, "CLOCK", "endpoint_gptp_1", CLOCK_ENDPOINT_GPTP_1_DEFAULT, config->endpoint_gptp[1], CFG_MAX_ENDPOINTS) < 0)
		goto err;

	if (cfg_get_string_list(configtree, "CLOCK", "bridge_gptp_1", CLOCK_BRIDGE_GPTP_1_DEFAULT, config->bridge_gptp[1], CFG_MAX_BRIDGES) < 0)
		goto err;
#endif

	if (cfg_get_string_list(configtree, "CLOCK", "endpoint_local", CLOCK_ENDPOINT_LOCAL_DEFAULT, config->endpoint_local, CFG_MAX_ENDPOINTS) < 0)
		goto err;

	if (cfg_get_string_list(configtree, "CLOCK", "bridge_local", CLOCK_BRIDGE_LOCAL_DEFAULT, config->bridge_local, CFG_MAX_BRIDGES) < 0)
		goto err;

	return 0;

err:
	return -1;
}

static int process_os_config(struct os_config *config, struct _SECTIONENTRY *configtree)
{

	/********************************/
	/* fetch values from configtree */
	/********************************/

	if (process_section_logical_port(configtree, &config->logical_port_config))
		goto err;

	if (process_section_clock(configtree, &config->clock_config))
		goto err;

	return 0;

err:
	return -1;
}

int os_config_get(struct os_config *config)
{
	struct _SECTIONENTRY *configtree;
	int rc;

	configtree = cfg_read(CONF_FILE_NAME);

	/* Allow non existing file */

	rc = process_os_config(config, configtree);

	/* finished parsing the configuration tree, so free memory */
	cfg_free_configtree(configtree);

	if (rc) /* Cfg file processing failed, exit */
		goto err_config;

	return 0;

err_config:
	return -1;
}
