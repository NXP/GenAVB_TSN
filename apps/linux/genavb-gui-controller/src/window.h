/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QListView>
#include <QStringListModel>
#include <QSlider>
#include <QModelIndex>
#include <QLabel>
#include "ui_mainform.h"

enum	ControlState { INIT, CONNECTED, PLAY, PAUSE, STOP };

#define MXCFB_SET_GBL_ALPHA _IOW('F', 0x21, struct mxcfb_gbl_alpha)
struct mxcfb_gbl_alpha {
        int enable;
        int alpha;
};
#define MXCFB_SET_CLR_KEY _IOW('F', 0x22, struct mxcfb_color_key)
struct mxcfb_color_key {
        int enable;
        long color_key;
};

class Window : public QWidget, private Ui::MainForm
{
	Q_OBJECT
public:
	explicit Window(QWidget *parent = 0, QString *composition = 0, bool video = 0, int fade = 10);
	~Window();
#ifndef WL_BUILD
	bool eventFilter(QObject * watched, QEvent * event);
#endif
	bool m_video;
	int m_play;
	int m_file_selected;


private:
	int m_GUIFadeInterval;
	QTimer *timerGUIFade;
	void updateState(ControlState);
#ifndef WL_BUILD
	int m_fd_fb;
	bool m_GUI_visible;
	void guiInvisible();
	void guiVisible();
	void setTransparency(long transparent_color);
#endif
signals:
	void playSignal(void);
	void pauseSignal(void);
	void stopSignal(void);
	void trackSignal(int);
	void previousSignal(void);
	void nextSignal(void);
	void volumeSignal(int);

public slots:
private slots:
	void startPauseClicked(void);
	void startPauseByAVDECC(void);
	void stopClicked(void);
	void stopByAVDECC(void);
	void mediaListUpdated(char **, int);
	void mediaFileActivated(int);
	void trackChangedByAVDECC(int);
	void volumeChangedByUser(int);
	void volumeChangedByAVDECC(int);
	void audioListenerDetectedHandler(int);
	void avbConnectHandler(int);
	void timerGUIFadeFired(void);
#ifdef WL_BUILD
	void handle_inputEvent(void);
	void handle_DoubleBtnPressed(void);
#endif
};

#endif // WINDOW_H
