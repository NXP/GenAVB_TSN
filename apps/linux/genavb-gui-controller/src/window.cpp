/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QMessageBox>
#include <QDir>
#include <QScreen>
#include <QApplication>
#include <QLabel>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "window.h"
#include "math.h"
#include "avbcontroller.h"

Window::Window(QWidget *parent, QString *composition, bool video, int fade) :
    QWidget(parent)
{
	//Get the screen resolution
	QRect rectangle = QGuiApplication::primaryScreen()->geometry();
	//Get the current palette
	QPalette currentPalette = QApplication::palette();
	//Set the background color to green
	currentPalette.setColor(QPalette::Active, QPalette::Window, QColor(0, 255, 0));
	currentPalette.setColor(QPalette::Inactive, QPalette::Window, QColor(0, 255, 0));
	//Set back the palette to the application
	QApplication::setPalette(currentPalette);

	setStyleSheet("background:transparent;");
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground, true);


	setupUi(this);

	setGeometry(rectangle);

	updateState(INIT);

	// video (1) or audio (0) mode
	m_video = video;

	//Allocate and intialize the timers
	timerGUIFade = new QTimer(this);
#ifndef WL_BUILD
	//Open the targeted frame buffer - fb identification should be provided by the app caller
	QByteArray ba = composition->toLatin1();
	m_fd_fb = ::open(ba.data(), O_RDWR, 0);
	if (m_fd_fb < 0)
	{
		qDebug() << "Could not open the specified frame buffer to set composition parameters.";
		qDebug() << "  You specified as frame buffer: " << *composition;
	}
#else
	Q_UNUSED(composition);
#endif

	//Connect signals and slots
	connect(media_combo, SIGNAL(activated(int)), this, SLOT(mediaFileActivated(int)));
	connect(m_btnStartPause,SIGNAL(clicked()), this, SLOT(startPauseClicked()));
	connect(m_btnStop,SIGNAL(clicked()), this, SLOT(stopClicked()));

	connect(m_volumeControl, SIGNAL(valueChanged(int)), this, SLOT(volumeChangedByUser(int)));

	connect(timerGUIFade, SIGNAL(timeout()), this, SLOT(timerGUIFadeFired()));

#ifndef WL_BUILD
	setTransparency(0x0000ff00);//Set transparency to green
	guiVisible();//Just in case the frame buffer was left in full transparent mode
	m_GUI_visible = true;
#endif
	m_GUIFadeInterval = fade*1000;
	timerGUIFade->setSingleShot(true);
	timerGUIFade->start(m_GUIFadeInterval);
}

void Window::timerGUIFadeFired(void)
{
	qDebug() <<"timerGUIFadeFired";
#ifndef WL_BUILD
	guiInvisible();//Just in case the frame buffer was left in full transparent mode
	m_GUI_visible = false;
#else
	qDebug() << "Set window visible false";
	setVisible(false);
#endif
}

