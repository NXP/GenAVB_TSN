/*
 * Copyright 2020 NXP
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
 \file stats.h
 \brief OS specific GenAVB stats API
 \details OS specific stats API definition for the GenAVB library

 \copyright Copyright 2020 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_STATS_API_H_
#define _OS_GENAVB_PUBLIC_STATS_API_H_

/** Get the number of available stats
 *
 * \ingroup		generic
 * \return 		number of available stats or negative error code.
 * \param port_id	logical port number
 */
int genavb_port_stats_get_number(unsigned int port_id);

/** Get the statistics names
 *
 * \ingroup		generic
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param buf		pointer to buffer which will be used to write statistics
 * 			names (array of GENAVB_PORT_STATS_STR_LEN bytes length strings)
 * \param buf_len	buffer length. At least GENAVB_PORT_STATS_STR_LEN * (stats number) bytes.
 */
int genavb_port_stats_get_strings(unsigned int port_id, char *buf, unsigned int buf_len);

/** Get the statistics values
 *
 * \ingroup 		generic
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param buf		pointer to buffer which will be used to write statistics
 * 			values (array of uint64_t values)
 * \param buf_len	buffer length. At least sizeof(uint64_t) * (stats number) bytes.
 */
int genavb_port_stats_get(unsigned int port_id, uint64_t *buf, unsigned int buf_len);

#endif /* _OS_GENAVB_PUBLIC_STATS_API_H_ */
