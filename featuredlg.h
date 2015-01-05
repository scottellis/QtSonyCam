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
	                    QHash<QString, quint32> feature_max,
                        bool have_wb,
                        quint32 wbu,
                        quint32 wbv,
                        quint32 wb_min,
                        quint32 wb_max);

	QHash<QString, quint32> newFeatures();

    quint32 newWhiteBalanceU();
    quint32 newWhiteBalanceV();

public slots:
	void onApply();
	void onCancel();

private:
	void layoutWindow(QHash<QString, quint32> feature_min,
	                  QHash<QString, quint32> feature_max,
                      quint32 wb_min, quint32 wb_max);

	void addControl(QString name,
	                quint32 current,
	                QHash<QString, quint32> feature_min,
	                QHash<QString, quint32> feature_max);

	bool setFeatureValue(ZCL_FEATUREID id, quint32 val);
	QHash<QString, quint32> pendingFeatures();

	QHash<QString, QLineEdit *> m_ctl;
	QLineEdit *m_whiteBalanceUCtl;
	QLineEdit *m_whiteBalanceVCtl;

	QPushButton *m_apply;
	QPushButton *m_ok;
	QPushButton *m_cancel;

	HCAMERA m_hCamera;
    QHash<QString, int> m_featureIDs; 
    QHash<QString, quint32> m_features;
	QHash<QString, quint32> m_pendingFeatures;

	bool m_have_wb;
	quint32 m_wbu;
	quint32 m_wbv;
	quint32 m_pending_wbu;
	quint32 m_pending_wbv;
};

#endif // FEATUREDLG_H
