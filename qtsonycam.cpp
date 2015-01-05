/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include <qdir.h>
#include <qmessagebox.h>
#include <opencv2/imgproc/imgproc.hpp>

#include "qtsonycam.h"
#include "featuredlg.h"

QtSonyCam::QtSonyCam(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_running = false;
	m_hCamera = NULL;
	m_cameraThread = NULL;
	m_cameraUID = 0;
	m_color = false;

	ZCLSetStructVersion(ZCL_LIBRARY_STRUCT_VERSION);

	m_settings = new QSettings(QDir::currentPath() + "/" + "qtsonycam.ini", QSettings::IniFormat);

	connect(ui.actionExit, SIGNAL(triggered()), SLOT(close()));
	connect(ui.actionImage_Info, SIGNAL(triggered()), SLOT(onImageInfo()));
	connect(ui.actionFeatures, SIGNAL(triggered()), SLOT(onFeatures()));
	connect(ui.actionStart, SIGNAL(triggered()), SLOT(onStart()));
	connect(ui.actionStop, SIGNAL(triggered()), SLOT(onStop()));
	
	initZCLLists();
	layoutStatusBar();
	restoreWindowState();

	ui.actionStop->setEnabled(false);

	if (!findCamera())
		ui.actionStart->setEnabled(false);

	updateStatusBar();
}

QtSonyCam::~QtSonyCam()
{
	if (m_settings)
		delete m_settings;
}

void QtSonyCam::closeEvent(QCloseEvent *)
{
	if (m_cameraThread) {
		m_cameraThread->stop(2000);
		delete m_cameraThread;
		m_cameraThread = NULL;
	}

	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	m_frameQMutex.lock();
	qDeleteAll(m_frameQ);
	m_frameQ.clear();
	m_frameQMutex.unlock();

	saveWindowState();
}

void QtSonyCam::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_frameTimer) {
		double fps = m_frameCount;
		fps /= 3.0;
		m_actualFPSStatus->setText(QString("Actual: %1 fps").arg(QString::number(fps, 'g', 5)));
		m_frameCount = 0;
	}
	else {
		cv::Mat *m = NULL;

		m_frameQMutex.lock();

		if (m_frameQ.count() > 0) {
			// only show the last image
			while (m_frameQ.count() > 1) {
				m = m_frameQ.dequeue();
				delete m;
			}

			m = m_frameQ.dequeue();
		}

		m_frameQMutex.unlock();

		if (m) {
			showFrame(m);
			delete m;
		}
	}	
}

void QtSonyCam::showFrame(cv::Mat *frame)
{
	cv::Mat *m = new cv::Mat(frame->rows, frame->cols, CV_8UC3);
	
	if (m_color)
		cvtColor(*frame, *m, CV_BayerGR2RGB, 3);
	else
		cvtColor(*frame, *m, CV_GRAY2RGB, 3); 

	QImage img((const quint8 *)m->data, m->cols, m->rows, (int)m->step, QImage::Format_RGB888);
	ui.cameraView->setPixmap(QPixmap::fromImage(img.scaled(ui.cameraView->size(), Qt::KeepAspectRatio)));

	delete m;
}

void QtSonyCam::newFrame(cv::Mat *frame)
{
	m_frameCount++;

	if (m_frameQMutex.tryLock()) {
		// in case we have a bug in the display, limit the queue
		if (m_frameQ.count() < 8)
			m_frameQ.enqueue(frame);
		else
			delete frame;

		m_frameQMutex.unlock();
	}
	else {
		delete frame;
	}
}

void QtSonyCam::onStart()
{
	m_cameraThread = new CameraThread(this, m_hCamera, m_color);

	if (m_cameraThread) {
		connect(m_cameraThread, SIGNAL(newFrame(cv::Mat *)), this, SLOT(newFrame(cv::Mat *)), Qt::DirectConnection);
		m_frameCount = 0;
		m_frameTimer = startTimer(3000);
		m_displayTimer = startTimer(25);
		m_cameraThread->start();
		m_running = true;
		ui.actionStart->setEnabled(false);
		ui.actionStop->setEnabled(true);	
	}
}

