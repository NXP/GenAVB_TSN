/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdlib>
#include <cstdio>

#include <QProxyStyle>
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "window.h"
#include "avbcontroller.h"
#include "avbinputeventlistener.h"

// Override style for better Touchscreen handling (larger slider cursor)
class CstProxyStyle : public QProxyStyle
{
	public:
		int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const
		{
			switch(metric) {
				case PM_SliderLength	:	return 50;
				default			:	return (QProxyStyle::pixelMetric(metric,option,widget));
				}
		}
};

//////////////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	/* Setting stdout to line buffered, otherwise printf in C sections are not coming out */
	setlinebuf(stdout);

	QApplication app(argc, argv);
	QApplication::setStyle(new CstProxyStyle);
	QApplication::setApplicationName("genavb-gui-controller");

	QProcessEnvironment environment = QProcessEnvironment();
	QCommandLineParser parser;
	parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
	QCommandLineOption videoOption(QStringList() << "v" << "video", QCoreApplication::translate("main", "Audio or Video talker."));
	QCommandLineOption fadeOption(QStringList() << "f" << "fade", "Time (seconds) for the GUI to be visible after the last touch event. Default is 10, minimum is 3.", "10");
#ifndef WL_BUILD
	QCommandLineOption compositionOption(QStringList() << "c" << "composition", "[FB Backend] The frame buffer that is to be composited on top of the one that displays the video. Example: /dev/fb1.", "/dev/fb1");
#else
	QCommandLineOption inputEventDeviceOption(QStringList() << "e" << "input event device", "[Wayland Backend] The event input device to monitor (for activity) used for GUI visibility. Example: /dev/input/event1.", "/dev/input/event1");
#endif
	QCommandLineOption talkerIdOption(QStringList() << "T" << "tid", "Association ID of the talker entity to wait for", "0");
	QCommandLineOption audioIdOption(QStringList() << "A" << "aid", "Association ID of the audio listener entity to wait for", "0");
	QCommandLineOption videoIdOption(QStringList() << "V" << "vid", "Association ID of the video listener entity to wait for", "0");
	parser.addOption(videoOption);
	parser.addOption(fadeOption);
#ifndef WL_BUILD
	parser.addOption(compositionOption);
#else
	parser.addOption(inputEventDeviceOption);
#endif
	parser.addOption(talkerIdOption);
	parser.addOption(audioIdOption);
	parser.addOption(videoIdOption);
	parser.addHelpOption();
#ifndef WL_BUILD
	parser.setApplicationDescription("[FB Backend] GUI AVB controller application");
#else
	parser.setApplicationDescription("[WL Backend] GUI AVB controller application");
#endif

	//Process the command line
	parser.process(app);

	bool video = parser.isSet(videoOption);
	QString strFade =parser.value(fadeOption);
	int fade = strFade.toInt();
	QString str_tid =parser.value(talkerIdOption);
	int tid = str_tid.toULongLong();
	QString str_aid =parser.value(audioIdOption);
	int aid = str_aid.toULongLong();
	QString str_vid =parser.value(videoIdOption);
	avb_u64 vid = str_vid.toULongLong();
	qDebug() << "Talker association ID = " << tid;
	qDebug() << "Audio listener association ID = " << aid;


	qDebug() <<"Starting application in " << (video ? "video" : "audio") << " mode";
	qDebug() <<"Fade is " << strFade;
	if(fade <= 0 )
	{
		qDebug() << "Specified fade duration lower than 3. Setting fade to 3. ";
		fade = 3;
	}

#ifndef WL_BUILD
	QString strComposition = parser.value(compositionOption);
	qDebug() <<" [FB Backend] Composited frame buffer is " << strComposition;
#else
	QString strInputEventDevice = parser.value(inputEventDeviceOption);
	QString strComposition; /*Default initialize the composition variable*/
	qDebug() <<" [WL Backend] Event Input device is " << strInputEventDevice;
#endif

	//Create AVB processing thread
	AvbController *avdecc_controller = new AvbController(tid, aid, vid);
	QThread *thread = new QThread();

	QObject::connect(thread, SIGNAL(started()), avdecc_controller, SLOT(doWork()));
	QObject::connect(thread, SIGNAL(finished()), avdecc_controller, SLOT(deleteLater()));
	QObject::connect(avdecc_controller, SIGNAL(finished()), thread, SLOT(quit()));
	avdecc_controller->moveToThread(thread);

#ifdef WL_BUILD
	QByteArray ba = strInputEventDevice.toLatin1();
	int input_event_fd = ::open(ba.data(), O_RDONLY , 0);
	if (input_event_fd < 0) {
		qDebug("Error Opening Input device %s", ba.data());
		return -1;
	}

	AvbInputEventListener *avb_event_listener = new AvbInputEventListener(input_event_fd);
	QThread *event_thread = new QThread();
	avb_event_listener->moveToThread(event_thread);

	QSocketNotifier * event_notifier = new QSocketNotifier(input_event_fd, QSocketNotifier::Read, &app);
	QObject::connect(event_notifier, SIGNAL(activated(int)), avb_event_listener, SLOT(doWork()));
	event_notifier->setEnabled(true);
#endif

	//Construct and show the main window
	Window window(0, &strComposition, video, fade);
	window.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

	QObject::connect(avdecc_controller, SIGNAL(volumeRemoteSignal(int)), &window, SLOT(volumeChangedByAVDECC(int)));
	QObject::connect(avdecc_controller, SIGNAL(stopRemoteSignal()), &window, SLOT(stopByAVDECC()));
	QObject::connect(avdecc_controller, SIGNAL(playRemoteSignal()), &window, SLOT(startPauseByAVDECC()));
	QObject::connect(avdecc_controller, SIGNAL(mediaListSignal(char **, int)), &window, SLOT(mediaListUpdated(char **, int)));
	QObject::connect(avdecc_controller, SIGNAL(trackRemoteSignal(int)), &window, SLOT(trackChangedByAVDECC(int)));

	QObject::connect(&window,SIGNAL(playSignal()), avdecc_controller, SLOT(playHandler()));
	QObject::connect(&window,SIGNAL(pauseSignal()), avdecc_controller, SLOT(pauseHandler()));
	QObject::connect(&window,SIGNAL(stopSignal()), avdecc_controller, SLOT(stopHandler()));
	QObject::connect(&window,SIGNAL(trackSignal(int)), avdecc_controller, SLOT(trackHandler(int)));
	QObject::connect(&window,SIGNAL(volumeSignal(int)), avdecc_controller, SLOT(volumeHandler(int)));
#ifdef WL_BUILD
	QObject::connect(avb_event_listener, SIGNAL(avbInputEventReceived()), &window, SLOT(handle_inputEvent()), Qt::QueuedConnection);
	QObject::connect(avb_event_listener, SIGNAL(avbDoubleBtnPressed()), &window, SLOT(handle_DoubleBtnPressed()), Qt::QueuedConnection);
#endif
	QObject::connect(avdecc_controller, SIGNAL(avbConnectedSignal(int)), &window, SLOT(avbConnectHandler(int)));
	QObject::connect(avdecc_controller, SIGNAL(audioListenerDetectedSignal(int)), &window, SLOT(audioListenerDetectedHandler(int)));
	app.installEventFilter(&window);
	thread->start();
#ifdef WL_BUILD
	event_thread->start();
#endif
	window.show();

	const int exitValue = app.exec();

#ifdef WL_BUILD
	delete event_notifier;
	delete event_thread;
#endif
	delete thread;
#ifdef WL_BUILD
	::close(input_event_fd);
#endif
	delete avdecc_controller;

	return exitValue;
}





