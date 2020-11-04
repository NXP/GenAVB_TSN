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


/**
 @file
 @brief clock and time service interface
 @details

 Copyright 2015 Freescale Semiconductor, Inc.
 All Rights Reserved.
*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

/** Initialize the PTP clock framework for a given Ethernet port.
 * \param port_id	id of the Ethernet port.
 * \return	0 on success or -1 on error.
 */
int clock_init(int port_id);

/** Close the PTP clock framework for a given Ethernet port.
 * \param port_id	id of the Ethernet port.
 * \return	0 on success or -1 on error.
 */
int clock_exit(int port_id);

/**
 * Get GPTP time in nanoseconds modulo 2^32.
 * \param port_id	id of the Ethernet port from which PTP clock is to be read.
 * \param ns	pointer to u32 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int clock_gettime32(int port_id, unsigned int *ns);

/**
 * Get GPTP time in nanoseconds.
 * \param port_id	id of the Ethernet port from which PTP clock is to be read.
 * \param ns	pointer to u64 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int clock_gettime64(int port_id, unsigned long long *ns);

#endif /* _OS_CLOCK_H_ */
