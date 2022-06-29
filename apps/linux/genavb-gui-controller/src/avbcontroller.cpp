/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QLabel>

#include <iostream>
#include <unistd.h>
#include <limits.h>

#include <genavb/aecp.h>

#include "avbcontroller.h"

#define AVB_DBG

#define CMD_STOP 0
#define CMD_PLAY 255

#define CMD_PREV 0
#define CMD_NEXT 1


AvbController::AvbController(avb_u64 tid, avb_u64 aid, avb_u64 vid)
{
	QString m_current_media_file_name = QString();


	m_app_controller_cfg.mode = 0;
	m_app_controller_cfg.config = 0;
	m_app_controller_cfg.talker_association_id = tid;
	m_app_controller_cfg.audio_listener_association_id = aid;
	m_app_controller_cfg.video_listener_association_id = vid;
	m_app_controller_cfg.connected = 0; /* =1 if avdecc connect received, =0 if not connect or avdecc disconnect received */
	m_app_controller_cfg.listener_discovered = 0;
	m_app_controller_cfg.talker_discovered = 0;
	m_app_controller_cfg.listener_entity_id = 0;
	m_app_controller_cfg.talker_entity_id = 0;
	m_app_controller_cfg.media_track = 1;
	m_app_controller_cfg.media_tracks = NULL;
	m_app_controller_cfg.media_track_count = 0;
	m_app_controller_cfg.volume = 50;
	m_app_controller_cfg.byte_count= 0;

	m_app_controller_cfg.avb_h = NULL;

	m_app_controller_cfg.ctrl_avdecc_control_fd = -1;
	m_app_controller_cfg.ctrl_avdecc_control_h = NULL;

	m_ctrl_avdecc_controller_notifier = NULL;
}


AvbController::~AvbController()
{

	if (m_app_controller_cfg.ctrl_avdecc_control_h != NULL) {
		avb_control_close(m_app_controller_cfg.ctrl_avdecc_control_h);
		m_app_controller_cfg.ctrl_avdecc_control_fd = -1;
	}

	emit finished();

	delete m_ctrl_avdecc_controller_notifier;
}


void AvbController::doWork(void)
{
	int rc;

	//Genavb stack initialization
	rc = app_avb_setup(&m_app_controller_cfg);
	if (rc < 0) {
		qDebug() << "avb_setup() failed";
	}
	else
	{
		//Control events notifier (AVDECC controls: volume/play/stop)
		m_ctrl_avdecc_controller_notifier = new QSocketNotifier(m_app_controller_cfg.ctrl_avdecc_control_fd, QSocketNotifier::Read, this);
		connect(m_ctrl_avdecc_controller_notifier, SIGNAL(activated(int)), this, SLOT(ctrlAvdeccController(int)), Qt::AutoConnection);
		m_ctrl_avdecc_controller_notifier->setEnabled(true);

		//Assume that here we are connected and we can start controlling the listener/talker
		emit avbConnectedSignal(0);
	}
}

void AvbController::deleteLater(void)
{

}

void AvbController::volumeHandler(int volume_percent)
{
	m_app_controller_cfg.volume = volume_percent;
	qDebug() << "AvbController::volumeHandler =" << volume_percent;
	app_avb_send_aecp_volume_control(&m_app_controller_cfg, volume_percent);
}

void AvbController::playHandler(void)
{
	qDebug() << "AvbController::playHandler";
	app_avb_send_aecp_playstop_control(&m_app_controller_cfg, CMD_PLAY);
}

void AvbController::pauseHandler(void)
{
	qDebug() << "AvbController::pauseHandler";
}

void AvbController::stopHandler(void)
{
	qDebug() << "AvbController::stopHandler";
	app_avb_send_aecp_playstop_control(&m_app_controller_cfg, CMD_STOP);
}

void AvbController::nextHandler(void)
{
	qDebug() << "AvbController::nextHandler";
	app_avb_send_avdecc_prevnext_control(&m_app_controller_cfg, CMD_NEXT);
}

void AvbController::previousHandler(void)
{
	qDebug() << "AvbController::previousHandler";
	app_avb_send_avdecc_prevnext_control(&m_app_controller_cfg, CMD_PREV);
}

void AvbController::trackHandler(int track)
{
	qDebug() << "AvbController::trackHandler track = "<< track;
	app_avb_send_avdecc_media_track_control(&m_app_controller_cfg, track + 1);
}

void AvbController::ctrlAvdeccController(int socket)
{
	Q_UNUSED(socket);
	avb_msg_type_t event_type;
	avb_u16 ctrl_index = 0;
	avb_u8 ctrl_value[96];  //FIXME
	int ret;
	int previous_talker_discovered = m_app_controller_cfg.talker_discovered;
	int previous_listener_discovered = m_app_controller_cfg.listener_discovered;

	m_ctrl_avdecc_controller_notifier->setEnabled(false);

	/*
	 * read control event from avdecc
	 */
	ret = app_avb_handle_avdecc_controller(&m_app_controller_cfg, &event_type, &ctrl_index, &ctrl_value);

	if (ret == AECP_AEM_SUCCESS) {
		switch(ctrl_index) {
		case 0: /* volume */
			m_app_controller_cfg.volume = ctrl_value[0];
			emit volumeRemoteSignal(m_app_controller_cfg.volume);
			qDebug() <<" emit volumeRemoteSignal(m_app_controller_cfg.volume); vol=" << m_app_controller_cfg.volume;

			break;

		case 1: /* play/stop */
			if (ctrl_value[0]) { /* PLAY <--> 255 */
				emit playRemoteSignal();
				qDebug() <<"emit playRemoteSignal();";
			} else { /* STOP <--> 0 */
				emit stopRemoteSignal();
				qDebug() <<"emit stopRemoteSignal();";
			}
			break;

		case 2: /* media track ID */
			qDebug() <<"Received media track event from AVDECC: track=" << ctrl_value[0];
			m_app_controller_cfg.media_track = ctrl_value[0];
			emit trackRemoteSignal(m_app_controller_cfg.media_track - 1);
			break;

		case 3:
			qDebug() <<"Received media track name event from AVDECC: " << (const char *)ctrl_value;
			break;

		case 10: /* Media file list updated */
			emit mediaListSignal(m_app_controller_cfg.media_tracks, m_app_controller_cfg.media_track_count);
			qDebug() <<"emit mediaListSignal";
			break;

		default:
			break;
		}
	}

	if ((m_app_controller_cfg.listener_discovered != previous_listener_discovered))
		emit audioListenerDetectedSignal(m_app_controller_cfg.listener_discovered);

	if ((m_app_controller_cfg.talker_discovered != previous_talker_discovered) && (m_app_controller_cfg.talker_discovered == 0))
		emit avbConnectedSignal(0); // Disable the GUI as soon as the listener departs
	if ((m_app_controller_cfg.talker_discovered != previous_talker_discovered) && (m_app_controller_cfg.talker_discovered == 1))
		emit avbConnectedSignal(1); // Enable the GUI as soon as the listener arrives?


	m_ctrl_avdecc_controller_notifier->setEnabled(true);
}
