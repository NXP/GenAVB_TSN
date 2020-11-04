/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
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

#ifndef _FILE_BUFFER_H_
#define _FILE_BUFFER_H_

#define PES_SIZE	188

#define FILE_BUFFER_SIZE (1024 * 10 * PES_SIZE)
#define FILE_BUFFER_READERS	2

#ifdef __cplusplus
extern "C" {
#endif

struct file_buffer {
	unsigned char buf[FILE_BUFFER_SIZE];
	unsigned int size;

	unsigned int w_offset;

	unsigned int r_offset[FILE_BUFFER_READERS];

	unsigned int eof;
	unsigned int readers;
};

/* initialize a buffer that can handle "readers" number of readers */
void file_buffer_init(struct file_buffer *buf, unsigned int readers);

unsigned int file_buffer_available(struct file_buffer *buf, int reader);
unsigned int file_buffer_available_wrap(struct file_buffer *buf, int reader);

/* return true if buffer is empty */
int file_buffer_empty(struct file_buffer *buf, int reader);

unsigned int file_buffer_free(struct file_buffer *buf, int reader);

void *file_buffer_buf(struct file_buffer *buf, int reader);

void file_buffer_read(struct file_buffer *buf, int reader, unsigned int len);

int file_buffer_write(struct file_buffer *buf, unsigned int fd, unsigned int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_BUFFER_H_ */
