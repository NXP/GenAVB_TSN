/*
* Copyright 2025-2026 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief GPTP managed objects definitions
 @details Defines strings and Identifier for all gPTP nodes
*/

#ifndef _GENAVB_PUBLIC_MANAGED_OBJECTS_GPTP_H_
#define _GENAVB_PUBLIC_MANAGED_OBJECTS_GPTP_H_

#include <string.h>

#include <genavb/managed_objects.h>

/* Top nodes names */
#define GPTP_INSTANCE_LIST_STR     "instance"
#define GPTP_COMMON_SERVICES_STR   "common-services"

/* Instance List index name */
#define GPTP_INSTANCE_INDEX_STR    "instance-index"

/* Container Names */
#define GPTP_DEFAULT_DS_STR            "default-ds"
#define GPTP_CURRENT_DS_STR            "current-ds"
#define GPTP_PARENT_DS_STR             "parent-ds"
#define GPTP_TIME_PROPERTIES_DS_STR    "time-properties-ds"
#define GPTP_PORTS_STR                 "ports"

/* Default Parameter Data Set Container LEAFs names */
#define GPTP_DEFAULT_DS_CLOCK_IDENTITY_STR             "clock-identity"
#define GPTP_DEFAULT_DS_NUMBER_PORTS_STR               "number-ports"
#define GPTP_DEFAULT_DS_CLOCK_QUALITY_STR              "clock-quality"
#define GPTP_DEFAULT_DS_PRIORITY1_STR                  "priority1"
#define GPTP_DEFAULT_DS_PRIORITY2_STR                  "priority2"
#define GPTP_DEFAULT_DS_DOMAIN_NUMBER_STR              "domain-number"
#define GPTP_DEFAULT_DS_CURRENT_TIME_STR               "current-time"
#define GPTP_DEFAULT_DS_GM_CAPABLE_STR                 "gm-capable"
#define GPTP_DEFAULT_DS_CURRENT_UTC_OFFSET_STR         "current-utc-offset"
#define GPTP_DEFAULT_DS_CURRENT_UTC_OFFSET_VALID_STR   "current-utc-offset-valid"
#define GPTP_DEFAULT_DS_LEAP59_STR                     "leap59"
#define GPTP_DEFAULT_DS_LEAP61_STR                     "leap61"
#define GPTP_DEFAULT_DS_TIME_TRACEABLE_STR             "time-traceable"
#define GPTP_DEFAULT_DS_FREQUENCY_TRACEABLE_STR        "frequency-traceable"
#define GPTP_DEFAULT_DS_PTP_TIMESCALE_STR              "ptp-timescale"
#define GPTP_DEFAULT_DS_TIME_SOURCE_STR                "time-source"

#define GPTP_CLOCK_QUALITY_CLOCK_CLASS_STR                 "clock-class"
#define GPTP_CLOCK_QUALITY_CLOCK_ACCURACY_STR              "clock-accuracy"
#define GPTP_CLOCK_QUALITY_OFFSET_SCALED_LOG_VARIANCE_STR  "offset-scaled-log-variance"

#define GPTP_CURRENT_TIME_SECONDS_FIELD_STR                "seconds-field"
#define GPTP_CURRENT_TIME_NANOSECONDS_FIELD_STR            "nanoseconds-field"

/* Current Parameter Data Set Container LEAFs */
#define GPTP_CURRENT_DS_STEPS_REMOVED_STR                  "steps-removed"
#define GPTP_CURRENT_DS_OFFSET_FROM_TIME_TRANSMITTER_STR   "offset-from-time-transmitter"
#define GPTP_CURRENT_DS_MEAN_DELAY_STR                     "mean-delay"
#define GPTP_CURRENT_DS_LAST_GM_PHASE_CHANGE_STR           "last-gm-phase-change"
#define GPTP_CURRENT_DS_LAST_GM_FREQ_CHANGE_STR            "last-gm-freq-change"
#define GPTP_CURRENT_DS_GM_TIMEBASE_INDICATOR_STR          "gm-timebase-indicator"
#define GPTP_CURRENT_DS_GM_CHANGE_COUNT_STR                "gm-change-count"
#define GPTP_CURRENT_DS_TIME_OF_LAST_GM_CHANGE_STR         "time-of-last-gm-change"
#define GPTP_CURRENT_DS_TIME_OF_LAST_PHASE_CHANGE_STR      "time-of-last-phase-change"
#define GPTP_CURRENT_DS_TIME_OF_LAST_FREQ_CHANGE_STR       "time-of-last-freq-change"

