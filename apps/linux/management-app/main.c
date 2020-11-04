/*
 * Copyright 2018 NXP.
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
 @brief GenAVB management application
 @details

 Copyright 2018 NXP.
 All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include <genavb/genavb.h>
#include "common.h"

extern int gptp_main(struct genavb_handle *avb_h, int argc, char *argv[]);
extern int srp_main(struct genavb_handle *avb_h, int argc, char *argv[]);

int main(int argc, char *argv[])
{
	struct genavb_handle *avb_h = NULL;
	int rc;

	rc = genavb_init(&avb_h, 0);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_init() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto exit;
	}

	if (argc < 2) {
		usage();
		goto exit;
	}

	if (!strcmp("ptp", argv[1]) ) {
		rc = gptp_main(avb_h, argc - 1, argv + 1);
	} else if (!strcmp("srp", argv[1]))  {
		rc = srp_main(avb_h, argc - 1 , argv + 1);
	} else {
		usage();
		goto exit;
	}

exit:
	if (avb_h)
		genavb_exit(avb_h);

	return rc;
}
