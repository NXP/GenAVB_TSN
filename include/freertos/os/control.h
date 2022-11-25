/*
 * Copyright 2018 NXP
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
 \file control.h
 \brief OS specific GenAVB public control API
 \details OS specific control API definition for the GenAVB library

 \copyright Copyright 2018 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_CONTROL_API_H_
#define _OS_GENAVB_PUBLIC_CONTROL_API_H_

/** Sets notification callback for a given control channel.
 * The callback must not block and should do minimal processing of events, typically just schedule
 * another task to do the actual event handling. The callback is disabled after each call and must
 * be explicitly re-enabled by calling (::genavb_control_enable_callback).
 *
 * GenAVB stack will call the callback function when:
 * A control receive message is available, the application should call ::genavb_control_receive()
 *
 * \ingroup control
 * \return 	::GENAVB_SUCCESS, or negative error code.
 * \param 	handle control handle returned by ::avb_control_open.
 * \param 	callback application callback function.
 * \param 	data application private data.
 */
int genavb_control_set_callback(struct genavb_control_handle *handle, int (*callback)(void *), void *data);

/** Enables notification callback for a given control channel.
 * Enabling callback is valid only for one event, so the application needs to re-enable it after each event.
 * In addition if a message is available this function will call directly the control callback.
 * \ingroup control
 * \return 	::GENAVB_SUCCESS, or negative error code.
 * \param 	handle control handle returned by ::avb_control_open.
 */
int genavb_control_enable_callback(struct genavb_control_handle *handle);

#endif /* _OS_GENAVB_PUBLIC_CONTROL_API_H_ */

