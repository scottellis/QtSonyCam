/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#ifndef CAMERADLG_H
#define CAMERADLG_H

#include <qdialog.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qstringlist.h>

#include <Windows.h>
#include <ZCLAPI.h>

class CameraDlg : public QDialog
{
	Q_OBJECT

public:
	explicit CameraDlg(QWidget *parent, QStringList formats, QStringList frameRates);
	~CameraDlg();

public slots:
	void cameraIndexChanged(int index);
	void videoModeIndexChanged(int index);
	void bitShiftIndexChanged(int index);
	void colorFilterIndexChanged(int index);
	void onOK();
	void onCancel();

private:
	void layoutWindow();
	void initSelectionLists();
	void populateControls();
	void getCameraList();
	void getVideoModeList();

	QComboBox *m_cameraCombo;
	QComboBox *m_videoModeCombo;
	QComboBox *m_bitShiftCombo;
	QComboBox *m_colorFilterCombo;

	QPushButton *m_ok;
	QPushButton *m_cancel;

	QString m_cameraString;
	quint64 m_uid;
	HCAMERA m_hCamera;
	ZCL_LIST *m_cameraList;
	QStringList m_formats;
	QStringList m_frameRates;
};

#endif // CAMERADLG_H
