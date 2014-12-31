/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include <qdebug.h>

#include <opencv2/imgproc/imgproc.hpp>
#include "camerathread.h"

CameraThread::CameraThread(QObject *parent, HCAMERA hCamera, bool color) 
	: QThread(parent)
{
	m_hCamera = hCamera;
	m_color = color;
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
			qDebug() << "CameraThread::run(): ZCLImageCompleteWait() timed out";
			m_stop = true;
			break;
		}

		if (m_color)
			handleColorFrame(data, info.Image.Height, info.Image.Width);
		else
			handleMonoFrame(data, info.Image.Height, info.Image.Width);
	}

	if (!ZCLIsoStop(m_hCamera))
		qDebug() << "CameraThread::run(): ZCLIsoStop() failed";
	
done:
	if (!ZCLIsoRelease(m_hCamera))
		qDebug() << "CameraThread::run(): ZCLIsoRelease() failed";

	if (data)
		delete [] data;
}

void CameraThread::handleMonoFrame(quint8 *data, int rows, int cols)
{
	cv::Mat temp(rows, cols, CV_8UC1, data);

	cv::Mat *m = new cv::Mat(rows, cols, CV_8UC3);

	cvtColor(temp, *m, CV_GRAY2RGB, 3); 

	emit newFrame(m);
}

void CameraThread::handleColorFrame(quint8 *data, int rows, int cols)
{
	cv::Mat temp(rows, cols, CV_8UC1, data);

	cv::Mat *m = new cv::Mat(rows, cols, CV_8UC3);

	//cvtColor(temp, *m, CV_BayerBG2RGB, 3);
	//cvtColor(temp, *m, CV_BayerGB2RGB, 3);
	//cvtColor(temp, *m, CV_BayerRG2RGB, 3);
	cvtColor(temp, *m, CV_BayerGR2RGB, 3);

	emit newFrame(m);
}