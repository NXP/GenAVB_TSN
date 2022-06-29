/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018, 2021 NXP
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
 \file genavb.h
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
            Copyright 2017-2018, 2021 NXP
*/

/**
*
* \defgroup library		Library API
* GenAVB public API definition.
* The public API is made of the following subsets:
* * \ref init "Initialization functions" (open/close library, etc)
* * \ref stream "Streaming functions": create/destroy a stream, send/receive data
* * \ref control "Control functions": open a control handle, send/receive commands through it
* * \ref socket "Socket functions": open/close a socket, send/receive network packets
* * \ref clock "Clock functions": retrieve time
* \if FREERTOS
* * \ref timer "Timer functions": create/start/stop timers
* * \ref qos "QoS functions": set/get network QoS configuration
* \endif
* * \ref generic "Generic functions" (logging, etc)
*
*
* \defgroup other		Additional definitions
* Additional definitions
* * [Network protocol headers](@ref protocol)
* * Kernel includes
*
* \defgroup init 		Init
* \ingroup library
*
* \defgroup stream		Streaming
* \ingroup library
*
* \defgroup control 		Control
* \ingroup library
*
* \defgroup socket		Socket
* \ingroup library
*
* \defgroup clock		Clock
* \ingroup library
*
* \if FREERTOS
* \defgroup timer		Timer
* \ingroup library
*
* \defgroup qos			802.1Q qos
* \ingroup library
* \endif
*
* \defgroup generic 		Generic
* \ingroup library
*
* \defgroup protocol		Network protocol headers
* \ingroup other
*
* \defgroup aem			AEM protocol
* AVDECC AECP AEM protocol definitions, from IEEE 1722.1-2013.
* Note: the description of AEM PDUs is based on the official protocol specification. For details on actual usage of those structures
* with the GenAVB stack control API, see @ref control_usage.
* \ingroup protocol
*/

#ifndef _GENAVB_PUBLIC_API_H_
#define _GENAVB_PUBLIC_API_H_

#include "init.h"
#include "control_avdecc.h"
#include "control_srp.h"
#include "control_clock_domain.h"
#include "control_gptp.h"
#include "control_maap.h"
#include "streaming.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "error.h"
#include "avdecc.h"
#include "srp.h"
#include "avtp.h"
#include "sr_class.h"


#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_API_H_ */
