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

#include <inttypes.h>

#include "log.h"
#include "time.h"

#define DEFAULT_LOG_DIR "/var/log/"
#define DEFAULT_LOG_FILE "avb_media_app"

FILE *g_logfile_hdl = NULL;
char g_timestamp_str[TS_STR_LEN] = {0};

int aar_log_init(char *log_file_path)
{
#if LOG_LEVEL > VERBOSE_NONE
	if (!log_file_path) /* no log file specified, use default. */
		g_logfile_hdl = fopen(DEFAULT_LOG_DIR DEFAULT_LOG_FILE, "a");
	else
		g_logfile_hdl = fopen(log_file_path, "a");

	if (!g_logfile_hdl)
		return -1;

	setlinebuf(g_logfile_hdl);
#endif
	return 0;
}

void aar_log_exit(void)
{
	if (g_logfile_hdl) {
		// Close log file and clear file handle.
		fclose(g_logfile_hdl);
		g_logfile_hdl = NULL;
	}
}

void aar_log_update_time(genavb_clock_id_t clock_id)
{
	uint64_t now;

	if (genavb_clock_gettime64(clock_id, &now) < 0)
		return;

	snprintf(g_timestamp_str, TS_STR_LEN, "%" PRIu64 "", now / NSECS_PER_SEC);
}
