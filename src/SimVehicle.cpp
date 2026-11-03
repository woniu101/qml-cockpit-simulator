#include "SimVehicle.h"
#include "VehicleProtocol.h"
#include <QJsonArray>
#include <algorithm>

SimVehicle::SimVehicle(QObject *parent):QObject(parent) {
    m_timer.setInterval(20);m_timer.setTimerType(Qt::PreciseTimer);connect(&m_timer,&QTimer::timeout,this,&SimVehicle::tick);m_clock.start();m_timer.start();
}
QJsonObject SimVehicle::snapshot() const {
    QJsonArray actual,targets,moving,doors,doorMoving;
    for(int i=0;i<4;++i){actual.append(m_windows[i]);targets.append(m_targets[i]);moving.append(m_moving[i]);doors.append(m_doors[i]);doorMoving.append(m_doorRemaining[i]>0);}
    return {{"soc",m_soc},{"gear",m_gear},{"speed",m_speed},{"tripKm",m_trip},{"forwardKm",m_forward},{"windows",actual},{"targets",targets},{"windowMoving",moving},{"doors",doors},{"doorMoving",doorMoving},
        {"locked",m_locked},{"hood",m_hood},{"hatch",m_hatch},{"hoodMoving",m_hoodRemaining>0},{"hatchMoving",m_hatchRemaining>0},{"port",m_port},{"mirrors",m_mirrors},{"plugged",m_plugged},{"charging",m_charging},{"chargeTarget",m_chargeTarget},{"chargePower",m_charging?55:0}};
}
QString SimVehicle::driveError() const {
    if(m_soc<=0)return QStringLiteral("电量不足，请先充电");
    if(m_hood||m_hatch||m_port||m_plugged||m_hoodRemaining>0||m_hatchRemaining>0)return QStringLiteral("请关闭备厢、充电口并断开充电枪");
    for(int i=0;i<4;++i)if(m_doors[i]||m_doorRemaining[i]>0)return QStringLiteral("请等待全部车门关闭");
    return {};
}
QStringList SimVehicle::resources(const QString &kind,const QJsonObject &a) {
    if(kind=="setWindow"||kind=="stopWindow")return {"window"+QString::number(a["index"].toInt())};
    if(kind=="closeWindows"||kind=="ventilateWindows")return {"window0","window1","window2","window3"};
    if(kind=="setDoor")return {"door"+QString::number(a["index"].toInt())};
    if(kind=="closeDoors")return {"door0","door1","door2","door3"};
    if(kind=="setHood")return {"hood"};if(kind=="setHatch")return {"hatch"};
    if(kind=="setGear"||kind=="setSpeed"||kind=="startDriving"||kind=="park")return {"motion"};
    if(kind=="startCharging"||kind=="stopCharging")return {"chargeSwitch"};
    if(kind=="setPort")return {"port"};if(kind=="setPlugged")return {"plug"};
    if(kind=="setChargeTarget")return {"chargeTarget"};if(kind=="setLocked")return {"lock"};if(kind=="setMirrors")return {"mirrors"};return {};
}
QString SimVehicle::commandError(const QString &kind,const QJsonObject &a) const {
    using VehicleProtocol::number;
    if(kind=="setWindow"||kind=="stopWindow"){
        if(!number(a["index"],0,3,true))return QStringLiteral("车窗位置无效");
        if(kind=="setWindow"&&!number(a["target"],0,100,true))return QStringLiteral("车窗目标应为 0–100%");
    }else if(kind=="closeWindows"||kind=="ventilateWindows"||kind=="stopCharging")return {};
    else if(kind=="setDoor"||kind=="closeDoors"){
        if(kind=="setDoor"&&(!number(a["index"],0,3,true)||!a["open"].isBool()))return QStringLiteral("车门参数无效");
        if(m_gear!="P")return QStringLiteral("请挂 P 挡后操作车门");
    }else if(kind=="setLocked"){
        if(!a["value"].isBool())return QStringLiteral("门锁参数无效");
        if(m_gear!="P")return QStringLiteral("行驶中保持车门锁定");
        if(a["value"].toBool())for(int i=0;i<4;++i)if(m_doors[i]||m_doorRemaining[i]>0)return QStringLiteral("请等待全部车门关闭后锁车");
    }else if(kind=="setHood"||kind=="setHatch"||kind=="setPort"||kind=="setMirrors"){
        if(!a["value"].isBool())return QStringLiteral("部件参数无效");
        if(m_gear!="P")return QStringLiteral("请挂 P 挡后操作车辆部件");
        if(kind=="setPort"&&m_plugged&&a["value"].toBool()!=m_port)return QStringLiteral("请先断开充电枪");
    }else if(kind=="setGear"||kind=="startDriving"||kind=="park"){
        const auto gear=kind=="startDriving"?QString("D"):kind=="park"?QString("P"):a["value"].toString();
        if(!QStringList{"P","R","N","D"}.contains(gear))return QStringLiteral("挡位无效");
        if(gear!="P"&&(m_plugged||m_charging))return QStringLiteral("请暂停充电并断开充电枪后换挡");
        if(gear=="D"||gear=="R")return driveError();
    }else if(kind=="setSpeed"){
        if(!number(a["value"],0,m_gear=="R"?15:100,true))return QStringLiteral("车速超出允许范围");
        if(a["value"].toInt()>0){if(m_gear!="D"&&m_gear!="R")return QStringLiteral("请先挂入 D 或 R 挡");return driveError();}
    }else if(kind=="setPlugged"){
        if(!a["value"].isBool())return QStringLiteral("插枪参数无效");
        if(a["value"].toBool()==m_plugged)return {};
        if(m_gear!="P"||!m_port)return QStringLiteral("请驻车并打开充电口");
        if(m_charging)return QStringLiteral("请先暂停充电");
    }else if(kind=="setChargeTarget"){
        if(!number(a["value"],50,100,true))return QStringLiteral("目标电量应为 50–100%");
    }else if(kind=="startCharging"){
        if(m_gear!="P"||!m_plugged||!m_port)return QStringLiteral("请驻车并连接充电枪");
        if(m_soc>=m_chargeTarget)return QStringLiteral("已达到目标电量，请提高目标后再充电");
    }else if(kind=="advance"){
        if(!number(a["seconds"],0.01,3600))return QStringLiteral("快进时间无效");
    }else return QStringLiteral("模拟器不支持此指令");
    return {};
}
QString SimVehicle::execute(const QString &kind,const QJsonObject &a) {
    const auto error=commandError(kind,a);if(!error.isEmpty())return error;
    const int index=a["index"].toInt(-1);const bool value=a["value"].toBool();
    if(kind=="setWindow"){
        m_targets[index]=a["target"].toInt();m_moving[index]=std::abs(m_windows[index]-m_targets[index])>0.001;
    }else if(kind=="stopWindow"){
        m_windows[index]=m_targets[index]=qRound(m_windows[index]);m_moving[index]=false;
    }else if(kind=="closeWindows"||kind=="ventilateWindows"){
        for(int i=0;i<4;++i){m_targets[i]=kind=="closeWindows"?0:15;m_moving[i]=std::abs(m_windows[i]-m_targets[i])>0.001;}
    }else if(kind=="setDoor"){
        const bool open=a["open"].toBool();if(open!=m_doors[index]){m_doors[index]=open;m_doorRemaining[index]=0.65;if(open)m_locked=false;}
    }else if(kind=="closeDoors"){
        for(int i=0;i<4;++i)if(m_doors[i]){m_doors[i]=false;m_doorRemaining[i]=0.65;}
    }else if(kind=="setLocked")m_locked=value;
    else if(kind=="setPort")m_port=value;
    else if(kind=="setMirrors")m_mirrors=value;
    else if(kind=="setHood"){if(value!=m_hood){m_hood=value;m_hoodRemaining=0.8;}}
    else if(kind=="setHatch"){if(value!=m_hatch){m_hatch=value;m_hatchRemaining=0.9;}}
    else if(kind=="setGear"||kind=="startDriving"||kind=="park"){
        m_gear=kind=="startDriving"?"D":kind=="park"?"P":a["value"].toString();
        if(m_gear=="D"||m_gear=="R"){m_locked=true;m_mirrors=false;}
        if(m_gear=="P"||m_gear=="N")m_speed=0;
        if(m_gear=="R")m_speed=std::min(m_speed,15);
        if(kind=="startDriving")m_speed=40;
    }else if(kind=="setSpeed")m_speed=a["value"].toInt();
    else if(kind=="setPlugged")m_plugged=value;
    else if(kind=="setChargeTarget"){m_chargeTarget=a["value"].toInt();if(m_soc>=m_chargeTarget)m_charging=false;}
    else if(kind=="startCharging")m_charging=true;
    else if(kind=="stopCharging")m_charging=false;
    else if(kind=="advance")energy(a["seconds"].toDouble());
    emit changed();return {};
}
QString SimVehicle::inject(const QString &action,const QJsonObject &a) {
    if(action=="soc"){
        if(!VehicleProtocol::number(a["value"],0,100))return QStringLiteral("SOC 应为 0–100%");m_soc=a["value"].toDouble();if(m_soc<=0)m_speed=0;if(m_soc>=m_chargeTarget)m_charging=false;
    }else if(action=="window"){
        if(!VehicleProtocol::number(a["index"],0,3,true)||!VehicleProtocol::number(a["value"],0,100,true))return QStringLiteral("窗位无效");const int i=a["index"].toInt();m_windows[i]=m_targets[i]=a["value"].toInt();m_moving[i]=false;
    }else if(action=="reset"){reset();return {};}
    else return execute(action,a);
    emit changed();return {};
}
bool SimVehicle::completed(const QString &kind,const QJsonObject &a) const {
    if(kind=="setWindow"){const int i=a["index"].toInt();return !m_moving[i]&&std::abs(m_windows[i]-a["target"].toInt())<0.01;}
    if(kind=="closeWindows"||kind=="ventilateWindows"){for(int i=0;i<4;++i)if(m_moving[i]||m_targets[i]!=(kind=="closeWindows"?0:15))return false;}
    if(kind=="setDoor")return m_doorRemaining[a["index"].toInt()]<=0;
    if(kind=="closeDoors")for(auto remaining:m_doorRemaining)if(remaining>0)return false;
    if(kind=="setHood")return m_hoodRemaining<=0;if(kind=="setHatch")return m_hatchRemaining<=0;
    return true;
}
void SimVehicle::energy(double seconds) {
    if(m_speed>0){const double km=std::min(m_speed*seconds/3600,m_soc/0.25);m_trip+=km;if(m_gear=="D")m_forward+=km;m_soc=std::max(0.0,m_soc-km*0.25);if(m_soc<=0)m_speed=0;}
    if(m_charging){m_soc=std::min(double(m_chargeTarget),m_soc+55.0/60/3600*100*seconds);if(m_soc>=m_chargeTarget)m_charging=false;}
}
void SimVehicle::advance(double seconds){if(std::isfinite(seconds)&&seconds>0&&seconds<=3600){energy(seconds);emit changed();}}
void SimVehicle::tick() {
    const double dt=std::min(0.1,m_clock.restart()/1000.0);bool active=m_speed>0||m_charging;
    energy(dt);
    for(int i=0;i<4;++i){
        if(m_moving[i]){active=true;const double step=dt*100/1.8;if(std::abs(m_targets[i]-m_windows[i])<=step){m_windows[i]=m_targets[i];m_moving[i]=false;}else m_windows[i]+=m_targets[i]>m_windows[i]?step:-step;}
        if(m_doorRemaining[i]>0){active=true;m_doorRemaining[i]=std::max(0.0,m_doorRemaining[i]-dt);}
    }
    if(m_hoodRemaining>0||m_hatchRemaining>0)active=true;m_hoodRemaining=std::max(0.0,m_hoodRemaining-dt);m_hatchRemaining=std::max(0.0,m_hatchRemaining-dt);
    if(active)emit changed();
}
void SimVehicle::reset(){m_windows.fill(0);m_targets.fill(0);m_moving.fill(false);m_doors.fill(false);m_doorRemaining.fill(0);m_hoodRemaining=m_hatchRemaining=0;m_gear="P";m_speed=0;m_chargeTarget=90;m_soc=82;m_trip=m_forward=0;m_locked=true;m_hood=m_hatch=m_port=m_plugged=m_charging=m_mirrors=false;emit changed();}
