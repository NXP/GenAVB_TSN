/*
 * Copyright 2018, 2020 NXP
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
 \brief OS specific GenAVB public API
 \details OS specific API definition for the GenAVB library

 \copyright Copyright 2018, 2020 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_STREAMING_API_H_
#define _OS_GENAVB_PUBLIC_STREAMING_API_H_

/** Sets a callback, used by GenAVB stack, to report stream related events.
 * The callback must not block and should do minimal processing of events, typically just schedule
 * another task to do the actual event handling. The callback is disabled after each call and must
 * be explicitly re-enabled by calling (::genavb_stream_enable_callback).
 *
 * GenAVB stack will call the callback function when:
 * * In case of listener stream: amount of stream data available is >= than stream batch_size (::genavb_stream_create),
 *   this means that application can read batch_size of media data (::genavb_stream_receive)
 * * In case of talker stream: amount of available space is >= batch_size, which means
 *   that application can send batch_size of media data (::genavb_stream_send)
 *
 * \ingroup stream
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param handle	genavb stream handle
 * \param callback	callback function
 * \param data		callback data
 */
int genavb_stream_set_callback(struct genavb_stream_handle const *handle, int (*callback)(void *), void *data);

/** Enable stream callback.
 * Enabling callback is valid only for one event, so the application needs to re-enable it after each event.
 * In addition, if two first conditions from ::genavb_stream_set_callback are satisfied this function will call directly stream callback.
 * \ingroup stream
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param handle	genavb stream handle
 */
int genavb_stream_enable_callback(struct genavb_stream_handle const *handle);

#endif /* _OS_GENAVB_PUBLIC_STREAMING_API_H_ */

