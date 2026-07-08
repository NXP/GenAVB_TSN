/*
* Copyright 2018, 2020-2021, 2023, 2025-2026 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief GPTP managed objects definition
 @details
*/

#ifndef _GPTP_MANAGED_OBJECTS_H_
#define _GPTP_MANAGED_OBJECTS_H_

#include "common/managed_objects.h"

#define GPTP_MAX_NODES 1

#define GPTP_INSTANCE_NUM_LEAVES 6
#define GPTP_DEFAULT_DS_NUM_LEAVES 16
#define GPTP_CURRENT_DS_NUM_LEAVES 10
#define GPTP_PARENT_DS_NUM_LEAVES 9
#define GPTP_TIME_PROPERTIES_DS_NUM_LEAVES 7
#define GPTP_PORTS_NUM_LEAVES 1
#define GPTP_PORT_NUM_LEAVES 3
#define GPTP_PORT_DS_NUM_LEAVES 24
#define GPTP_PORT_STATS_DS_NUM_LEAVES 18

#define GPTP_CLOCK_QUALITY_NUM_LEAVES 3
#define GPTP_CURRENT_TIME_NUM_LEAVES 2
#define GPTP_PORT_IDENTITY_NUM_LEAVES 2

/* Managed object tree definition, IEEE 802.1AS-2011, section 14 */
/* YANG module ieee1588-ptp-tt augmented by ieee802-dot1as-gptp */
MODULE(gptp_managed_objects, GPTP_MAX_NODES,
	LIST(instanceList, CFG_MAX_GPTP_DOMAINS,
		LIST_ENTRY(instance, CFG_MAX_GPTP_DOMAINS, GPTP_INSTANCE_NUM_LEAVES,

			LEAF(instanceIndex);

			CONTAINER(default_ds, GPTP_DEFAULT_DS_NUM_LEAVES,
				LEAF(clockIdentity);
				LEAF(numberPorts);

				CONTAINER(clock_quality, GPTP_CLOCK_QUALITY_NUM_LEAVES,
					LEAF(clockClass);
					LEAF(clockAccuracy);
					LEAF(offsetScaledLogVariance);
				);

				LEAF(priority1);
				LEAF(priority2);
				LEAF(domainNumber);

				CONTAINER(current_time, GPTP_CURRENT_TIME_NUM_LEAVES,
					LEAF(secondsField);
					LEAF(nanosecondsField);
				);

				LEAF(gmCapable);
				LEAF(currentUtcOffset);
				LEAF(currentUtcOffetValid);
				LEAF(leap59);
				LEAF(leap61);
				LEAF(timeTraceable);
				LEAF(frequencyTraceable);
				LEAF(ptpTimescale);
				LEAF(timeSource);
			);

			CONTAINER(current_ds, GPTP_CURRENT_DS_NUM_LEAVES,
				LEAF(stepsRemoved);
				LEAF(offsetFromMaster);
				LEAF(meanDelay);
				LEAF(lastGmPhaseChange);
				LEAF(lastGmFreqChange);
				LEAF(gmTimebaseIndicator);
				LEAF(gmChangeCount);
				LEAF(timeOfLastGmChangeEvent);
				LEAF(timeOfLastGmPhaseChangeEvent);
				LEAF(timeOfLastGmFreqChangeEvent);
			);

			CONTAINER(parent_ds, GPTP_PARENT_DS_NUM_LEAVES,
				CONTAINER(parentPortIdentity, GPTP_PORT_IDENTITY_NUM_LEAVES,
					LEAF(clockIdentity);
					LEAF(portNumber);
				);

				LEAF(parentStats);
				LEAF(parentOffsetVariance);
				LEAF(parentPhaseChangeRate);
				LEAF(grandMasterIdentity);

				CONTAINER(grandmaster_clock_quality, GPTP_CLOCK_QUALITY_NUM_LEAVES,
					LEAF(clockClass);
					LEAF(clockAccuracy);
					LEAF(offsetScaledLogVariance);
				);

				LEAF(grandMasterPriority1);
				LEAF(grandMasterPriority2);
				LEAF(cumulativeRateRatio);
			);

			CONTAINER(time_properties_ds, GPTP_TIME_PROPERTIES_DS_NUM_LEAVES,
				LEAF(currentUtcOffset);
				LEAF(currentUtcOffsetValid);
				LEAF(leap59);
				LEAF(leap61);
				LEAF(timeTraceable);
				LEAF(frequencyTraceable);
				LEAF(timeSource);
			);

			CONTAINER(ports, GPTP_PORTS_NUM_LEAVES,
				LIST(portList, CFG_GPTP_MAX_NUM_PORT,
					LIST_ENTRY(port, CFG_GPTP_MAX_NUM_PORT, GPTP_PORT_NUM_LEAVES,

						LEAF(portIndex);

						CONTAINER(port_ds, GPTP_PORT_DS_NUM_LEAVES,
							CONTAINER(portIdentity, GPTP_PORT_IDENTITY_NUM_LEAVES,
								LEAF(clockIdentity);
								LEAF(portNumber);
							);
							LEAF(portRole);
							LEAF(meanLinkDelay);
							LEAF(logAnnounceInterval);
							LEAF(announceReceiptTimeout);
							LEAF(logSyncInterval);
							LEAF(versionNumber);
							LEAF(delayAsymmetry);
							LEAF(pttPortEnabled);
							LEAF(isMeasuringDleay);
							LEAF(asCapable);
							LEAF(neighborPropDelayThresh);
							LEAF(neighborRateRatio);
							LEAF(initialLogAnnounceInterval);
							LEAF(currentLogAnnounceInterval);
							LEAF(initialLogSyncInterval);
							LEAF(currentLogSyncInterval);
							LEAF(syncReceiptTimeout);
							LEAF(syncReceiptTimeoutTimeInterval);
							LEAF(initialLogPdelayReqInterval);
							LEAF(currentLogPdelayReqInterval);
							LEAF(allowedLostResponses);
							LEAF(allowedFaults);
						);

						CONTAINER(port_statistics_ds, GPTP_PORT_STATS_DS_NUM_LEAVES,
							LEAF(rxSyncCount);
							LEAF(rxFollowUpCount);
							LEAF(rxPdelayRequestCount);
							LEAF(rxPdelayResponseCount);
							LEAF(rxPdelayResponseFollowUpCount);
							LEAF(rxAnnounceCount);
							LEAF(rxPTPPacketDiscardCount);
							LEAF(syncReceiptTimeoutCount);
							LEAF(announceReceiptTimeoutCount);
							LEAF(pdelayAllowedLostResponsesExceededCount);
							LEAF(txSyncCount);
							LEAF(txFollowUpCount);
							LEAF(txPdelayRequestCount);
							LEAF(txPdelayResponseCount);
							LEAF(txPdelayResponseFollowUpCount);
							LEAF(txAnnounceCount);
						);
					);
				);
			);
		);
	);
);


struct gptp_ctx;

void gptp_managed_objects_init(struct gptp_managed_objects *module, struct gptp_ctx *gptp);
unsigned int gptp_managed_objects_get(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end);
unsigned int gptp_managed_objects_set(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end);

#endif /* _GPTP_MANAGED_OBJECTS_H_ */
