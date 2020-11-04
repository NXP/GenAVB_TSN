/*
 * AVB queue
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2019 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

void queue_flush(struct queue *q, void *data)
{
	unsigned long entry;

	/* Frees all the buffers in the queue */
	/* It's only safe to dequeue from a rx queue because the user is closing it (i.e, no longer dequeing) */
	while (!queue_empty(q)) {
		entry = queue_dequeue(q);

		q->entry_free(data, entry);
	}
}
