/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2019, 2021 NXP
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

#ifndef _QUEUE_COMMON_H_
#define _QUEUE_COMMON_H_

/**
 * DOC: Queue implementation
 *
 * Basic queue implementation supporting lockless single reader/single writer.
 * To support multiple readers or multiple writers the user of the queue
 * needs to add it's own reader/write side locking.
 * The queue entries are the size of a pointer and usually store pointers to buffers.
 */

#ifndef QUEUE_ENTRIES_MAX
#error "OS specific code must define queue size"
#endif

/**
 * struct queue - Queue structure
 * @size - queue size (in number of entries)
 * @flags - queue flags
 * @read - atomic queue read pointer
 * @write - atomic queue write pointer
 * @entry - queue entry storage
 *
 * It's possible to use a bigger queue size at run time by allocating a bigger memory area
 * for the queue structure.
 *
 */
struct queue {
	unsigned int size;
	atomic_t read;
	atomic_t write;
	void (*entry_free)(void *data, unsigned long entry);
	unsigned long entry[QUEUE_ENTRIES_MAX]; /* Placed last so that queue user can allocate bigger size */
};

void queue_flush(struct queue *q, void *data);

static inline void queue_init(struct queue *q, void (*entry_free)(void *data, unsigned long entry))
{
	q->size = QUEUE_ENTRIES_MAX;
	q->entry_free = entry_free;
	atomic_set(&q->read, 0);
	atomic_set(&q->write, 0);
}

/**
 * queue_pending() -
 * @q - pointer to queue structure
 *
 * Return: number of pending entries (that can be dequeued)
 *
 */
static inline unsigned int queue_pending(struct queue *q)
{
	u32 read = atomic_read(&q->read);
	u32 write = atomic_read(&q->write);

	if (write >= read)
		return write - read;
	else
		return (write + q->size) - read;
}

/**
 * queue_available() -
 * @q - pointer to queue structure
 *
 * Return: number of available free entries (that can be queued)
 *
 */
static inline unsigned int queue_available(struct queue *q)
{
	return q->size - 1 - queue_pending(q);
}


static inline int queue_full(struct queue *q)
{
	return !queue_available(q);
}

static inline int queue_empty(struct queue *q)
{
	return !queue_pending(q);
}

static inline void __queue_incr(struct queue *q, u32 *index)
{
	(*index)++;
	if (*index >= q->size)
		*index = 0;
}

/**
 * queue_enqueue_init() - optimized queueing init
 * @q - pointer to queue structure
 * @write - caller provided storage to track optimized queueing operation
 *
 * queue_enqueue_init()/queue_enqueue_next()/queue_enqueue_done() provide
 * an optimized interface to queue several entries in a single go.
 * The functions must be called in that order. queue_enqueue_init()/queue_enqueue_done()
 * must be called once, queue_enqueue_next() may be called several times.
 * The @write variable must not be modified by the caller between the
 * queue_enqueue_init() and queue_enqueue_done() calls (it is used
 * internally by the enqueue functions)
 * At the beginning of the sequence the caller should verify how many entries are
 * available to avoid a queue overflow.
 *
 */
static inline void queue_enqueue_init(struct queue *q, u32 *write)
{
	*write = atomic_read(&q->write);
}

static inline void queue_enqueue_next(struct queue *q, u32 *write, unsigned long entry)
{
	q->entry[*write] = entry;

	__queue_incr(q, write);
}

static inline void queue_enqueue_done(struct queue *q, u32 write)
{
	smp_wmb();
	atomic_set(&q->write, write);
}

/**
 * queue_dequeue_init() - optimized dequeueing init
 * @q - pointer to queue structure
 * @read - caller provided storage to track optimized dequeueing operation
 *
 * queue_dequeue_init()/queue_dequeue_next()/queue_dequeue_done() provide
 * an optimized interface to dequeue several entries in a single go.
 * The functions must be called in that order. queue_dequeue_init()/queue_dequeue_done()
 * must be called once, queue_dequeue_next() may be called several times.
 * The @read variable must not be modified by the caller between the
 * queue_dequeue_init() and queue_dequeue_done() calls (it is used
 * internally by the dequeue functions)
 * At the beginning of the sequence the caller should verify how many entries are
 * pending to avoid a queue underflow.
 *
 */
static inline void queue_dequeue_init(struct queue *q, u32 *read)
{
	*read = atomic_read(&q->read);
}

static inline unsigned long queue_dequeue_next(struct queue *q, u32 *read)
{
	unsigned long entry = q->entry[*read];

	__queue_incr(q, read);

	return entry;
}

static inline void queue_dequeue_done(struct queue *q, u32 read)
{
	smp_wmb();
	atomic_set(&q->read, read);
}

static inline unsigned long queue_peek(struct queue *q)
{
	if (queue_empty(q))
		return -1; /* 0 is a valid entry value */
	else
		return q->entry[atomic_read(&q->read)];
}

static inline int queue_enqueue(struct queue *q, unsigned long entry)
{
	u32 write;

	if (queue_full(q))
		return -1;

	queue_enqueue_init(q, &write);

	queue_enqueue_next(q, &write, entry);

	queue_enqueue_done(q, write);

	return 0;
}

static inline unsigned long queue_dequeue(struct queue *q)
{
	u32 read;
	unsigned long entry;

	if (queue_empty(q))
		return -1; /* 0 is a valid entry value */

	queue_dequeue_init(q, &read);

	entry = queue_dequeue_next(q, &read);

	queue_dequeue_done(q, read);

	return entry;
}

#endif /* _QUEUE_COMMON_H_ */

