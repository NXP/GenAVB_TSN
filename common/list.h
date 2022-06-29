/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
*
* NXP Confidential. This software is owned or controlled by NXP and may only
* be used strictly in accordance with the applicable license terms.  By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*/

/**
 @file
 @brief Double linked list implementation
 @details Double linked list data types and helper functions.
*/

#ifndef _LIST_H_
#define _LIST_H_

#include "common/types.h"

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

static inline void list_head_init(struct list_head *head)
{
	head->prev = head;
	head->next = head;
}

static inline void list_add(struct list_head *head, struct list_head *entry)
{
	entry->next = head->next;
	entry->next->prev = entry;

	head->next = entry;
	entry->prev = head;
}

static inline void list_add_tail(struct list_head *head, struct list_head *entry)
{
	entry->prev = head->prev;
	entry->prev->next = entry;

	head->prev = entry;
	entry->next = head;
}

static inline void list_del(struct list_head *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	entry->prev = NULL;
	entry->next = NULL;
}

#define list_empty(head)	((head)->next == (head))

#define list_first(head)	((head)->next)
#define list_last(head)		((head)->prev)
#define list_next(entry)	((entry)->next)

#endif /* _LIST_H_ */
