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
                        HCAMERA hCamera,
                        QHash<QString, int> featureIDs, 
	                    QHash<QString, int> features,
	                    QHash<QString, int> feature_min,
	                    QHash<QString, int> feature_max);

	QHash<QString, int> newFeatures();

public slots:
	void onApply();
	void onCancel();

private:
	void layoutWindow(QHash<QString, int> features,
	                  QHash<QString, int> feature_min,
	                  QHash<QString, int> feature_max);

	void addControl(QString name,
	                QHash<QString, int> features,
	                QHash<QString, int> feature_min,
	                QHash<QString, int> feature_max);

	bool setFeatureValue(ZCL_FEATUREID id, int val);
	QHash<QString, int> pendingFeatures();

	QHash<QString, QLineEdit *> m_ctl;

	QPushButton *m_apply;
	QPushButton *m_ok;
	QPushButton *m_cancel;

	HCAMERA m_hCamera;
    QHash<QString, int> m_featureIDs; 
    QHash<QString, int> m_features;
	QHash<QString, int> m_pendingFeatures;
};

#endif // FEATUREDLG_H
