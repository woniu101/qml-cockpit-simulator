#pragma once
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <array>

class SimVehicle:public QObject {
    Q_OBJECT
public:
    explicit SimVehicle(QObject *parent=nullptr);
    QJsonObject snapshot() const;
    QString commandError(const QString &kind,const QJsonObject &args={}) const;
    QString execute(const QString &kind,const QJsonObject &args);
    QString inject(const QString &action,const QJsonObject &args);
    bool completed(const QString &kind,const QJsonObject &args) const;
    static QStringList resources(const QString &kind,const QJsonObject &args);
    void advance(double seconds);
    void reset();
signals:
    void changed();
private:
    void tick();
    QString driveError() const;
    void energy(double seconds);
    QTimer m_timer;QElapsedTimer m_clock;
    std::array<double,4> m_windows{};std::array<int,4> m_targets{};
    std::array<bool,4> m_moving{},m_doors{};
    std::array<double,4> m_doorRemaining{};
    double m_hoodRemaining=0,m_hatchRemaining=0;
    QString m_gear="P";int m_speed=0,m_chargeTarget=90;
    double m_soc=82,m_trip=0,m_forward=0;
    bool m_locked=true,m_hood=false,m_hatch=false,m_port=false,m_plugged=false,m_charging=false,m_mirrors=false;
};