void QtSonyCam::onStop()
{
	if (m_cameraThread) {
		if (!m_cameraThread->stop(2000))
			QMessageBox::warning(this, "Error", "Camera thread stop failed");

		disconnect(m_cameraThread, SIGNAL(newFrame(cv::Mat *)), this, SLOT(newFrame(cv::Mat *)));

		killTimer(m_frameTimer);
		m_frameTimer = 0;

		killTimer(m_displayTimer);
		m_displayTimer = 0;

		delete m_cameraThread;
		m_cameraThread = NULL;

		m_actualFPSStatus->setText("Actual: 0 fps");
	}

	m_running = false;
	ui.actionStop->setEnabled(false);	

	if (m_hCamera)
		ui.actionStart->setEnabled(true);

	m_frameQMutex.lock();
	qDeleteAll(m_frameQ);
	m_frameQ.clear();
	m_frameQMutex.unlock();
}

void QtSonyCam::onImageInfo()
{
	QString s;
	QString color;
	ZCL_GETIMAGEINFO info;

	if (!ZCLGetImageInfo(m_hCamera, &info)) {
		QMessageBox::warning(this, "Error", "ZCLGetImageInfo failed");
		return;
	}

	if ((int)info.Image.ColorID < m_ZCLColorIDs.length())
		color = m_ZCLColorIDs.at((int)info.Image.ColorID);
	else
		color = "Unknown";

	s.sprintf("Width: %u\nHeight: %u\nBufferLen: %lu\nDataLen: %lu\nColorID: ",
		info.Image.Width,
		info.Image.Height,
		info.Image.Buffer,
		info.Image.DataLength);

	QMessageBox::information(this, "Image Info", s + color);
}

bool QtSonyCam::findCamera()
{
	int index;
	ZCL_LIST *cameraList;
	DWORD count = 0;

	if (m_running)
		return false;

	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	m_cameraUID = 0;

	if (!ZCLGetList((ZCL_LIST *)&count)) {
		QMessageBox::warning(this, "Error", "No cameras found");
		return false;
	}

	cameraList = (ZCL_LIST *)malloc(sizeof(ZCL_LIST) + (sizeof(ZCL_CAMERAINFO) * count));

	if (!cameraList) {
		QMessageBox::warning(this, "Error", "Failed to allocate camera list");
		return false;
	}

	cameraList->CameraCount = count;
	
	if (!ZCLGetList(cameraList)) {
		QMessageBox::warning(this, "Error", "ZCLGetList failed");
		return false;
	}

	if (count > 1) {
		// launch a dialog to choose a camera
		index = 0;
	}
	else {
		index = 0;
	}

	m_cameraUID = cameraList->Info[index].UID;
	m_cameraModel = (char *)cameraList->Info[index].ModelName;

	if (!ZCLOpen(m_cameraUID, &m_hCamera)) {
		QMessageBox::warning(this, "Error", "ZCLOpen failed");
		return false;
	}

	if (!findZCLStdModeAndFPS()) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
		m_cameraUID = 0;
		QMessageBox::warning(this, "Error", "Failed finding mode and fps");
		return false;
	}

	initFeatureLists();

	return true;
}

bool QtSonyCam::findZCLStdModeAndFPS()
{
	int i;
	ZCL_CAMERAMODE mode;

	m_cameraFormatStatus->setText("");
	m_cameraFPSStatus->setText("");

	if (!m_hCamera)
		return false;

	if (m_cameraModel == "XCD-SX90") {		
		m_zclStdMode = ZCL_SXGA_MONO;
		m_color = false;
		mode.StdMode_Flag = TRUE;
		mode.u.Std.Mode = m_zclStdMode;
	}
	else {
		m_zclExtMode = ZCL_Mode_3; // 0, 1, 2, 3, 
		m_color = true;
		mode.StdMode_Flag = FALSE;
		mode.u.Ext.Mode = m_zclExtMode;
		mode.u.Ext.ColorID = ZCL_RAW;
		mode.u.Ext.FilterID = ZCL_FGBRG;
	}

	if (m_color) {
		if (!ZCLCheckCameraMode(m_hCamera, &mode))
			return false;
	}
	else {
		for (i = (int)ZCL_Fps_30; i >= 0; i--) {
			mode.u.Std.FrameRate = (ZCL_FPS) i;

			if (ZCLCheckCameraMode(m_hCamera, &mode)) {
				m_zclFps = (ZCL_FPS) i;
				break;
			}
		}
	
		if (i < 0)
			return false;	

		mode.StdMode_Flag = TRUE;
		mode.u.Std.Mode = m_zclStdMode;
		mode.u.Std.FrameRate = m_zclFps;

		if (!ZCLSetCameraMode(m_hCamera, &mode)) {
			QMessageBox::warning(this, "Error", "ZCLSetCameraMode failed");
			return false;		
		}
	}

	return true;
}

