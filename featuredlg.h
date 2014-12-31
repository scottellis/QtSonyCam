/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#ifndef FEATUREDLG_H
#define FEATUREDLG_H

#include <qdialog.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qhash.h>

#include <Windows.h>
#include <ZCLAPI.h>

class FeatureDlg : public QDialog
{
	Q_OBJECT

public:
	explicit FeatureDlg(QWidget *parent, 
	                    QHash<QString, int> features,
	                    QHash<QString, int> feature_min,
	                    QHash<QString, int> feature_max);

	QHash<QString, int> newFeatures();

private:
	void layoutWindow(QHash<QString, int> features,
	                  QHash<QString, int> feature_min,
	                  QHash<QString, int> feature_max);

	void addControl(QString name,
	                QHash<QString, int> features,
	                QHash<QString, int> feature_min,
	                QHash<QString, int> feature_max);

	QHash<QString, QLineEdit *> m_ctl;

	QPushButton *m_ok;
	QPushButton *m_cancel;
};

#endif // FEATUREDLG_H
