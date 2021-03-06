/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#ifndef ZCLCAMERA_H
#define ZCLCAMERA_H

#include <qthread.h>

#include <Windows.h>
#include <ZCLAPI.h>
#include <opencv2/core/core.hpp>

class CameraThread : public QThread 
{
	Q_OBJECT

public:
	CameraThread(QObject *parent, HCAMERA hCamera, bool color, bool externalTrigger);

	bool stop(unsigned long max_wait);

signals:
	void newFrame(cv::Mat *frame);

protected:
	void run();

private:
	void handleFrame(quint8 *data, int rows, int cols);

	bool m_stop;
	bool m_color;
	bool m_externalTrigger;
	HCAMERA m_hCamera;
};

#endif // ZCLCAMERA_H