/* Parent Parameter Data Set Container LEAFs */
#define GPTP_PARENT_DS_PARENT_PORT_IDENTITY_STR                "parent-port-identity"
#define GPTP_PARENT_DS_PARENT_STATS_STR                        "parent-stats"
#define GPTP_PARENT_DS_GRANDMASTER_IDENTITY_STR                "grandmaster-identity"
#define GPTP_PARENT_DS_GRANDMASTER_CLOCK_QUALITY_STR           "grandmaster-clock-quality"
#define GPTP_PARENT_DS_GRANDMASTER_PRIORITY1_STR               "grandmaster-priority1"
#define GPTP_PARENT_DS_GRANDMASTER_PRIORITY2_STR               "grandmaster-priority2"
#define GPTP_PARENT_DS_CUMULATIVE_RATE_RATIO_STR               "cumulative-rate-ratio"

#define GPTP_PORT_IDENTITY_CLOCK_IDENTITY_STR      "clock-identity"
#define GPTP_PORT_IDENTITY_PORT_NUMBER_STR         "port-number"

/* Time Properties Parameter Data Set Container LEAFs */
#define GPTP_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_STR         "current-utc-offset"
#define GPTP_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID_STR   "current-utc-offset-valid"
#define GPTP_TIME_PROPERTIES_DS_LEAP59_STR                     "leap59"
#define GPTP_TIME_PROPERTIES_DS_LEAP61_STR                     "leap61"
#define GPTP_TIME_PROPERTIES_DS_TIME_TRACEABLE_STR             "time-traceable"
#define GPTP_TIME_PROPERTIES_DS_FREQUENCY_TRACEABLE_STR        "frequency-traceable"
#define GPTP_TIME_PROPERTIES_DS_TIME_SOURCE_STR                "time-source"

/* Ports Parameter Data Set Container LEAFs */
#define GPTP_PORTS_PORT_STR                "port"

/* Port Container LEAFs */
#define GPTP_PORT_PORT_INDEX_STR                           "port-index"
#define GPTP_PORT_PORT_DS_STR                              "port-ds"
#define GPTP_PORT_PORT_STATS_DS_STR                        "port-statistics-ds"

/* Port Parameter Data Set Container LEAFs */
#define GPTP_PORT_DS_PORT_IDENTITY_STR                         "port-identity"
#define GPTP_PORT_DS_PORT_STATE_STR                            "port-state"
#define GPTP_PORT_DS_MEAN_LINK_DELAY_STR                       "mean-link-delay"
#define GPTP_PORT_DS_LOG_ANNOUNCE_INTERVAL_STR                 "log-announce-interval"
#define GPTP_PORT_DS_ANNOUNCE_RECEIPT_TIMEOUT_STR              "announce-receipt-timeout"
#define GPTP_PORT_DS_LOG_SYNC_INTERVAL_STR                     "log-sync-interval"
#define GPTP_PORT_DS_VERSION_NUMBER_STR                        "version-number"
#define GPTP_PORT_DS_DELAY_ASYMMETRY_STR                       "delay-asymmetry"
#define GPTP_PORT_DS_PORT_ENABLED_STR                          "port-enable"
#define GPTP_PORT_DS_IS_MEASURING_DELAY_STR                    "is-measuring-delay"
#define GPTP_PORT_DS_AS_CAPABLE_STR                            "as-capable"
#define GPTP_PORT_DS_MEAN_LINK_DELAY_THRESH_STR                "mean-link-delay-thresh"
#define GPTP_PORT_DS_NEIGHBOR_RATE_RATIO_STR                   "neighbor-rate-ratio"
#define GPTP_PORT_DS_INITIAL_LOG_ANNOUNCE_INTERVAL_STR         "initial-log-announce-interval"
#define GPTP_PORT_DS_CURRENT_LOG_ANNOUNCE_INTERVAL_STR         "current-log-announce-interval"
#define GPTP_PORT_DS_INITIAL_LOG_SYNC_INTERVAL_STR             "initial-log-sync-interval"
#define GPTP_PORT_DS_CURRENT_LOG_SYNC_INTERVAL_STR             "current-log-sync-interval"
#define GPTP_PORT_DS_SYNC_RECEIPT_TIMEOUT_STR                  "sync-receipt-timeout"
#define GPTP_PORT_DS_SYNC_RECEIPT_TIMEOUT_TIME_INTERVAL_STR    "sync-receipt-timeout-interval"
#define GPTP_PORT_DS_INITIAL_LOG_PDELAY_REQ_INTERVAL_STR       "initial-log-pdelay-req-interval"
#define GPTP_PORT_DS_CURRENT_LOG_PDELAY_REQ_INTERVAL_STR       "current-log-pdelay-req-interval"
#define GPTP_PORT_DS_ALLOWED_LOST_RESPONSES_STR                "allowed-lost-responses"
#define GPTP_PORT_DS_ALLOWED_FAULTS_STR                        "allowed-faults"

