/*
 * Copyright 2016 Freescale Semiconductor, Inc.
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
 * @file alsa_config.h
 *
 * @brief This file defines data type for ALSA configurations.
 *
 * @details    Copyright 2016 Freescale Semiconductor, Inc.
 */

#ifndef __ALSA_CONFIG_H__
#define __ALSA_CONFIG_H__

/**
 * @addtogroup aar
 * @{
 */

enum {
	ALSA_CAPTURE0 = 0,
	ALSA_CAPTURE1,
	ALSA_CAPTURE2,
	ALSA_CAPTURE3,
	ALSA_CAPTURE4,
	ALSA_CAPTURE5,
	MAX_ALSA_CAPTURE
};

enum {
	ALSA_PLAYBACK0 = 0,
	ALSA_PLAYBACK1,
	ALSA_PLAYBACK2,
	ALSA_PLAYBACK3,
	ALSA_PLAYBACK4,
	ALSA_PLAYBACK5,
	MAX_ALSA_PLAYBACK
};

/** ALSA device parameter structure */
typedef struct _ALSA_DEVICE_PARAMETER {
	unsigned int period_time_ns;           /**< ALSA device period */
	int          format;                   /**< Audio data format */
	unsigned int fifo_threshold;           /**< FIFO threshold below which the ESAI block triggers a transfer request, in bytes.
							Only used to correct the reported ALSA latency statistics. */
	unsigned int pcm_start_delay;         /**< Estimate of Alsa snd_pcm_start overhead, in nanoseconds. This is used to correct
							the amount of silence frames that need to be added before calling snd_pcm_start,
							and improve the accuracy of the playback time of audio samples. */
} aar_alsa_param_t;

extern const char alsa_playback_device_names[MAX_ALSA_PLAYBACK][15];
extern const char alsa_capture_device_names[MAX_ALSA_CAPTURE][15];

#endif /* __ALSA_CONFIG_H__ */