void QtSonyCam::onFeatures()
{
	FeatureDlg dlg(this, m_hCamera, m_featureIDs, m_features, m_featuresMin, m_featuresMax,
                   m_haveWhiteBalance, m_whiteBalance_U, m_whiteBalance_V,
                   m_whiteBalanceMin, m_whiteBalanceMax);

	if (QDialog::Accepted == dlg.exec()) {
		QHash<QString, quint32> newFeatures = dlg.newFeatures();

		QHash<QString, quint32>::const_iterator i;

		for (i = newFeatures.constBegin(); i != newFeatures.constEnd(); ++i) {
			if (m_features.value(i.key()) != newFeatures.value(i.key()))
				m_features[i.key()] = newFeatures.value(i.key());					
		}

		m_whiteBalance_U = dlg.newWhiteBalanceU();
		m_whiteBalance_V = dlg.newWhiteBalanceV();
	}
}

bool QtSonyCam::setFeatureValue(ZCL_FEATUREID id, quint32 val)
{
	ZCL_SETFEATUREVALUE value;

	if (!m_hCamera)
		return false;

	value.FeatureID = id;
	value.ReqID = ZCL_VALUE;
	value.u.Std.Value = val;

	if (!ZCLSetFeatureValue(m_hCamera, &value))
		return false;

	return true;
}

void QtSonyCam::initFeatureLists()
{
	ZCL_CHECKFEATURE feature;
	ZCL_GETFEATUREVALUE value;
	quint32 min, max, current;

	if (!m_hCamera)
		return;

	m_featureIDs.clear();
	m_features.clear();
	m_featuresMin.clear();
	m_featuresMax.clear();

	m_featureIDs.insert("Shutter", (int) ZCL_SHUTTER);
	m_featureIDs.insert("Gain", (int) ZCL_GAIN);
	m_featureIDs.insert("Brightness", (int) ZCL_BRIGHTNESS);
	m_featureIDs.insert("Gamma", (int) ZCL_GAMMA);
	m_featureIDs.insert("Hue", (int) ZCL_HUE);
	m_featureIDs.insert("Saturation", (int) ZCL_SATURATION);
	m_featureIDs.insert("Sharpness", (int) ZCL_SHARPNESS);
	m_featureIDs.insert("AutoExposure", (int) ZCL_AE);

	QHash<QString, int>::const_iterator i;

	for (i = m_featureIDs.constBegin(); i != m_featureIDs.constEnd(); ++i) {
		feature.FeatureID = (ZCL_FEATUREID) i.value();

		if (ZCLCheckFeature(m_hCamera, &feature)) {
			if (feature.PresenceFlag) {
				min = feature.u.Std.Min_Value;
				max = feature.u.Std.Max_Value;

				value.FeatureID = feature.FeatureID;
				value.u.WhiteBalance.Abs = 0;

				if (ZCLGetFeatureValue(m_hCamera, &value)) {
					current = value.u.Std.Value;			

					m_features.insert(i.key(), current);
					m_featuresMin.insert(i.key(), min);
					m_featuresMax.insert(i.key(), max);
				}
			}
		}
	}

	m_haveWhiteBalance = false;

	if (m_color) {
		// white balance is different
		feature.FeatureID = ZCL_WHITEBALANCE;

		if (ZCLCheckFeature(m_hCamera, &feature)) {
			if (feature.PresenceFlag) {
				m_whiteBalanceMin = feature.u.Std.Min_Value;
				m_whiteBalanceMax = feature.u.Std.Max_Value;

				value.FeatureID = feature.FeatureID;

				if (ZCLGetFeatureValue(m_hCamera, &value)) {
					m_whiteBalance_U = value.u.WhiteBalance.UB_Value;
					m_whiteBalance_V = value.u.WhiteBalance.VR_Value;
					m_haveWhiteBalance = true;					
				}
			}
		}
	}

}

