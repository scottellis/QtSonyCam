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
	                   QHash<QString, quint32> feature_max)                     
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	m_hCamera = hCamera;
	m_featureIDs = featureIDs;
	m_features = features;
	m_pendingFeatures = features;

	layoutWindow(features, feature_min, feature_max);

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
	ZCL_SETFEATUREVALUE value;

	QHash<QString, quint32> change = pendingFeatures();

	QHash<QString, quint32>::const_iterator i;

	for (i = change.constBegin(); i != change.constEnd(); ++i) {
		ZCL_FEATUREID id = (ZCL_FEATUREID) m_featureIDs.value(i.key());
		int val = i.value();

		if (setFeatureValue(id, val))
			m_pendingFeatures[i.key()] = val;
	}
}

void FeatureDlg::onCancel()
{
	ZCL_SETFEATUREVALUE value;

	QHash<QString, quint32>::const_iterator i;

	for (i = m_pendingFeatures.constBegin(); i != m_pendingFeatures.constEnd(); ++i) {
		ZCL_FEATUREID id = (ZCL_FEATUREID) m_featureIDs.value(i.key());
		int val = m_features.value(i.key());
		
		setFeatureValue(id, val);
	}

	reject();
}

bool FeatureDlg::setFeatureValue(ZCL_FEATUREID id, quint32 val)
{
	ZCL_SETFEATUREVALUE value;

	value.FeatureID = (ZCL_FEATUREID) id;
	value.ReqID = ZCL_VALUE;
	value.u.Std.Value = val;

	if (!ZCLSetFeatureValue(m_hCamera, &value))
		return false;

	return true;
}

void FeatureDlg::addControl(QString name,
	                        QHash<QString, quint32> features,
	                        QHash<QString, quint32> feature_min,
	                        QHash<QString, quint32> feature_max)
{
	QLineEdit *ctl = new QLineEdit;
	
	if (features.contains(name)) {
		quint32 current = features.value(name);
		quint32 min = feature_min.value(name);
		quint32 max = feature_max.value(name);

		if (current < min)
			current = min;
		else if (current > max)
			current = max;

		ctl->setValidator(new QIntValidator(min, max));
		ctl->setText(QString::number(current));
		ctl->setMaximumWidth(80);
	}
	else {
		ctl->setEnabled(false);
	}

	m_ctl.insert(name, ctl);
}

void FeatureDlg::layoutWindow(QHash<QString, quint32> features,
	                          QHash<QString, quint32> feature_min,
	                          QHash<QString, quint32> feature_max)
{
	QString minmax;
	QFormLayout *form = new QFormLayout;
	QHash<QString, quint32>::const_iterator i;

	for (i = features.constBegin(); i != features.constEnd(); ++i) { 
		addControl(i.key(), features, feature_min, feature_max);

		minmax = QString(" [%1 - %2]").arg(feature_min.value(i.key())).arg(feature_max.value(i.key()));
		form->addRow(i.key() + minmax, m_ctl.value(i.key()));
	}

    QHBoxLayout *buttons = new QHBoxLayout;
	buttons->addStretch();

	m_apply = new QPushButton("Apply");
	buttons->addWidget(m_apply);

    m_ok = new QPushButton("OK");
    buttons->addWidget(m_ok);

    m_cancel = new QPushButton("Cancel");
    buttons->addWidget(m_cancel);

    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    centralLayout->addLayout(form);
	centralLayout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
    centralLayout->addLayout(buttons);	
}