/* Port Parameter Statistics Container LEAFs */
#define GPTP_PORT_STATS_DS_RX_SYNC_COUNT_STR                                   "rx-sync-count"
#define GPTP_PORT_STATS_DS_RX_FOLLOW_UP_COUNT_STR                              "rx-follow-up-count"
#define GPTP_PORT_STATS_DS_RX_PDELAY_REQUEST_COUNT_STR                         "rx-pdelay-req-count"
#define GPTP_PORT_STATS_DS_RX_PDELAY_RESPONSE_COUNT_STR                        "rx-pdelay-resp-count"
#define GPTP_PORT_STATS_DS_RX_PDELAY_RESPONSE_FOLLOW_UP_COUNT_STR              "rx-pdelay-resp-follow-up-count"
#define GPTP_PORT_STATS_DS_RX_ANNOUNCE_COUNT_STR                               "rx-announce-count"
#define GPTP_PORT_STATS_DS_RX_PACKET_DISCARD_COUNT_STR                         "rx-packet-discard-count"
#define GPTP_PORT_STATS_DS_SYNC_RECEIPT_TIMEOUT_COUNT_STR                      "sync-receipt-timeout-count"
#define GPTP_PORT_STATS_DS_ANNOUNCE_RECEIPT_TIMEOUT_COUNT_STR                  "announce-receipt-timeout-count"
#define GPTP_PORT_STATS_DS_PDELAY_ALLOWED_LOST_RESPONSES_EXCEEDED_COUNT_STR    "pdelay-allowed-lost-exceeded-count"
#define GPTP_PORT_STATS_DS_TX_SYNC_COUNT_STR                                   "tx-sync-count"
#define GPTP_PORT_STATS_DS_TX_FOLLOW_UP_COUNT_STR                              "tx-follow-up-count"
#define GPTP_PORT_STATS_DS_TX_PDELAY_REQUEST_COUNT_STR                         "tx-pdelay-req-count"
#define GPTP_PORT_STATS_DS_TX_PDELAY_RESPONSE_COUNT_STR                        "tx-pdelay-resp-count"
#define GPTP_PORT_STATS_DS_TX_PDELAY_RESPONSE_FOLLOW_UP_COUNT_STR              "tx-pdelay-resp-follow-up-count"
#define GPTP_PORT_STATS_DS_TX_ANNOUNCE_COUNT_STR                               "tx-announce-count"

/*
 * \defgroup managed-objects	IEEE 802.1AS Managed Objects
 * \ingroup library
 *
 * Managed object tree definition, IEEE 802.1AS-2011, section 14
 * YANG module ieee1588-ptp-tt augmented by ieee802-dot1as-gptp
 */

/**
* \ingroup managed-objects
* gPTP node top level container enumerations
*/
typedef enum {
	GPTP_NODE_INSTANCE_LIST = 0,
	GPTP_NODE_COMMON_SERVICES,
	GPTP_NODE_MAX
} gptp_top_node_t;

/**
* \ingroup managed-objects
* Instance's container enumerations
*/
typedef enum {
	GPTP_INSTANCE_INSTANCE_INDEX = 0,
	GPTP_INSTANCE_DEFAULT_DS,
	GPTP_INSTANCE_CURRENT_DS,
	GPTP_INSTANCE_PARENT_DS,
	GPTP_INSTANCE_TIME_PROPERTIES_DS,
	GPTP_INSTANCE_PORTS,
	GPTP_INSTANCE_MAX
} gptp_instance_container_t;

/**
* \ingroup managed-objects
* Common Services's container enumerations
*/
typedef enum {
	GPTP_COMMON_SERVICES_CMLDS = 0,
	GPTP_COMMON_SERVICES_MAX
} gptp_common_services_container_type_t;

