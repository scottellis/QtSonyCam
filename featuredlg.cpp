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
	                   QHash<QString, int> features,
                       QHash<QString, int> feature_min,
	                   QHash<QString, int> feature_max)                     
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow(features, feature_min, feature_max);

	setWindowTitle("Features");

	connect(m_ok, SIGNAL(clicked()), SLOT(accept()));
	connect(m_cancel, SIGNAL(clicked()), SLOT(reject()));
}

QHash<QString, int> FeatureDlg::newFeatures()
{
	int val;
	QHash<QString, int> features;
	
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

void FeatureDlg::addControl(QString name,
	                        QHash<QString, int> features,
	                        QHash<QString, int> feature_min,
	                        QHash<QString, int> feature_max)
{
	QLineEdit *ctl = new QLineEdit;
	
	if (features.contains(name)) {
		int current = features.value(name);
		int min = feature_min.value(name, -1);
		int max = feature_max.value(name, -1);

		if (min != -1 && max != -1 && min < max) {
			if (current < min)
				current = min;
			else if (current > max)
				current = max;

			ctl->setValidator(new QIntValidator(min, max));
		}

		ctl->setText(QString::number(current));
		ctl->setMaximumWidth(80);
	}
	else {
		ctl->setEnabled(false);
	}

	m_ctl.insert(name, ctl);
}

void FeatureDlg::layoutWindow(QHash<QString, int> features,
	                          QHash<QString, int> feature_min,
	                          QHash<QString, int> feature_max)
{
	QString minmax;
	QFormLayout *form = new QFormLayout;
	QHash<QString, int>::const_iterator i;

	for (i = features.constBegin(); i != features.constEnd(); ++i) { 
		addControl(i.key(), features, feature_min, feature_max);

		minmax = QString(" (%1 to %2)").arg(feature_min.value(i.key())).arg(feature_max.value(i.key()));
		form->addRow(i.key() + minmax, m_ctl.value(i.key()));
	}

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
