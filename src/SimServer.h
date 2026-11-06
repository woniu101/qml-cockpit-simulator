#pragma once
#include "SimVehicle.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QPointer>
#include <QHash>
#include <memory>

class SimServer:public QObject {
    Q_OBJECT
public:
    explicit SimServer(QObject *parent=nullptr);
    ~SimServer() override;
    bool start(const QString &name);
    void stop();
    bool running() const{return m_server.isListening();}
    QString name() const{return m_server.serverName();}
    int cockpitCount() const;
    QString control(const QString &action,const QJsonObject &args);
    QString shiftFromDriver(const QString &gear);
    quint32 beginWindowHold(int index, bool opening);
    void endWindowHold(int index, quint32 hold);
    SimVehicle *model(){return &m_vehicle;}
    bool telemetryPaused() const{return m_pause;}
    bool rejecting() const{return m_reject;}
    bool dropping() const{return m_drop;}
    int delayMs() const{return m_delay;}
signals:
    void logLine(const QString &line);
    void statusChanged();
private:
    struct Client{QByteArray buffer;QString role;bool ready=false;QString connection;QHash<QString,QJsonObject> requests,replies;};
    struct Pending{QPointer<QLocalSocket> socket;QString id,kind;QJsonObject args;QStringList resources;};
    struct Delayed{QPointer<QLocalSocket> socket;QJsonObject message;QStringList resources;};
    void accept();void read(QLocalSocket *socket);void message(QLocalSocket*,const QJsonObject&);void remove(QLocalSocket*);
    void send(QLocalSocket*,const QJsonObject&);void snapshot();void checkPending();
    void result(QLocalSocket*,const QString&,const QString&,const QString&);
    void execute(QPointer<QLocalSocket>,const QJsonObject&);
    void cancelResources(const QStringList&,const QString&);
    void cancelDelayed(const QStringList&,const QString&);
    void log(const QString&);
    SimVehicle m_vehicle;QLocalServer m_server;QTimer m_publish;
    QHash<QLocalSocket*,Client> m_clients;QHash<QString,Pending> m_pending;
    QHash<QString,Delayed> m_delayed;
    std::array<quint32,4> m_windowHolds{};
    quint32 m_nextHold=0;
    std::unique_ptr<QLockFile> m_lock;
    QString m_session;quint64 m_sequence=0;bool m_applying=false,m_pause=false,m_reject=false,m_drop=false;int m_delay=0;
};
