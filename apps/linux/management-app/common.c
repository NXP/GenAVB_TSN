/*
 * Copyright 2020 NXP.
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
 @file
 @brief GenAVB gptp management application
 @details

 Copyright 2020 NXP.
 All Rights Reserved.
 */

#include <stdio.h>
#include <genavb/genavb.h>

void usage (void)
{
	printf("\nUsage:\nmanagement-app [ptp|srp] [options]\n");
	printf("\nOptions:\n"
		"\nCommon options:\n"
		"\t-E                    manages endpoint stack (default)\n"
		"\t-B                    manages bridge stack\n"
		"\t-S                    sets managed object\n"
		"\t-G                    gets managed object (default)\n"
		"\t-P <port>             manages port <port> (default 0)\n"
		"\t-D <direction>        manages direction <direction> (default 0: talker, 1: listener)\n"
		"\t-h                    print this help text\n"
		"\nPTP options:\n"
		"\t-p <priority1>        set/get gPTP priority1 value\n"
		"\t-s <state>            set/get gPTP port state\n"
		"\t-d                    dump port stats\n"
		"\nSRP options:\n"
		"\t-s <stream_id>        set/get streams table (default wilcard if <streamID> omitted)\n"
		"\t-r <stream_id>        set/get reservations table per <port> <direction> <stream_id> (default wilcard if <stream_id> omitted)\n"
		"\t-b                    set/get bridge base table\n"
		"\t-p                    set/get bridge port table\n"
		"\t-l <traffic_class>    set/get latency parameter table\n");
}

int check_response(uint16_t *expected, uint16_t *response, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (expected[i] != response[i]) {
			return -1;
		}

	return 0;
}

uint8_t *get_response_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status)
{
	*id = ((uint16_t *)buf)[0];
	*length = ((uint16_t *)buf)[1];
	*status = ((uint16_t *)buf)[2];

	buf += 6;

	return buf;
}

uint8_t *get_node_next(uint8_t *buf, uint16_t length)
{
	buf += length -2;

	return buf;
}

uint8_t *get_node_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status)
{
	buf = get_response_header(buf, id, length, status);

	return buf;
}

