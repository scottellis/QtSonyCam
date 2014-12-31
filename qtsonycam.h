/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#ifndef QTSONYCAM_H
#define QTSONYCAM_H

#include <QtWidgets/QMainWindow>
#include <qsettings.h>
#include <qqueue.h>
#include <qmutex.h>
#include <qhash.h>

#include <opencv2/highgui/highgui.hpp>

#include "ui_qtsonycam.h"
#include "camerathread.h"

class QtSonyCam : public QMainWindow
{
	Q_OBJECT

public:
	QtSonyCam(QWidget *parent = 0);
	~QtSonyCam();

public slots:
	void onFeatures();
	void onImageInfo();
	void onCamera();
	void onStart();
	void onStop();

	void newFrame(cv::Mat *frame);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	bool findCamera();
	bool findZCLStdModeAndFPS();
	void showFrame(cv::Mat *frame);
	bool setFeatureValue(QString feature);

	void initFeatureLists();
	void initZCLLists();
	void updateStatusBar();
	void layoutStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::QtSonyCamClass ui;

	QStringList m_ZCLFormats;
	QStringList m_ZCLFrameRates;
	QStringList m_ZCLColorIDs;

	bool m_running;
	bool m_color;
	CameraThread *m_cameraThread;
	HCAMERA m_hCamera;
	QString m_cameraModel;
	quint64 m_cameraUID;
	ZCL_STDMODE m_zclStdMode;
	ZCL_FPS m_zclFps;

	int m_frameTimer;
	int m_frameCount;
	QQueue<cv::Mat *> m_frameQ;
	QMutex m_frameQMutex;

	QHash<QString, int> m_features;
	QHash<QString, int> m_featuresMin;
	QHash<QString, int> m_featuresMax;

	int m_displayTimer;

	QSettings *m_settings;
	QLabel *m_cameraModelStatus;
	QLabel *m_cameraUIDStatus;
	QLabel *m_cameraFormatStatus;
	QLabel *m_cameraFPSStatus;
	QLabel *m_actualFPSStatus;
};

#endif // QTSONYCAM_H
