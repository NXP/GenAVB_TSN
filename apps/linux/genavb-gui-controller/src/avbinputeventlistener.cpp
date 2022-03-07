/*
 * Copyright NXP 2018.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QMessageBox>
#include <QDir>
#include <QDesktopWidget>
#include <QApplication>
#include <QLabel>

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <linux/input.h>

#include "avbinputeventlistener.h"


AvbInputEventListener::AvbInputEventListener(int file_fd)
{
	m_file_fd = file_fd;
	timerclear(&m_lastTimeBtnPress);
}


AvbInputEventListener::~AvbInputEventListener()
{
}


void AvbInputEventListener::doWork(void)
{

	struct input_event ev[64];
	int rc,i;
	struct timeval diff, max_diff_time;

	max_diff_time.tv_sec = 0;
	max_diff_time.tv_usec = 300000;

	rc = ::read(m_file_fd, &ev, sizeof(ev));
	if (rc < (int) sizeof(struct input_event)) {
		qWarning("Error (%s) reading event device descriptor: expected %d bytes, got %d.", std::strerror(errno), (int) sizeof(struct input_event), rc);
	} else {
		for (i = 0; i < rc / (int) sizeof(struct input_event); i++) {
			unsigned int type, code;

			type = ev[i].type;
			code = ev[i].code;

			if (type == EV_KEY && ( code == BTN_TOUCH || code == BTN_LEFT ) && ev[i].value == 1) {
				timersub(&ev[i].time, &m_lastTimeBtnPress, &diff);
				m_lastTimeBtnPress = ev[i].time;
				if (timercmp(&diff, &max_diff_time, < )) {
					/* This is a double click/tap */
					emit avbDoubleBtnPressed();
					return;
				}
			}

		}
		emit avbInputEventReceived();
	}
}

void AvbInputEventListener::deleteLater(void)
{
}