#ifndef WL_BUILD
void Window::guiInvisible()
{
	unsigned char current_alpha = 255;
	char increment = -1;

	struct mxcfb_gbl_alpha alpha;
	if(m_fd_fb > 0)
	{
		while(current_alpha > 0)
		{
	        alpha.alpha = current_alpha;
	    	alpha.enable = 1;
	    	if ( ioctl(m_fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0)
	        {
				//Should signal some error here
				break;
			}
			current_alpha +=increment;
			QThread::msleep(1);
		}
	}
}

void Window::guiVisible()
{
	qDebug() <<"guiVisible";
	struct mxcfb_gbl_alpha alpha;
	if(m_fd_fb > 0)
	{
        alpha.alpha = 255;
    	alpha.enable = 1;
    	if ( ioctl(m_fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0)
        {
			//Should signal some error here
		}
	}
}

void Window::setTransparency(long transparent_color)
{
    struct mxcfb_color_key color_key;
	if(m_fd_fb > 0)
	{
	    color_key.color_key = transparent_color;
	    color_key.enable = 1;
		if ( ioctl(m_fd_fb, MXCFB_SET_CLR_KEY, &color_key) < 0)
	    {
			//Should signal some error here
		}
	}
}

bool Window::eventFilter(QObject * watched, QEvent * event)
{
	if (event->type() == QEvent::MouseButtonPress ) {
		timerGUIFade->start(m_GUIFadeInterval);//Reset the interval for the GUI fade
		guiVisible();
		if (!m_GUI_visible)
		{
			m_GUI_visible = true;

			return true;
		}
	}
	return QObject::eventFilter(watched, event);
}
#else
void Window::handle_inputEvent(void)
{
	int remain = timerGUIFade->remainingTime();
	/* Avoid resetiing the timer on too close successive input events*/
	if (remain < (m_GUIFadeInterval - 1000)) {
		timerGUIFade->start(m_GUIFadeInterval);//Reset the interval for the GUI fade
		setVisible(true);
	}
}

void Window::handle_DoubleBtnPressed(void)
{
	setVisible(false);
	setVisible(true);
	timerGUIFade->start(m_GUIFadeInterval);//Reset the interval for the GUI fade
}
#endif

void Window::startPauseClicked(void)
{
	if (!m_play)
	{
		updateState(PLAY);
		emit playSignal();
	} else
	{
		updateState(PAUSE);
		emit pauseSignal();
	}
}

void Window::startPauseByAVDECC(void)
{
	qDebug() << "Window::startPauseByAVDECC";
	if (!m_play)
		updateState(PLAY);
	else
		updateState(PAUSE);
}

void Window::mediaListUpdated(char **tracks, int count)
{
	int i;

	while (media_combo->count())
		media_combo->removeItem(0);

	for (i = 0; i < count; i++)
		media_combo->insertItem(i, QString(tracks[i]), 0);

}

void Window::mediaFileActivated(int index)
{
	qDebug() << "Window::mediaFileActivated index = "<< index;
	emit trackSignal(index);
}

void Window::trackChangedByAVDECC(int index)
{
	media_combo->setCurrentIndex(index);
}

void Window::stopClicked(void)
{
	emit stopSignal();
	updateState(STOP);
}

void Window::stopByAVDECC(void)
{
	qDebug() << "Window::stopByAVDECC";
	updateState(STOP);
}

void Window::volumeChangedByUser(int value)
{
	qDebug() << "Window::volumeChangedByUser =" << value;
	emit volumeSignal(value);
}

void Window::volumeChangedByAVDECC(int value)
{
	if (!m_volumeControl->isSliderDown()) {
		bool blocked = m_volumeControl->blockSignals(true);
		m_volumeControl->setSliderPosition(value);
		m_volumeControl->blockSignals(blocked);
		qDebug() << "Window::volumeChangedByAVDECC =" << value;
	}
}

void Window::audioListenerDetectedHandler(int detected)
{
	qDebug() << "Window::audioListenerDetectedHandler =" << detected;
	if (detected)
		m_volumeControl->setEnabled(true);
	else
		m_volumeControl->setEnabled(false);
}


void Window::avbConnectHandler(int connected)
{
	if (connected)
		updateState(PLAY);
	else
		updateState(INIT);
}

void Window::updateState(ControlState state)
{
	switch (state)
	{
		case INIT:
			m_play = 0;
			m_file_selected = 0;
			m_btnStartPause->setEnabled(false);
			m_btnStop->setEnabled(false);
			m_volumeControl->setEnabled(false);
			break;

		case CONNECTED:
			m_play = 0;
			m_btnStartPause->setEnabled(true);
			m_btnStop->setEnabled(false);
			break;

		case PLAY:
			m_play = 1;
			m_btnStop->setEnabled(true);
			if (!m_video)/* audio case */
				m_btnStartPause->setText("PAUSE");
			else /* video case, no PAUSE */
				m_btnStartPause->setEnabled(false);
			break;

		case PAUSE:
			m_play = 0;
			m_btnStop->setEnabled(true);
			break;

		case STOP:
			m_play = 0;
			m_btnStartPause->setEnabled(true);
			m_btnStop->setEnabled(false);
			break;

		default:
			break;
	}
}

Window::~Window()
{
#ifndef WL_BUILD
	::close(m_fd_fb);
#endif
	delete timerGUIFade;
}

