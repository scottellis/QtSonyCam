/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include <qlabel.h>
#include <qboxlayout.h>
#include <qformlayout.h>

#include "cameradlg.h"

CameraDlg::CameraDlg(QWidget *parent, QStringList formats, QStringList frameRates)                      
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	m_cameraList = NULL;
	m_uid = 0;
	m_hCamera = NULL;
	m_formats = formats;
	m_frameRates = frameRates;

	layoutWindow();
	populateControls();

	setWindowTitle("Camera");

	connect(m_ok, SIGNAL(clicked()), SLOT(onOK()));
	connect(m_cancel, SIGNAL(clicked()), SLOT(onCancel()));
	connect(m_cameraCombo, SIGNAL(currentIndexChanged(int)), SLOT(cameraIndexChanged(int)));
	connect(m_videoModeCombo, SIGNAL(currentIndexChanged(int)), SLOT(videoModeIndexChanged(int)));
	connect(m_bitShiftCombo, SIGNAL(currentIndexChanged(int)), SLOT(bitShiftIndexChanged(int)));
	connect(m_colorFilterCombo, SIGNAL(currentIndexChanged(int)), SLOT(colorFilterIndexChanged(int)));
}

CameraDlg::~CameraDlg()
{
	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	if (m_cameraList) {
		free(m_cameraList);
		m_cameraList = NULL;
	}
}

void CameraDlg::onOK()
{
	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	accept();
}

void CameraDlg::onCancel()
{
	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	reject();
}

void CameraDlg::cameraIndexChanged(int index)
{
	if (!m_cameraList)
		m_uid = 0;

	if (index < 0)
		m_uid = 0;

	if (index > m_cameraList->CameraCount)
		m_uid = 0;

	m_uid = m_cameraList->Info[index].UID;

	if (m_hCamera) {
		ZCLClose(m_hCamera);
		m_hCamera = NULL;
	}

	if (!ZCLOpen(m_uid, &m_hCamera)) {
		m_uid = 0;
		return;
	}

	getVideoModeList();
}

void CameraDlg::videoModeIndexChanged(int index)
{
}

void CameraDlg::bitShiftIndexChanged(int index)
{
}

void CameraDlg::colorFilterIndexChanged(int index)
{
}

void CameraDlg::populateControls()
{
	getCameraList();

	if (!m_cameraList)
		return;
}

void CameraDlg::getCameraList()
{
	DWORD count = 0;

	if (!ZCLGetList((ZCL_LIST *)&count)) {
		m_cameraCombo->addItem("<No Cameras Found>");
		return;
	}

	m_cameraList = (ZCL_LIST *)malloc(sizeof(ZCL_LIST) + (sizeof(ZCL_CAMERAINFO) * count));

	if (!m_cameraList)
		return;

	m_cameraList->CameraCount = count;
	
	if (!ZCLGetList(m_cameraList))
		return;

	for (int i = 0; i < m_cameraList->CameraCount; i++) {
		QString s;

		s.sprintf("%s : 0x%08x%08x", (char *)m_cameraList->Info[i].ModelName,
			(DWORD)((m_cameraList->Info[i].UID & 0xffffffff00000000) >> 32),
			(DWORD)(m_cameraList->Info[i].UID & 0x00000000ffffffff));

		m_cameraCombo->addItem(s);

		if (m_uid == m_cameraList->Info[i].UID)
			m_cameraCombo->setCurrentIndex(i);
	}

	// force a change event when we only have one camera
	if (m_cameraCombo->currentIndex() < 1)
		cameraIndexChanged(0);
}

void CameraDlg::getVideoModeList()
{
	ZCL_CAMERAMODE mode;

	m_videoModeCombo->clear();

	if (!m_hCamera)
		return;

	for (int i = 0; i < m_formats.count(); i++) {
		for (int j = 0; j < m_frameRates.count(); j++) {
			mode.StdMode_Flag = TRUE;
			mode.u.Std.Mode = (ZCL_STDMODE) i;
			mode.u.Std.FrameRate = (ZCL_FPS) j;

			if (ZCLCheckCameraMode(m_hCamera, &mode)) {
				QString s = QString("%1 @ %2").arg(m_formats.at(i)).arg(m_frameRates.at(j));
				quint32 data = (i << 16) | j;
				m_videoModeCombo->addItem(s, data);
			}
		}
	}	
}

void CameraDlg::layoutWindow()
{
    QFormLayout *form = new QFormLayout;

	m_cameraCombo = new QComboBox;
	form->addRow("Camera", m_cameraCombo);

	m_videoModeCombo = new QComboBox;
	form->addRow("Video Mode", m_videoModeCombo);

	m_bitShiftCombo = new QComboBox;
	form->addRow("Bit Shift", m_bitShiftCombo);

	m_colorFilterCombo = new QComboBox;	
	form->addRow("Color Filter", m_colorFilterCombo);
	
    QHBoxLayout *buttons = new QHBoxLayout;
	buttons->addStretch();

    m_ok = new QPushButton("OK");
    buttons->addWidget(m_ok);

    m_cancel = new QPushButton("Cancel");
    buttons->addWidget(m_cancel);

    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    centralLayout->addLayout(form);
	centralLayout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
    centralLayout->addLayout(buttons);	
}
