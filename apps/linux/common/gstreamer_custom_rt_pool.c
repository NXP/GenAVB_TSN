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

#define _GNU_SOURCE

#include <pthread.h>
#include "gstreamer_custom_rt_pool.h"

/* --- standard type macros --- */
#define TYPE_AVB_RT_POOL             (avb_rt_pool_get_type ())
#define AVB_RT_POOL(pool)            (G_TYPE_CHECK_INSTANCE_CAST ((pool), TYPE_AVB_RT_POOL, AvbRTPool))
#define TEST_IS_AVB_RT_POOL(pool)         (G_TYPE_CHECK_INSTANCE_TYPE ((pool), TYPE_AVB_RT_POOL))
#define AVB_RT_POOL_CLASS(pclass)    (G_TYPE_CHECK_CLASS_CAST ((pclass), TYPE_AVB_RT_POOL, AvbRTPoolClass))
#define TEST_IS_AVB_RT_POOL_CLASS(pclass) (G_TYPE_CHECK_CLASS_TYPE ((pclass), TYPE_AVB_RT_POOL))
#define AVB_RT_POOL_GET_CLASS(pool)  (G_TYPE_INSTANCE_GET_CLASS ((pool), TYPE_AVB_RT_POOL, AvbRTPoolClass))
#define AVB_RT_POOL_CAST(pool)       ((AvbRTPool*)(pool))

typedef struct _AvbRTPool {
	GstTaskPool    object;
} AvbRTPool;

typedef struct _AvbRTPoolClass {
	GstTaskPoolClass parent_class;
} AvbRTPoolClass;

typedef struct _AvbRTthreadID {
	pthread_t thread;
} AvbRTthreadID;

GType           avb_rt_pool_get_type    (void);

G_DEFINE_TYPE (AvbRTPool, avb_rt_pool, GST_TYPE_TASK_POOL);

static void avb_rt_pool_finalize(GObject * object);

static void avb_rt_thread_pool_prepare(GstTaskPool * pool, GError ** error)
{
}

static void avb_rt_thread_pool_cleanup(GstTaskPool * pool)
{
}

static gpointer avb_rt_thread_push(GstTaskPool * pool, GstTaskPoolFunction func, gpointer data, GError ** error)
{
	AvbRTthreadID *tid;
	gint res;
	pthread_attr_t attr;
	struct sched_param param;

	tid = g_slice_new0(AvbRTthreadID);

	pthread_attr_init(&attr);

	if ((res = pthread_attr_setschedpolicy(&attr, GST_THREADS_SCHED_POLICY)) != 0)
		g_warning("setschedpolicy: failure: %p", g_strerror (res));

	param.sched_priority = GST_THREADS_PRIORITY;

	if ((res = pthread_attr_setschedparam(&attr, &param)) != 0)
		g_warning("setschedparam: failure: %p", g_strerror (res));

	if ((res = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) != 0)
		g_warning("setinheritsched: failure: %p", g_strerror (res));

	res = pthread_create(&tid->thread, &attr, (void *(*)(void *)) func, data);

	if (res != 0) {
		g_set_error(error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN,
		"Error creating thread: %s", g_strerror (res));
		g_slice_free(AvbRTthreadID, tid);
		tid = NULL;
	}

	return tid;
}

static void avb_rt_thread_join(GstTaskPool * pool, gpointer id)
{
	AvbRTthreadID *tid = (AvbRTthreadID *) id;

	pthread_join(tid->thread, NULL);

	g_slice_free(AvbRTthreadID, tid);
}

static void avb_rt_pool_class_init(AvbRTPoolClass * klass)
{
	GstTaskPoolClass *gsttaskpool_class;
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass *) klass;
	gsttaskpool_class = (GstTaskPoolClass *) klass;

	gobject_class->finalize = GST_DEBUG_FUNCPTR(avb_rt_pool_finalize);

	gsttaskpool_class->prepare = avb_rt_thread_pool_prepare;
	gsttaskpool_class->cleanup = avb_rt_thread_pool_cleanup;
	gsttaskpool_class->push = avb_rt_thread_push;
	gsttaskpool_class->join = avb_rt_thread_join;
}

static void avb_rt_pool_init(AvbRTPool * pool)
{
}

static void avb_rt_pool_finalize(GObject * object)
{
	G_OBJECT_CLASS(avb_rt_pool_parent_class)->finalize(object);
}

GstTaskPool * avb_rt_pool_new(void)
{
	GstTaskPool *pool;

	pool = g_object_new(TYPE_AVB_RT_POOL, NULL);

	return pool;
}
