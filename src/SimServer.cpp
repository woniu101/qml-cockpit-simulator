#include "SimServer.h"
#include "VehicleProtocol.h"
#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>

SimServer::SimServer(QObject *parent):QObject(parent) {
    m_server.setSocketOptions(QLocalServer::UserAccessOption);connect(&m_server,&QLocalServer::newConnection,this,&SimServer::accept);
    m_publish.setInterval(100);connect(&m_publish,&QTimer::timeout,this,&SimServer::snapshot);
    connect(&m_vehicle,&SimVehicle::changed,this,[this]{if(!m_applying)checkPending();});
}
SimServer::~SimServer(){stop();}
void SimServer::log(const QString &line){emit logLine(QTime::currentTime().toString("HH:mm:ss.zzz")+"  "+line);}
bool SimServer::start(const QString &name) {
    if(running())return true;
    if(name.isEmpty()||name.size()>120){log(QStringLiteral("服务名称无效"));return false;}
    const auto hash=QCryptographicHash::hash(name.toUtf8(),QCryptographicHash::Sha256).toHex().left(16);
    m_lock=std::make_unique<QLockFile>(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("qml-cockpit-"+hash+".lock"));
    if(!m_lock->tryLock(0)){log(QStringLiteral("同名模拟器已经运行"));m_lock.reset();return false;}
    if(!m_server.listen(name)){log(m_server.errorString());m_lock.reset();return false;}
    m_session=QUuid::createUuid().toString(QUuid::WithoutBraces);m_sequence=0;m_publish.start();log(QStringLiteral("服务已启动：")+name);emit statusChanged();return true;
}
void SimServer::stop(){m_publish.stop();m_pending.clear();m_delayed.clear();for(auto *socket:m_clients.keys())socket->abort();m_server.close();m_lock.reset();emit statusChanged();}
int SimServer::cockpitCount() const {int n=0;for(const auto &c:m_clients)if(c.ready&&c.role=="cockpit")++n;return n;}
void SimServer::accept(){while(m_server.hasPendingConnections()){
    auto *socket=m_server.nextPendingConnection();if(m_clients.size()>=8){socket->abort();socket->deleteLater();continue;}
    socket->setReadBufferSize(VehicleProtocol::MaxFrame*4);Client c;c.connection=QUuid::createUuid().toString();m_clients.insert(socket,c);
    connect(socket,&QLocalSocket::readyRead,this,[this,socket]{read(socket);});connect(socket,&QLocalSocket::disconnected,this,[this,socket]{remove(socket);});
    QTimer::singleShot(3000,socket,[this,socket]{if(m_clients.contains(socket)&&!m_clients[socket].ready)socket->abort();});
}}
void SimServer::remove(QLocalSocket *socket){
    const auto c=m_clients.take(socket);for(auto i=m_pending.begin();i!=m_pending.end();)if(i->socket==socket)i=m_pending.erase(i);else ++i;
    for(auto i=m_delayed.begin();i!=m_delayed.end();)if(i->socket==socket)i=m_delayed.erase(i);else ++i;
    if(c.ready)log(QStringLiteral("客户端断开：")+c.role);socket->deleteLater();emit statusChanged();
}
void SimServer::send(QLocalSocket *socket,const QJsonObject &object){if(socket&&socket->state()==QLocalSocket::ConnectedState)socket->write(VehicleProtocol::encode(object));}
void SimServer::read(QLocalSocket *socket){
    if(!m_clients.contains(socket))return;auto &buffer=m_clients[socket].buffer;buffer+=socket->read(VehicleProtocol::MaxFrame*4);
    QList<QJsonObject> messages;QString error;if(!VehicleProtocol::decode(buffer,messages,error)){log(QStringLiteral("协议错误：")+error);socket->abort();return;}
    for(const auto &m:messages){if(!m_clients.contains(socket))break;message(socket,m);}
}
void SimServer::message(QLocalSocket *socket,const QJsonObject &m){
    auto &client=m_clients[socket];const auto type=m["type"].toString();
    if(!client.ready){
        const auto role=m["role"].toString();if(type!="hello"||!QStringList{"cockpit","operator"}.contains(role)){socket->abort();return;}
        client.ready=true;client.role=role;send(socket,{{"type","hello"},{"session",m_session},{"server",QStringLiteral("QML Cockpit Simulator")}});log(QStringLiteral("客户端已连接：")+role);snapshot();emit statusChanged();return;
    }
    if(type=="control"&&client.role=="operator"){
        const auto error=control(m["action"].toString(),m["args"].toObject());send(socket,{{"type","controlResult"},{"id",m["id"]},{"ok",error.isEmpty()},{"reason",error}});return;
    }
    if(type!="command"||client.role!="cockpit")return;
    const auto id=m["id"].toString();if(id.isEmpty()||id.size()>80||!m["args"].isObject()||!m["kind"].isString()||m["session"].toString()!=m_session){socket->abort();return;}
    if(client.requests.contains(id)){
        if(client.requests[id]!=m){socket->abort();return;}if(client.replies.contains(id))send(socket,client.replies[id]);return;
    }
    if(client.requests.size()>=256){const auto oldest=client.requests.begin().key();client.requests.remove(oldest);client.replies.remove(oldest);}
    client.requests[id]=m;log(QStringLiteral("请求 ")+id.left(8)+"  "+m["kind"].toString());
    if(m_drop){log(QStringLiteral("故障注入：请求已丢弃"));return;}
    if(m_reject){result(socket,id,"rejected",QStringLiteral("模拟器拒绝了本次操作（故障注入）"));return;}
    if(m_delay>0){
        const auto error=m_vehicle.commandError(m["kind"].toString(),m["args"].toObject());
        if(!error.isEmpty()){result(socket,id,"rejected",error);return;}
        const auto resources=SimVehicle::resources(m["kind"].toString(),m["args"].toObject());
        cancelDelayed(resources,QStringLiteral("尚未执行的指令已被新操作替换"));
        const auto key=client.connection+":"+id;m_delayed.insert(key,{socket,m,resources});
        QTimer::singleShot(m_delay,this,[this,key]{if(!m_delayed.contains(key))return;const auto delayed=m_delayed.take(key);execute(delayed.socket,delayed.message);});
    }else execute(socket,m);
}
void SimServer::execute(QPointer<QLocalSocket> socket,const QJsonObject &m){
    if(!socket||!m_clients.contains(socket)||m["session"].toString()!=m_session)return;
    const auto id=m["id"].toString(),kind=m["kind"].toString();const auto args=m["args"].toObject();
    m_applying=true;const auto error=m_vehicle.execute(kind,args);m_applying=false;
    if(!error.isEmpty()){result(socket,id,"rejected",error);return;}
    const auto resources=SimVehicle::resources(kind,args);cancelResources(resources,QStringLiteral("已被新操作替换"));
    result(socket,id,"accepted",QStringLiteral("已接收，等待实际状态"));
    m_pending.insert(m_clients[socket].connection+":"+id,{socket,id,kind,args,resources});snapshot();checkPending();
}
void SimServer::result(QLocalSocket *socket,const QString &id,const QString &status,const QString &reason){
    if(!m_clients.contains(socket))return;QJsonObject reply{{"type","result"},{"session",m_session},{"id",id},{"status",status},{"reason",reason}};m_clients[socket].replies[id]=reply;send(socket,reply);log(id.left(8)+"  "+status+"  "+reason);
}
void SimServer::checkPending(){
    QStringList completed;for(auto i=m_pending.cbegin();i!=m_pending.cend();++i)if(m_vehicle.completed(i->kind,i->args))completed<<i.key();
    if(completed.isEmpty())return;snapshot();for(const auto &key:completed){const auto p=m_pending.take(key);result(p.socket,p.id,"completed",QStringLiteral("已完成"));}
}
void SimServer::cancelResources(const QStringList &resources,const QString &reason){
    cancelDelayed(resources,reason);
    QStringList cancelled;for(auto i=m_pending.cbegin();i!=m_pending.cend();++i)for(const auto &key:resources)if(i->resources.contains(key)){cancelled<<i.key();break;}
    for(const auto &key:cancelled){const auto p=m_pending.take(key);result(p.socket,p.id,"cancelled",reason);}
}
void SimServer::cancelDelayed(const QStringList &resources,const QString &reason){
    QStringList cancelled;for(auto i=m_delayed.cbegin();i!=m_delayed.cend();++i)for(const auto &key:resources)if(i->resources.contains(key)){cancelled<<i.key();break;}
    for(const auto &key:cancelled){const auto p=m_delayed.take(key);result(p.socket,p.message["id"].toString(),"cancelled",reason);}
}
void SimServer::snapshot(){if(m_pause)return;const QJsonObject message{{"type","snapshot"},{"session",m_session},{"sequence",double(++m_sequence)},{"sentAt",double(QDateTime::currentMSecsSinceEpoch())},{"state",m_vehicle.snapshot()}};for(auto *socket:m_clients.keys())if(m_clients[socket].ready)send(socket,message);}
QString SimServer::control(const QString &action,const QJsonObject &args){
    if(action=="faults"){
        for(const char *key:{"pauseTelemetry","reject","drop"})if(args.contains(key)&&!args[key].isBool())return QStringLiteral("故障开关必须是布尔值");
        if(args.contains("delayMs")&&!VehicleProtocol::number(args["delayMs"],0,10000,true))return QStringLiteral("延迟应为 0–10000 ms");
        if(args.contains("pauseTelemetry"))m_pause=args["pauseTelemetry"].toBool();if(args.contains("reject"))m_reject=args["reject"].toBool();if(args.contains("drop"))m_drop=args["drop"].toBool();if(args.contains("delayMs"))m_delay=args["delayMs"].toInt();
        log(QStringLiteral("故障设置已更新"));emit statusChanged();snapshot();return {};
    }
    if(action=="disconnect"){
        for(auto *socket:m_clients.keys())if(m_clients[socket].role=="cockpit")socket->abort();return {};
    }
    m_applying=true;const auto error=m_vehicle.inject(action,args);m_applying=false;if(!error.isEmpty()){log(error);return error;}
    if(action=="reset"){
        const auto pending=m_pending;m_pending.clear();for(const auto &p:pending)result(p.socket,p.id,"cancelled",QStringLiteral("模拟器已重置"));
        const auto delayed=m_delayed;m_delayed.clear();for(const auto &p:delayed)result(p.socket,p.message["id"].toString(),"cancelled",QStringLiteral("模拟器已重置，延迟指令已取消"));
    }
    else cancelResources(action=="window"?QStringList{"window"+QString::number(args["index"].toInt())}:SimVehicle::resources(action,args),QStringLiteral("状态已由模拟器修改"));
    log(QStringLiteral("控制台更新：")+action);snapshot();checkPending();return {};
}