/**
* \ingroup managed-objects
* Default Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_DEFAULT_DS_CLOCK_IDENTITY = 0,
	GPTP_DEFAULT_DS_NUMBER_PORTS,
	GPTP_DEFAULT_DS_CLOCK_QUALITY,
	GPTP_DEFAULT_DS_PRIORITY1,
	GPTP_DEFAULT_DS_PRIORITY2,
	GPTP_DEFAULT_DS_DOMAIN_NUMBER,
	GPTP_DEFAULT_DS_CURRENT_TIME,
	GPTP_DEFAULT_DS_GM_CAPABLE,
	GPTP_DEFAULT_DS_CURRENT_UTC_OFFSET,
	GPTP_DEFAULT_DS_CURRENT_UTC_OFFSET_VALID,
	GPTP_DEFAULT_DS_LEAP59,
	GPTP_DEFAULT_DS_LEAP61,
	GPTP_DEFAULT_DS_TIME_TRACEABLE,
	GPTP_DEFAULT_DS_FREQUENCY_TRACEABLE,
	GPTP_DEFAULT_DS_PTP_TIMESCALE,
	GPTP_DEFAULT_DS_TIME_SOURCE,
	GPTP_DEFAULT_DS_MAX
} gptp_default_ds_leaf_t;

typedef enum {
	GPTP_CLOCK_QUALITY_CLOCK_CLASS = 0,
	GPTP_CLOCK_QUALITY_CLOCK_ACCURACY,
	GPTP_CLOCK_QUALITY_OFFSET_SCALED_LOG_VARIANCE,
	GPTP_CLOCK_QUALITY_MAX
} gptp_clock_quality_leaf_t;

typedef enum {
	GPTP_CURRENT_TIME_SECONDS_FIELD = 0,
	GPTP_CURRENT_TIME_NANOSECONDS_FIELD,
	GPTP_CURRENT_TIME_MAX
} gptp_current_time_leaf_t;

/**
* \ingroup managed-objects
* Current Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_CURRENT_DS_STEPS_REMOVED = 0,
	GPTP_CURRENT_DS_OFFSET_FROM_TIME_TRANSMITTER,
	GPTP_CURRENT_DS_MEAN_DELAY,
	GPTP_CURRENT_DS_LAST_GM_PHASE_CHANGE,
	GPTP_CURRENT_DS_LAST_GM_FREQ_CHANGE,
	GPTP_CURRENT_DS_GM_TIMEBASE_INDICATOR,
	GPTP_CURRENT_DS_GM_CHANGE_COUNT,
	GPTP_CURRENT_DS_TIME_OF_LAST_GM_CHANGE,
	GPTP_CURRENT_DS_TIME_OF_LAST_PHASE_CHANGE,
	GPTP_CURRENT_DS_TIME_OF_LAST_FREQ_CHANGE,
	GPTP_CURRENT_DS_MAX
} gptp_current_parameter_leaf_t;

/**
* \ingroup managed-objects
* Parent Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_PARENT_DS_PARENT_PORT_IDENTITY = 0,
	GPTP_PARENT_DS_PARENT_STATS,
	GPTP_PARENT_DS_GRANDMASTER_IDENTITY,
	GPTP_PARENT_DS_GRANDMASTER_CLOCK_QUALITY,
	GPTP_PARENT_DS_GRANDMASTER_PRIORITY1,
	GPTP_PARENT_DS_GRANDMASTER_PRIORITY2,
	GPTP_PARENT_DS_CUMULATIVE_RATE_RATIO,
	GPTP_PARENT_DS_MAX
} gptp_parent_ds_leaf_t;

typedef enum {
	GPTP_PORT_IDENTITY_CLOCK_IDENTITY = 0,
	GPTP_PORT_IDENTITY_PORT_NUMBER,
	GPTP_PORT_IDENTITY_MAX
} gptp_port_identity_leaf_t;

/**
* \ingroup managed-objects
* Time Properties Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET = 0,
	GPTP_TIME_PROPERTIES_DS_CURRENT_UTC_OFFSET_VALID,
	GPTP_TIME_PROPERTIES_DS_LEAP59,
	GPTP_TIME_PROPERTIES_DS_LEAP61,
	GPTP_TIME_PROPERTIES_DS_TIME_TRACEABLE,
	GPTP_TIME_PROPERTIES_DS_FREQUENCY_TRACEABLE,
	GPTP_TIME_PROPERTIES_DS_TIME_SOURCE,
	GPTP_TIME_PROPERTIES_DS_MAX
} gptp_time_properties_ds_leaf_t;

/**
* \ingroup managed-objects
* Ports container LEAFs enumerations
*/
typedef enum {
	GPTP_PORTS_PORT = 0,
	GPTP_PORTS_MAX
} gptp_ports_leaf_t;

