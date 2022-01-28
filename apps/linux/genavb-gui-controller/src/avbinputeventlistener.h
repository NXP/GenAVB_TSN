/*
 * Copyright NXP 2018
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AVB_INPUT_EVENT_LISTENER_H
#define AVB_INPUT_EVENT_LISTENER_H

#include <QtCore>
#include <QSocketNotifier>
#include <QString>
#include <sys/time.h>

class AvbInputEventListener : public QObject
{
	Q_OBJECT

public:
	AvbInputEventListener(int m_file_fd);
	~AvbInputEventListener();

private:
	int m_file_fd;
	struct timeval m_lastTimeBtnPress;

signals:
	void avbInputEventReceived();
	void avbDoubleBtnPressed();

public slots:
	void doWork();
	void deleteLater();

};


#endif // AVB_INPUT_EVENT_LISTENER_H
