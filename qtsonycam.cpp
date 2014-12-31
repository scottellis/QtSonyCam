/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include <qdir.h>
#include <qmessagebox.h>

#include "qtsonycam.h"
#include "cameradlg.h"
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
	connect(ui.actionCamera, SIGNAL(triggered()), SLOT(onCamera()));
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

		if (m_frameQ.count() > 0)
			m = m_frameQ.dequeue();

		m_frameQMutex.unlock();

		if (m) {
			showFrame(m);
			delete m;
		}
	}	
}

void QtSonyCam::showFrame(cv::Mat *frame)
{
	/*
	QImage img((quint8 *)d.constData(), 1280, 960, QImage::Format_RGB888);
	ui.cameraView->setPixmap(QPixmap::fromImage(img.scaled(ui.cameraView->size(), Qt::KeepAspectRatio)));
	*/
	QImage img((const quint8 *)frame->data, frame->cols, frame->rows, (int)frame->step, QImage::Format_RGB888);
	ui.cameraView->setPixmap(QPixmap::fromImage(img.scaled(ui.cameraView->size(), Qt::KeepAspectRatio)));
}

void QtSonyCam::onStart()
{
	m_cameraThread = new CameraThread(this, m_hCamera, m_color);

	if (m_cameraThread) {
		connect(m_cameraThread, SIGNAL(newFrame(cv::Mat *)), this, SLOT(newFrame(cv::Mat *)), Qt::DirectConnection);
		m_frameCount = 0;
		m_frameTimer = startTimer(3000);
		m_displayTimer = startTimer(10);
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

void QtSonyCam::newFrame(cv::Mat *frame)
{
	cv::Mat *m;

	m_frameCount++;

	if (m_frameQMutex.tryLock()) {
		while (m_frameQ.size() > 0) {
			m = m_frameQ.dequeue();
			delete m;
		}

		m_frameQ.enqueue(frame);
		m_frameQMutex.unlock();
	}
	else {
		delete frame;
	}
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
		//m_zclStdMode = ZCL_SXGA_MONO16;
		m_color = false;
	}
	else {
		m_zclStdMode = ZCL_SXGA_YUV;
		//m_zclStdMode = ZCL_SXGA_RGB;
		m_color = true;
	}

	mode.StdMode_Flag = TRUE;
	mode.u.Std.Mode = m_zclStdMode;

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

	return true;
}

void QtSonyCam::onCamera()
{
	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	CameraDlg dlg(this, m_ZCLFormats, m_ZCLFrameRates);

	dlg.exec();
}

void QtSonyCam::onFeatures()
{
	FeatureDlg dlg(this, m_features, m_featuresMin, m_featuresMax);

	if (QDialog::Accepted == dlg.exec()) {
		QHash<QString, int> newFeatures = dlg.newFeatures();

		QHash<QString, int>::const_iterator i;

		for (i = m_features.constBegin(); i != m_features.constEnd(); ++i) {
			if (newFeatures.contains(i.key())) {
				if (m_features.value(i.key()) != newFeatures.value(i.key())) {
					m_features[i.key()] = newFeatures.value(i.key());
					
					if (!setFeatureValue(i.key())) {
						QMessageBox::warning(this, "Error", QString("Failed to set feature %1").arg(i.key()));
						break;
					}				
				}
			}
		}
	}
}

bool QtSonyCam::setFeatureValue(QString feature)
{
	ZCL_SETFEATUREVALUE value;

	if (!m_hCamera)
		return false;

	if (feature == "Shutter") {
		value.FeatureID = ZCL_SHUTTER;
		value.ReqID = ZCL_VALUE;
		value.u.Std.Value = m_features.value(feature);
	}
	else if (feature == "Gain") {
		value.FeatureID = ZCL_GAIN;
		value.ReqID = ZCL_VALUE;
		value.u.Std.Value = m_features.value(feature);
	}

	if (!ZCLSetFeatureValue(m_hCamera, &value))
		return false;

	return true;
}

void QtSonyCam::initFeatureLists()
{
	ZCL_CHECKFEATURE feature;
	ZCL_GETFEATUREVALUE value;
	int min, max, current;

	if (!m_hCamera)
		return;

	m_features.clear();
	m_featuresMin.clear();
	m_featuresMax.clear();

	feature.FeatureID = ZCL_SHUTTER;

	if (ZCLCheckFeature(m_hCamera, &feature)) {
		if (feature.PresenceFlag) {
			min = feature.u.Std.Min_Value;
			max = feature.u.Std.Max_Value;

			value.FeatureID = feature.FeatureID;

			if (ZCLGetFeatureValue(m_hCamera, &value)) {
				current = value.u.Std.Value;			

				m_features.insert("Shutter", current);
				m_featuresMin.insert("Shutter", min);
				m_featuresMax.insert("Shutter", max);
			}
		}
	}

	feature.FeatureID = ZCL_GAIN;

	if (ZCLCheckFeature(m_hCamera, &feature)) {
		if (feature.PresenceFlag) {
			min = feature.u.Std.Min_Value;
			max = feature.u.Std.Max_Value;

			value.FeatureID = feature.FeatureID;

			if (ZCLGetFeatureValue(m_hCamera, &value)) {
				current = value.u.Std.Value;			

				m_features.insert("Gain", current);
				m_featuresMin.insert("Gain", min);
				m_featuresMax.insert("Gain", max);
			}
		}
	}
}

void QtSonyCam::updateStatusBar()
{
	QString s;

	if (m_cameraUID) {
		m_cameraModelStatus->setText(m_cameraModel);
		m_cameraFormatStatus->setText(m_ZCLFormats.at((int)m_zclStdMode));
		m_cameraFPSStatus->setText(m_ZCLFrameRates.at((int)m_zclFps));
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
	m_ZCLFormats.append("160x120 YUV(4:4:4");
	m_ZCLFormats.append("320x240 YUV(4:2:2)");
	m_ZCLFormats.append("640x480 YUV(4:1:1)");
	m_ZCLFormats.append("640x480 YUV(4:2:2)");
	m_ZCLFormats.append("640x480 RGB");
	m_ZCLFormats.append("640x480 MONO");
	m_ZCLFormats.append("640x480 MONO16");
	m_ZCLFormats.append("800x600 YUV(4:2:2)");
	m_ZCLFormats.append("800x600 RGB");
	m_ZCLFormats.append("800x600 MONO");
	m_ZCLFormats.append("800x600 MONO16");
	m_ZCLFormats.append("1024x768 YUV(4:2:2)");
	m_ZCLFormats.append("1024x768 RGB");
	m_ZCLFormats.append("1024x768 MONO");
	m_ZCLFormats.append("1024x768 MONO16");
	m_ZCLFormats.append("1280x960 YUV(4:2:2)");
	m_ZCLFormats.append("1280x960 RGB");
	m_ZCLFormats.append("1280x960 MONO");
	m_ZCLFormats.append("1280x960 MONO16");
	m_ZCLFormats.append("1600x1200 YUV(4:2:2)");
	m_ZCLFormats.append("1600x1200 RGB");
	m_ZCLFormats.append("1600x1200 MONO");
	m_ZCLFormats.append("1600x1200 MONO16");

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