void QtSonyCam::updateStatusBar()
{
	QString s;

	if (m_cameraUID) {
		m_cameraModelStatus->setText(m_cameraModel);
		if (!m_color) {
			m_cameraFormatStatus->setText(m_ZCLMonoFormats.at((int)m_zclStdMode));
			m_cameraFPSStatus->setText(m_ZCLFrameRates.at((int)m_zclFps));
		}
	}
	else {
		m_cameraModelStatus->setText("");
		m_cameraFormatStatus->setText("");
		m_cameraFPSStatus->setText("");
	}

	s.sprintf("%08x%08x",
			(DWORD)((m_cameraUID & 0xffffffff00000000) >> 32),
			(DWORD)(m_cameraUID & 0x00000000ffffffff));

	m_cameraUIDStatus->setText(s);
}

void QtSonyCam::layoutStatusBar()
{
	m_cameraModelStatus = new QLabel();
	m_cameraModelStatus->setFrameStyle(QFrame::Panel);
	m_cameraModelStatus->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_cameraModelStatus);

	m_cameraUIDStatus = new QLabel();
	m_cameraUIDStatus->setFrameStyle(QFrame::Panel);
	m_cameraUIDStatus->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_cameraUIDStatus);

	m_cameraFormatStatus = new QLabel();
	m_cameraFormatStatus->setFrameStyle(QFrame::Panel);
	m_cameraFormatStatus->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_cameraFormatStatus);

	m_cameraFPSStatus = new QLabel();
	m_cameraFPSStatus->setFrameStyle(QFrame::Panel);
	m_cameraFPSStatus->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_cameraFPSStatus);
	
	m_actualFPSStatus = new QLabel("Actual: 0 fps");
	m_actualFPSStatus->setFrameStyle(QFrame::Panel);
	m_actualFPSStatus->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_actualFPSStatus);
}

void QtSonyCam::saveWindowState()
{
	if (!m_settings)
		return;

	m_settings->beginGroup("Window");
	m_settings->setValue("Geometry", saveGeometry());
	m_settings->setValue("State", saveState());
	m_settings->endGroup();
}

void QtSonyCam::restoreWindowState()
{
	if (!m_settings)
		return;

    m_settings->beginGroup("Window");
    restoreGeometry(m_settings->value("Geometry").toByteArray());
    restoreState(m_settings->value("State").toByteArray());
    m_settings->endGroup();
}

