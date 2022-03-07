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

/**
 @file
 @brief FreeRTOS specific Network service implementation
 @details
*/

#ifndef _FREERTOS_NET_PORT_STATS_H_
#define _FREERTOS_NET_PORT_STATS_H_

typedef enum {
	/* IEEE 802.3 counters (Clause 30) */
	IEEE_OUT_FRAMES,		    /* aFramesTransmittedOK */
	IEEE_IN_FRAMES,			    /* aFramesReceivedOK */
	IEEE_IN_ERROR_CRC_FRAMES,	    /* aFrameCheckSequenceErrors */
	IEEE_IN_ERROR_ALIGN_FRAMES,	    /* aAlignmentErrors */
	IEEE_OUT_ERROR_MAC_INTERNAL_FRAMES, /* aFramesLostDueToIntMACXmitError */
	IEEE_IN_ERROR_MAC_INTERNAL_FRAMES,  /* aFramesLostDueToIntMACRcvError */
	IEEE_OUT_MC_FRAMES,		    /* aMulticastFramesXmittedOK */
	IEEE_OUT_BC_FRAMES,		    /* aBroadcastFramesXmittedOK */
	IEEE_IN_MC_FRAMES,		    /* aMulticastFramesReceivedOK */
	IEEE_IN_BC_FRAMES,		    /* aBroadcastFramesReceivedOK */
	IEEE_OUT_PAUSE_FRAMES,		    /* aPAUSEMACCtrlFramesTransmitted */
	IEEE_IN_PAUSE_FRAMES,		    /* aPAUSEMACCtrlFramesReceived */
	IEEE_OUT_LPI_TIME,		    /* aTransmitLPIMicroseconds */
	IEEE_IN_LPI_TIME,		    /* aReceiveLPIMicroseconds */
	IEEE_OUT_LPI_TRANSITIONS,	    /* aTransmitLPITransitions */
	IEEE_IN_LPI_TRANSITIONS,	    /* aReceiveLPITransitions */
	/* Remote monitoring (IETF RFC 2819) */
	RMON_IN_ERROR_UNDERSIZE_FRAMES, /* etherStatsUndersizePkts */
	RMON_IN_ERROR_OVERSIZE_FRAMES,	/* etherStatsOversizePkts */
	RMON_IN_ERROR_FRAGS_FRAMES,	/* etherStatsFragments */
	RMON_IN_64_FRAMES,		/* etherStatsPkts64Octets */
	RMON_IN_65_127_FRAMES,		/* etherStatsPkts65to127Octets */
	RMON_IN_128_255_FRAMES,		/* etherStatsPkts128to255Octets */
	RMON_IN_256_511_FRAMES,		/* etherStatsPkts256to511Octets */
	RMON_IN_512_1023_FRAMES,	/* etherStatsPkts512to1023Octets */
	RMON_IN_1024_MAX_FRAMES,	/* etherStatsPkts1024to1518Octets */
	RMON_OUT_64_FRAMES,		/* Not part of the spec but common */
	RMON_OUT_65_127_FRAMES,		/* Not part of the spec but common */
	RMON_OUT_128_255_FRAMES,	/* Not part of the spec but common */
	RMON_OUT_256_511_FRAMES,	/* Not part of the spec but common */
	RMON_OUT_512_1023_FRAMES,	/* Not part of the spec but common */
	RMON_OUT_1024_MAX_FRAMES,	/* Not part of the spec but common */
	ETH_STD_STATS_MAX
} eth_std_stats_t;

const char *net_port_stats_str(unsigned int id);

#endif /* _FREERTOS_NET_PORT_STATS_H_ */
