
/*
* Copyright 2021 NXP
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
 @brief Linux Standard network rtnetlink helpers
 @details
*/
#ifndef _LINUX_RTNETLINK_H_
#define _LINUX_RTNETLINK_H_

#include <sys/uio.h>
#include <linux/netlink.h>

#define NLMSG_NEXT_DATA(nmsg) \
        ((void *) (((char *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define rtnetlink_nlmsghdr_init(nh, len, type, flags) \
	(nh)->nlmsg_len = len; \
	(nh)->nlmsg_type = type; \
	(nh)->nlmsg_flags = flags; \
	(nh)->nlmsg_seq = 0; \
	(nh)->nlmsg_pid = 0;

int rtnetlink_socket_init(void);
void rtnetlink_socket_exit(void);
int rtnetlink_socket_send_iov(struct iovec *iov, unsigned int iovlen);
int rtnetlink_attr_add(struct nlmsghdr *nh, unsigned int req_buf_size, int type, const void *data, unsigned int data_len);

#endif /* _LINUX_RTNETLINK_H_ */