void QtSonyCam::initZCLLists()
{
	m_ZCLMonoFormats.append("160x120 YUV(4:4:4");
	m_ZCLMonoFormats.append("320x240 YUV(4:2:2)");
	m_ZCLMonoFormats.append("640x480 YUV(4:1:1)");
	m_ZCLMonoFormats.append("640x480 YUV(4:2:2)");
	m_ZCLMonoFormats.append("640x480 RGB");
	m_ZCLMonoFormats.append("640x480 MONO");
	m_ZCLMonoFormats.append("640x480 MONO16");
	m_ZCLMonoFormats.append("800x600 YUV(4:2:2)");
	m_ZCLMonoFormats.append("800x600 RGB");
	m_ZCLMonoFormats.append("800x600 MONO");
	m_ZCLMonoFormats.append("800x600 MONO16");
	m_ZCLMonoFormats.append("1024x768 YUV(4:2:2)");
	m_ZCLMonoFormats.append("1024x768 RGB");
	m_ZCLMonoFormats.append("1024x768 MONO");
	m_ZCLMonoFormats.append("1024x768 MONO16");
	m_ZCLMonoFormats.append("1280x960 YUV(4:2:2)");
	m_ZCLMonoFormats.append("1280x960 RGB");
	m_ZCLMonoFormats.append("1280x960 MONO");
	m_ZCLMonoFormats.append("1280x960 MONO16");
	m_ZCLMonoFormats.append("1600x1200 YUV(4:2:2)");
	m_ZCLMonoFormats.append("1600x1200 RGB");
	m_ZCLMonoFormats.append("1600x1200 MONO");
	m_ZCLMonoFormats.append("1600x1200 MONO16");

	m_ZCLColorFormats.append("Mode 0");
	m_ZCLColorFormats.append("Mode 1");
	m_ZCLColorFormats.append("Mode 2");
	m_ZCLColorFormats.append("Mode 3");
	m_ZCLColorFormats.append("Mode 4");
	m_ZCLColorFormats.append("Mode 5");
	m_ZCLColorFormats.append("Mode 6");
	m_ZCLColorFormats.append("Mode 7");
	m_ZCLColorFormats.append("Mode 8");
	m_ZCLColorFormats.append("Mode 9");
	m_ZCLColorFormats.append("Mode 10");
	m_ZCLColorFormats.append("Mode 11");
	m_ZCLColorFormats.append("Mode 12");
	m_ZCLColorFormats.append("Mode 13");
	m_ZCLColorFormats.append("Mode 14");
	m_ZCLColorFormats.append("Mode 15");
	m_ZCLColorFormats.append("Mode 16");
	m_ZCLColorFormats.append("Mode 17");
	m_ZCLColorFormats.append("Mode 18");
	m_ZCLColorFormats.append("Mode 19");
	m_ZCLColorFormats.append("Mode 20");
	m_ZCLColorFormats.append("Mode 21");
	m_ZCLColorFormats.append("Mode 22");
	m_ZCLColorFormats.append("Mode 23");
	m_ZCLColorFormats.append("Mode 24");
	m_ZCLColorFormats.append("Mode 25");
	m_ZCLColorFormats.append("Mode 26");
	m_ZCLColorFormats.append("Mode 27");
	m_ZCLColorFormats.append("Mode 28");
	m_ZCLColorFormats.append("Mode 29");
	m_ZCLColorFormats.append("Mode 30");
	m_ZCLColorFormats.append("Mode 31");

	m_ZCLFrameRates.append("1.875 fps");
	m_ZCLFrameRates.append("3.75 fps");
	m_ZCLFrameRates.append("7.5 fps");
	m_ZCLFrameRates.append("15 fps");
	m_ZCLFrameRates.append("30 fps");
	m_ZCLFrameRates.append("60 fps");
	m_ZCLFrameRates.append("120 fps");
	m_ZCLFrameRates.append("240 fps");
	
	m_ZCLColorIDs.append("MONO - 8bit No Conv");
	m_ZCLColorIDs.append("YUV411 - 8bit");
	m_ZCLColorIDs.append("YUV422 - 8bit");
	m_ZCLColorIDs.append("YUV444 - 8bit");
	m_ZCLColorIDs.append("RGB - 8bit");
	m_ZCLColorIDs.append("MONO16 - 16bit");
	m_ZCLColorIDs.append("RGB16 - 16bit");
	m_ZCLColorIDs.append("RGB16 - 16bit");
	m_ZCLColorIDs.append("Signed MONO16	- 16bit");
	m_ZCLColorIDs.append("RAW - 8bit");
	m_ZCLColorIDs.append("RAW16 - 16bit");
	m_ZCLColorIDs.append("MONO - 12bit");
	m_ZCLColorIDs.append("RAW - 12bit");
	m_ZCLColorIDs.append("MONO - 10bit");
	m_ZCLColorIDs.append("RAW - 10bit");
	m_ZCLColorIDs.append("BGR - 8bit No Conv");
	m_ZCLColorIDs.append("BGR - 16bit");
	m_ZCLColorIDs.append("RGBA - 8bit");
	m_ZCLColorIDs.append("BGRA - 8bit No Conv");
	m_ZCLColorIDs.append("RGB V1 - 10bit");
	m_ZCLColorIDs.append("RGB V2 - 10bit");
	m_ZCLColorIDs.append("RGB - 12bit");
	m_ZCLColorIDs.append("RGB - 5bit");
	m_ZCLColorIDs.append("BGR - 5bit No Conv");
	m_ZCLColorIDs.append("YUV422 - 8bit");
}
