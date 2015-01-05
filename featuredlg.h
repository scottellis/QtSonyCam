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
	                    QHash<QString, quint32> features,
	                    QHash<QString, quint32> feature_min,
	                    QHash<QString, quint32> feature_max);

	QHash<QString, quint32> newFeatures();

public slots:
	void onApply();
	void onCancel();

private:
	void layoutWindow(QHash<QString, quint32> features,
	                  QHash<QString, quint32> feature_min,
	                  QHash<QString, quint32> feature_max);

	void addControl(QString name,
	                QHash<QString, quint32> features,
	                QHash<QString, quint32> feature_min,
	                QHash<QString, quint32> feature_max);

	bool setFeatureValue(ZCL_FEATUREID id, quint32 val);
	QHash<QString, quint32> pendingFeatures();

	QHash<QString, QLineEdit *> m_ctl;

	QPushButton *m_apply;
	QPushButton *m_ok;
	QPushButton *m_cancel;

	HCAMERA m_hCamera;
    QHash<QString, int> m_featureIDs; 
    QHash<QString, quint32> m_features;
	QHash<QString, quint32> m_pendingFeatures;
};

#endif // FEATUREDLG_H
