/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include <qdebug.h>

#include "camerathread.h"

CameraThread::CameraThread(QObject *parent, HCAMERA hCamera, bool color, bool externalTrigger) 
	: QThread(parent)
{
	m_hCamera = hCamera;
	m_color = color;
	m_externalTrigger = externalTrigger;
	m_stop = false;
}

bool CameraThread::stop(unsigned long max_wait)
{
	m_stop = true;

	return wait(max_wait);
}

void CameraThread::run()
{
	ZCL_GETIMAGEINFO info;

	quint8 *data = NULL;

	if (!m_hCamera) {
		qDebug() << "CameraThread::run(): m_hCamera is NULL";
		return;
	}

	if (!ZCLGetImageInfo(m_hCamera, &info)) {
		qDebug() << "CameraThread::run(): ZCLGetImageInfo() failed";
		return;
	}

	if (!ZCLIsoAlloc(m_hCamera)) {
		qDebug() << "CameraThread::run(): ZCLIsoAlloc() failed";
		return;
	}

	data = new quint8[info.Image.Buffer];

	if (!data) {
		qDebug() << "CameraThread::run(): failed to allocate data buffer";
		goto done;
	}

	// 0 is free run mode
	if (!ZCLIsoStart(m_hCamera, 0)) {
		qDebug() << "CameraThread::run(): ZCLIsoStart() failed";
		goto done;
	}

	while (!m_stop) {
		if (!ZCLImageReq(m_hCamera, data, info.Image.Buffer)) {
			qDebug() << "CameraThread::run(): ZCLImageReq() failed";
			m_stop = true;
			break;
		}

		if (!ZCLImageCompleteWaitTimeOut(m_hCamera, data, NULL, NULL, NULL, 1000)) {
			if (!ZCLAbortImageReqAll(m_hCamera))
				qDebug() << "CameraThread::run(): ZCLAbortImageReqAll() failed";

			if (!m_externalTrigger) {
				qDebug() << "CameraThread::run(): ZCLImageCompleteWait() timed out";
				m_stop = true;
				break;
			}
		}
		else {
			handleFrame(data, info.Image.Height, info.Image.Width);
		}
	}

	if (!ZCLIsoStop(m_hCamera))
		qDebug() << "CameraThread::run(): ZCLIsoStop() failed";
	
done:

	if (!ZCLIsoRelease(m_hCamera))
		qDebug() << "CameraThread::run(): ZCLIsoRelease() failed";

	if (data)
		delete [] data;
}

void CameraThread::handleFrame(quint8 *data, int rows, int cols)
{
	cv::Mat *m = new cv::Mat(rows, cols, CV_8UC1, data);

	if (m)
		emit newFrame(m);
}
