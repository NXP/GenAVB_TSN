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
 \file log.h
 \brief GenAVB public logging API
 \details logging API definition for the GenAVB library

 \copyright Copyright 2020 NXP
*/

#ifndef _GENAVB_PUBLIC_LOG_API_H_
#define _GENAVB_PUBLIC_LOG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \cond FREERTOS
 */

/**
 * \ingroup generic
 * GenAVB log level. Messages at or below the configured log level will be printed in the log file.
 */
typedef enum {
	GENAVB_LOG_LEVEL_CRIT = 0, /**< Critical messages. Lowest verbosity level */
	GENAVB_LOG_LEVEL_ERR,	   /**< Error messages. */
	GENAVB_LOG_LEVEL_INIT,	   /**< Initialization messages. */
	GENAVB_LOG_LEVEL_INFO,	   /**< Informational messages. */
	GENAVB_LOG_LEVEL_DEBUG	   /**< Debug messages. Highest verbosity level */
} genavb_log_level_t;

/**
 * \ingroup generic
 * GenAVB log component ID.
 */
typedef enum {
	GENAVB_LOG_COMPONENT_ID_AVTP = 0, /**< AVTP component */
	GENAVB_LOG_COMPONENT_ID_AVDECC,	  /**< AVDECC component */
	GENAVB_LOG_COMPONENT_ID_SRP,	  /**< SRP component */
	GENAVB_LOG_COMPONENT_ID_MAAP,	  /**< MAAP component */
	GENAVB_LOG_COMPONENT_ID_COMMON,	  /**< common layer */
	GENAVB_LOG_COMPONENT_ID_OS,	  /**< OS specific layer*/
	GENAVB_LOG_COMPONENT_ID_GPTP,	  /**< gPTP component */
	GENAVB_LOG_COMPONENT_ID_API,	  /**< API component */
	GENAVB_LOG_COMPONENT_ID_MGMT,	  /**< Management component */
} genavb_log_component_id_t;

/**
 * \endcond
 */

/* OS specific headers */
#include "os/log.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_LOG_API_H_ */
