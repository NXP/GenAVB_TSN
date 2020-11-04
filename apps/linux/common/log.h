/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2020 NXP
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

#ifndef __COMMON_LOG_H__
#define __COMMON_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <genavb/clock.h>

#define VERBOSE_NONE  (0)
#define VERBOSE_ERROR (1)
#define VERBOSE_INFO  (2)
#define VERBOSE_DEBUG (3)


#ifndef PRINT_LEVEL
#define PRINT_LEVEL VERBOSE_NONE
#endif /* PRINT_LEVEL */

#ifndef LOG_LEVEL
#define LOG_LEVEL  VERBOSE_INFO
#endif /* LOG_LEVEL */

#define _LHF g_logfile_hdl
#define _LTS g_timestamp_str

#define DBG_FILE_BASE(fmt, args...)                                                  \
	if (_LHF) {                                                                  \
		fprintf(_LHF, "\nDBG  %-11s %-32.32s " fmt, _LTS, __FUNCTION__, ##args); \
	}
#define INF_FILE_BASE(fmt, args...)                                                  \
	if (_LHF) {                                                                  \
		fprintf(_LHF, "\nINFO %-11s %-32.32s " fmt, _LTS, __FUNCTION__, ##args); \
	}
#define _INF_FILE_BASE(fmt, args...)                                                 \
	if (_LHF) {                                                                  \
		fprintf(_LHF, fmt, ##args);                                          \
	}
#define ERR_FILE_BASE(fmt, args...)                                                  \
	if (_LHF) {                                                                  \
		fprintf(_LHF, "\nERR  %-11s %-32.32s " fmt, _LTS, __FUNCTION__, ##args); \
	}

#define DBG_OUT_BASE(fmt, args...) printf("\nDBG  %-11s %-32.32s " fmt, _LTS, __FUNCTION__, ##args)
#define INF_OUT_BASE(fmt, args...) printf("\nINFO %-11s %-32.32s " fmt, _LTS, __FUNCTION__, ##args)
#define _INF_OUT_BASE(fmt, args...) printf(fmt, ##args)
#define ERR_OUT_BASE(fmt, args...) printf("\nERR  %-11s %-32.32s " fmt, _LTS, __FUNCTION__, ##args)

#if LOG_LEVEL >= VERBOSE_DEBUG
#define DBG_LOG(fmt,args...) DBG_FILE_BASE(fmt, ##args)
#else
#define DBG_LOG(fmt,args...)
#endif /* LOG_LEVEL >= VERBOSE_DEBUG */

#if LOG_LEVEL >= VERBOSE_INFO
#define INF_LOG(fmt,args...) INF_FILE_BASE(fmt, ##args)
#define _INF_LOG(fmt,args...) _INF_FILE_BASE(fmt, ##args)
#else
#define INF_LOG(fmt,args...)
#define _INF_LOG(fmt,args...)
#endif /* LOG_LEVEL >= VERBOSE_INFO */

#if LOG_LEVEL >= VERBOSE_ERROR
#define ERR_LOG(fmt,args...) ERR_FILE_BASE(fmt, ##args)
#else
#define ERR_LOG(fmt,args...)
#endif /* LOG_LEVEL >= VERBOSE_ERROR */

#if PRINT_LEVEL >= VERBOSE_DEBUG
#define DBG_OUT(fmt,args...) DBG_OUT_BASE(fmt, ##args)
#else
#define DBG_OUT(fmt,args...)
#endif /* PRINT_LEVEL >= VERBOSE_DEBUG */

#if PRINT_LEVEL >= VERBOSE_INFO
#define INF_OUT(fmt,args...) INF_OUT_BASE(fmt, ##args)
#define _INF_OUT(fmt,args...) INF_OUT_BASE(fmt, ##args)
#else
#define INF_OUT(fmt,args...)
#define _INF_OUT(fmt,args...)
#endif /* PRINT_LEVEL >= VERBOSE_INFO */

#if PRINT_LEVEL >= VERBOSE_ERROR
#define ERR_OUT(fmt,args...) ERR_OUT_BASE(fmt, ##args)
#else
#define ERR_OUT(fmt,args...)
#endif /* PRINT_LEVEL >= VERBOSE_ERROR */

#define DBG(fmt,args...) do{DBG_OUT(fmt,##args);DBG_LOG(fmt,##args);}while(0)
#define INF(fmt,args...) do{INF_OUT(fmt,##args);INF_LOG(fmt,##args);}while(0)
#define _INF(fmt,args...) do{_INF_OUT(fmt,##args);_INF_LOG(fmt,##args);}while(0)
#define ERR(fmt,args...) do{ERR_OUT(fmt,##args);ERR_LOG(fmt,##args);}while(0)

#define STREAM_STR_FMT "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x"
#define STREAM_STR(_stream_id) _stream_id[0], _stream_id[1], _stream_id[2], _stream_id[3], _stream_id[4], _stream_id[5], _stream_id[6], _stream_id[7]

#define MAC_STR_FMT "%02x-%02x-%02x-%02x-%02x-%02x"
#define MAC_STR(_dst_mac) _dst_mac[0], _dst_mac[1], _dst_mac[2], _dst_mac[3], _dst_mac[4], _dst_mac[5]

#define TS_STR_LEN 30
extern FILE *g_logfile_hdl;
extern char g_timestamp_str[TS_STR_LEN];

int aar_log_init(char *log_file_path);
void aar_log_exit(void);
void aar_log_update_time(genavb_clock_id_t clock_id);

#endif /* __COMMON_LOG_H__ */
