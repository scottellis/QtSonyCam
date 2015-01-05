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
	bool setFeatureValue(ZCL_FEATUREID id, quint32 val);

	void initFeatureLists();
	void initZCLLists();
	void updateStatusBar();
	void layoutStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::QtSonyCamClass ui;

	QStringList m_ZCLMonoFormats;
	QStringList m_ZCLColorFormats;
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
	ZCL_EXTMODE m_zclExtMode;
	ZCL_COLORID m_zclColorID;

	int m_frameTimer;
	int m_frameCount;
	QQueue<cv::Mat *> m_frameQ;
	QMutex m_frameQMutex;

	int m_displayTimer;

	QHash<QString, int> m_featureIDs;
	QHash<QString, quint32> m_features;
	QHash<QString, quint32> m_featuresMin;
	QHash<QString, quint32> m_featuresMax;
	quint32 m_whiteBalance_U;
	quint32 m_whiteBalance_V;
	quint32 m_whiteBalanceMin;
	quint32 m_whiteBalanceMax;
	bool m_haveWhiteBalance;

	QSettings *m_settings;
	QLabel *m_cameraModelStatus;
	QLabel *m_cameraUIDStatus;
	QLabel *m_cameraFormatStatus;
	QLabel *m_cameraFPSStatus;
	QLabel *m_actualFPSStatus;
};

#endif // QTSONYCAM_H
