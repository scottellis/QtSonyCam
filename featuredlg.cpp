/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include <qlabel.h>
#include <qboxlayout.h>
#include <qformlayout.h>
#include <qvalidator.h>

#include "featuredlg.h"

FeatureDlg::FeatureDlg(QWidget *parent,
                       HCAMERA hCamera,
                       QHash<QString, int> featureIDs, 
	                   QHash<QString, quint32> features,
                       QHash<QString, quint32> feature_min,
	                   QHash<QString, quint32> feature_max,
                        bool have_wb,
                        quint32 wbu,
                        quint32 wbv,
                        quint32 wb_min,
                        quint32 wb_max)                     
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	m_hCamera = hCamera;
	m_featureIDs = featureIDs;
	m_features = features;
	m_pendingFeatures = features;
	m_have_wb = have_wb;
	m_wbu = m_pending_wbu = wbu;
	m_wbv = m_pending_wbv = wbv;

	layoutWindow(feature_min, feature_max, wb_min, wb_max);

	setWindowTitle("Features");

	connect(m_apply, SIGNAL(clicked()), SLOT(onApply()));
	connect(m_ok, SIGNAL(clicked()), SLOT(accept()));
	connect(m_cancel, SIGNAL(clicked()), SLOT(onCancel()));
}

QHash<QString, quint32> FeatureDlg::newFeatures()
{
	int val;
	QHash<QString, quint32> features;
	
	QHash<QString, QLineEdit *>::const_iterator i;

	for (i = m_ctl.constBegin(); i != m_ctl.constEnd(); ++i) {
		QLineEdit *edit = i.value();

		if (edit->isEnabled()) {
			val = edit->text().toInt();
			features.insert(i.key(), val);
		}
	}

	return features;
}

quint32 FeatureDlg::newWhiteBalanceU()
{
	return m_pending_wbu;
}

quint32 FeatureDlg::newWhiteBalanceV()
{
	return m_pending_wbv;
}

QHash<QString, quint32> FeatureDlg::pendingFeatures()
{
	int val;
	QHash<QString, quint32> features;
	
	QHash<QString, QLineEdit *>::const_iterator i;

	for (i = m_ctl.constBegin(); i != m_ctl.constEnd(); ++i) {
		QLineEdit *edit = i.value();

		if (edit->isEnabled()) {
			val = edit->text().toInt();

			if (val != m_pendingFeatures.value(i.key()))
				features.insert(i.key(), val);
		}
	}

	return features;
}

void FeatureDlg::onApply()
{
	ZCL_SETFEATUREVALUE zclSetVal;
	ZCL_FEATUREID id;
	int val;

	QHash<QString, quint32> change = pendingFeatures();

	QHash<QString, quint32>::const_iterator i;

	for (i = change.constBegin(); i != change.constEnd(); ++i) {
		id = (ZCL_FEATUREID) m_featureIDs.value(i.key());
		val = i.value();

		if (setFeatureValue(id, val))
			m_pendingFeatures[i.key()] = val;
	}

	if (m_have_wb) {
		zclSetVal.FeatureID = ZCL_WHITEBALANCE;
		zclSetVal.ReqID = ZCL_VALUE;
		zclSetVal.u.WhiteBalance.UB_Value = m_whiteBalanceUCtl->text().toUInt();
		zclSetVal.u.WhiteBalance.VR_Value = m_whiteBalanceVCtl->text().toUInt();

		if (ZCLSetFeatureValue(m_hCamera, &zclSetVal)) {
			m_pending_wbu = zclSetVal.u.WhiteBalance.UB_Value;
			m_pending_wbv = zclSetVal.u.WhiteBalance.VR_Value;
		}
	}
}

void FeatureDlg::onCancel()
{
	ZCL_SETFEATUREVALUE zclSetVal;

	QHash<QString, quint32>::const_iterator i;

	for (i = m_pendingFeatures.constBegin(); i != m_pendingFeatures.constEnd(); ++i) {
		ZCL_FEATUREID id = (ZCL_FEATUREID) m_featureIDs.value(i.key());
		int val = m_features.value(i.key());
		
		setFeatureValue(id, val);
	}

	if (m_have_wb) {
		if (m_pending_wbu != m_wbu || m_pending_wbv != m_wbv) {
			zclSetVal.FeatureID = ZCL_WHITEBALANCE;
			zclSetVal.ReqID = ZCL_VALUE;
			zclSetVal.u.WhiteBalance.UB_Value = m_wbu;
			zclSetVal.u.WhiteBalance.VR_Value = m_wbv;

			ZCLSetFeatureValue(m_hCamera, &zclSetVal);

			m_pending_wbv = m_wbv;
			m_pending_wbu = m_wbu;
		}
	}

	reject();
}

bool FeatureDlg::setFeatureValue(ZCL_FEATUREID id, quint32 val)
{
	ZCL_SETFEATUREVALUE zclSetVal;

	zclSetVal.FeatureID = (ZCL_FEATUREID) id;
	zclSetVal.ReqID = ZCL_VALUE;
	zclSetVal.u.Std.Value = val;

	if (!ZCLSetFeatureValue(m_hCamera, &zclSetVal))
		return false;

	return true;
}

void FeatureDlg::addControl(QString name,
	                        quint32 current,
	                        QHash<QString, quint32> feature_min,
	                        QHash<QString, quint32> feature_max)
{
	QLineEdit *ctl = new QLineEdit;
	
	quint32 min = feature_min.value(name);
	quint32 max = feature_max.value(name);

	if (current < min)
		current = min;
	else if (current > max)
		current = max;

	ctl->setValidator(new QIntValidator(min, max));
	ctl->setText(QString::number(current));
	ctl->setMaximumWidth(80);

	m_ctl.insert(name, ctl);
}

void FeatureDlg::layoutWindow(QHash<QString, quint32> feature_min,
	                          QHash<QString, quint32> feature_max,
                              quint32 wb_min, quint32 wb_max)
{
	QString minmax;
	QFormLayout *form = new QFormLayout;
	QHash<QString, quint32>::const_iterator i;

	for (i = m_features.constBegin(); i != m_features.constEnd(); ++i) { 
		addControl(i.key(), i.value(), feature_min, feature_max);

		minmax = QString(" [ %1 - %2 ]").arg(feature_min.value(i.key())).arg(feature_max.value(i.key()));
		form->addRow(i.key() + minmax, m_ctl.value(i.key()));
	}

	if (m_have_wb) {
		minmax = QString(" [ %1 - %2 ]").arg(wb_min).arg(wb_max);

		m_whiteBalanceUCtl = new QLineEdit(QString::number(m_wbu));
		m_whiteBalanceUCtl->setValidator(new QIntValidator(wb_min, wb_max));
		m_whiteBalanceUCtl->setMaximumWidth(80);
		form->addRow("White Balance U" + minmax, m_whiteBalanceUCtl);

		m_whiteBalanceVCtl = new QLineEdit(QString::number(m_wbv));
		m_whiteBalanceVCtl->setValidator(new QIntValidator(wb_min, wb_max));
		m_whiteBalanceVCtl->setMaximumWidth(80);
		form->addRow("White Balance V" + minmax, m_whiteBalanceVCtl);
	}
	else {
		m_whiteBalanceUCtl = NULL;
		m_whiteBalanceVCtl = NULL;
	}

    QHBoxLayout *buttons = new QHBoxLayout;
	buttons->addStretch();

	m_apply = new QPushButton("Apply");
	buttons->addWidget(m_apply);

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