/**
* \ingroup managed-objects
* Port list LEAFs enumerations
*/
typedef enum {
	GPTP_PORT_PORT_INDEX = 0,
	GPTP_PORT_PORT_DS,
	GPTP_PORT_PORT_STATS_DS,
	GPTP_PORT_MAX
} gptp_port_leaf_t;

/**
* \ingroup managed-objects
* Port Parameter Data Set container LEAFs enumerations
*/
typedef enum {
	GPTP_PORT_DS_PORT_IDENTITY = 0,
	GPTP_PORT_DS_PORT_STATE,
	GPTP_PORT_DS_MEAN_LINK_DELAY,
	GPTP_PORT_DS_LOG_ANNOUNCE_INTERVAL,
	GPTP_PORT_DS_ANNOUNCE_RECEIPT_TIMEOUT,
	GPTP_PORT_DS_LOG_SYNC_INTERVAL,
	GPTP_PORT_DS_VERSION_NUMBER,
	GPTP_PORT_DS_DELAY_ASYMMETRY,
	GPTP_PORT_DS_PORT_ENABLED,
	GPTP_PORT_DS_IS_MEASURING_DELAY,
	GPTP_PORT_DS_AS_CAPABLE,
	GPTP_PORT_DS_MEAN_LINK_DELAY_THRESH,
	GPTP_PORT_DS_NEIGHBOR_RATE_RATIO,
	GPTP_PORT_DS_INITIAL_LOG_ANNOUNCE_INTERVAL,
	GPTP_PORT_DS_CURRENT_LOG_ANNOUNCE_INTERVAL,
	GPTP_PORT_DS_INITIAL_LOG_SYNC_INTERVAL,
	GPTP_PORT_DS_CURRENT_LOG_SYNC_INTERVAL,
	GPTP_PORT_DS_SYNC_RECEIPT_TIMEOUT,
	GPTP_PORT_DS_SYNC_RECEIPT_TIMEOUT_TIME_INTERVAL,
	GPTP_PORT_DS_INITIAL_LOG_PDELAY_REQ_INTERVAL,
	GPTP_PORT_DS_CURRENT_LOG_PDELAY_REQ_INTERVAL,
	GPTP_PORT_DS_ALLOWED_LOST_RESPONSES,
	GPTP_PORT_DS_ALLOWED_FAULTS,
	GPTP_PORT_DS_MAX
} gptp_port_ds_leaf_t;

/**
* \ingroup managed-objects
* Port Parameter Statistics container LEAFs enumerations
*/
typedef enum {
	GPTP_PORT_STATS_DS_RX_SYNC_COUNT = 0,
	GPTP_PORT_STATS_DS_RX_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_RX_PDELAY_REQUEST_COUNT,
	GPTP_PORT_STATS_DS_RX_PDELAY_RESPONSE_COUNT,
	GPTP_PORT_STATS_DS_RX_PDELAY_RESPONSE_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_RX_ANNOUNCE_COUNT,
	GPTP_PORT_STATS_DS_RX_PACKET_DISCARD_COUNT,
	GPTP_PORT_STATS_DS_SYNC_RECEIPT_TIMEOUT_COUNT,
	GPTP_PORT_STATS_DS_ANNOUNCE_RECEIPT_TIMEOUT_COUNT,
	GPTP_PORT_STATS_DS_PDELAY_ALLOWED_LOST_RESPONSES_EXCEEDED_COUNT,
	GPTP_PORT_STATS_DS_TX_SYNC_COUNT,
	GPTP_PORT_STATS_DS_TX_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_TX_PDELAY_REQUEST_COUNT,
	GPTP_PORT_STATS_DS_TX_PDELAY_RESPONSE_COUNT,
	GPTP_PORT_STATS_DS_TX_PDELAY_RESPONSE_FOLLOW_UP_COUNT,
	GPTP_PORT_STATS_DS_TX_ANNOUNCE_COUNT,
	GPTP_PORT_STATS_DS_MAX
} gptp_port_statistics_ds_leaf_t;

#endif /* _GENAVB_PUBLIC_MANAGED_OBJECTS_GPTP_H_ */
