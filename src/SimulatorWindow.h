#pragma once
#include <QMainWindow>
#include "SimServer.h"
class QLabel;class QLineEdit;class QPushButton;class QDoubleSpinBox;class QSpinBox;class QComboBox;class QCheckBox;class QPlainTextEdit;
class QBoxLayout;
class SimulatorWindow:public QMainWindow {
    Q_OBJECT
public:
    explicit SimulatorWindow(SimServer *server,const QString &name);
private:
    void refresh();void apply(const QString&,const QJsonObject&);
    SimServer *m_server;QLabel *m_status,*m_trip,*m_charge;QLineEdit *m_name;QPushButton *m_start;
    QDoubleSpinBox *m_soc;QSpinBox *m_speed,*m_target,*m_delay;QComboBox *m_gear;
    std::array<QSpinBox*,4> m_windows;std::array<QLabel*,4> m_windowInfo;std::array<QCheckBox*,4> m_doors;
    QCheckBox *m_port,*m_plug,*m_pause,*m_reject,*m_drop;QPlainTextEdit *m_log;
    QPushButton *m_drive,*m_park,*m_advance,*m_close,*m_vent,*m_closeDoors,*m_beginCharge,*m_stopCharge,*m_disconnect,*m_clearFaults;
    std::array<QCheckBox*,4> m_bodyParts;
    QLabel *m_driveHint,*m_chargeHint,*m_faultSummary;
    QBoxLayout *m_columns=nullptr;
protected:
    void resizeEvent(QResizeEvent *event) override;
};
