/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AVB_AECP_H
#define AVB_AECP_H

#include <QtCore>
#include <QString>


#include "avb.h"


class AvbController : public QObject
{
	Q_OBJECT

public:
	AvbController(avb_u64 tid, avb_u64 aid, avb_u64 vid);
	~AvbController();

private:
	struct app_avb_cfg m_app_controller_cfg;
	QSocketNotifier *m_ctrl_avdecc_controller_notifier;

signals:
	void avbConnectedSignal(int);
	void volumeRemoteSignal(int);
	void audioListenerDetectedSignal(int);
	void trackRemoteSignal(int);
	void playRemoteSignal();
	void stopRemoteSignal();
	void finished();
	void mediaListSignal(char **, int);

public slots:

	//handlers for GUI events
	void volumeHandler(int);
	void playHandler(void);
	void pauseHandler(void);
	void stopHandler(void);
	void trackHandler(int);
	void nextHandler(void);
	void previousHandler(void);
	void doWork();
	void deleteLater();

private slots:
	//handlers for AVB stack events
	void ctrlAvdeccController(int socket);
};


#endif // AVB_AECP_H
