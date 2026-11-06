#include <QtTest>
#include "SimVehicle.h"
#include "SimServer.h"
#include "VehicleProtocol.h"
#include <QUuid>
class SimulatorTests:public QObject {
    Q_OBJECT
private slots:
    void forwardSpeedLimitIs120() {
        SimVehicle vehicle;
        QVERIFY(vehicle.execute("setGear", {{"value", "D"}}).isEmpty());
        QVERIFY(vehicle.execute("setSpeed", {{"value", 120}}).isEmpty());
        QVERIFY(!vehicle.execute("setSpeed", {{"value", 121}}).isEmpty());
        vehicle.setPedals(true, false);
        QTest::qWait(100);
        QCOMPARE(vehicle.snapshot()["speed"].toInt(), 120);
        QVERIFY(VehicleProtocol::validState(vehicle.snapshot()));
        auto invalid = vehicle.snapshot();
        invalid["speed"] = 121;
        QVERIFY(!VehicleProtocol::validState(invalid));
        vehicle.setPedals(false, false);
        QVERIFY(vehicle.execute("setGear", {{"value", "R"}}).isEmpty());
        QCOMPARE(vehicle.snapshot()["speed"].toInt(), 15);
        QVERIFY(!vehicle.execute("setSpeed", {{"value", 16}}).isEmpty());
    }
    void physicalPedalsAndShiftInterlocks() {
        SimServer server;
        auto *vehicle = server.model();
        QVERIFY(!server.shiftFromDriver("D").isEmpty());
        vehicle->setPedals(true, false);
        QTest::qWait(100);
        QCOMPARE(vehicle->snapshot()["speed"].toInt(), 0);
        vehicle->setPedals(true, true);
        QVERIFY(server.shiftFromDriver("D").isEmpty());
        QTest::qWait(120);
        QCOMPARE(vehicle->snapshot()["speed"].toInt(), 0);
        vehicle->setPedals(true, false);
        QTRY_VERIFY_WITH_TIMEOUT(vehicle->snapshot()["speed"].toInt() >= 12, 1800);
        QVERIFY(!server.shiftFromDriver("R").isEmpty());
        vehicle->setPedals(false, false);
        const auto coastingSpeed = vehicle->snapshot()["speed"].toInt();
        QTRY_VERIFY_WITH_TIMEOUT(vehicle->snapshot()["speed"].toInt() < coastingSpeed, 1000);
        vehicle->setPedals(true, true);
        QTRY_COMPARE_WITH_TIMEOUT(vehicle->snapshot()["speed"].toInt(), 0, 1800);
        QTRY_VERIFY_WITH_TIMEOUT(vehicle->physicalShiftError("P").isEmpty(), 500);
        QVERIFY(server.shiftFromDriver("P").isEmpty());
        QVERIFY(vehicle->snapshot()["forwardKm"].toDouble() > 0);
        QVERIFY(server.control("setPort", {{"value",true}}).isEmpty());
        QVERIFY(server.control("setPlugged", {{"value",true}}).isEmpty());
        QVERIFY(!server.shiftFromDriver("D").isEmpty());
        QCOMPARE(vehicle->snapshot()["gear"].toString(), QString("P"));
        QVERIFY(VehicleProtocol::validState(vehicle->snapshot()));
    }

    void staleWindowReleaseKeepsReplacementCommand() {
        SimServer server;
        const auto first = server.beginWindowHold(0, true);
        QVERIFY(first != 0);
        QTRY_VERIFY(server.model()->snapshot()["windows"].toArray()[0].toDouble() > 3);
        server.endWindowHold(0, first);
        const auto stopped = server.model()->snapshot()["windows"].toArray()[0].toDouble();
        QTest::qWait(100);
        QCOMPARE(server.model()->snapshot()["windows"].toArray()[0].toDouble(), stopped);
        const auto second = server.beginWindowHold(0, true);
        QVERIFY(second != 0);
        QVERIFY(server.control("setWindow", {{"index",0},{"target",50}}).isEmpty());
        server.endWindowHold(0, second);
        QTRY_COMPARE_WITH_TIMEOUT(server.model()->snapshot()["windows"].toArray()[0].toDouble(), 50.0, 2000);
    }

    void chargingInterlocksAndTargetChanges(){
        SimVehicle v;QVERIFY(!v.commandError("startCharging").isEmpty());
        QVERIFY(v.execute("setPort",{{"value",true}}).isEmpty());QVERIFY(v.execute("setPlugged",{{"value",true}}).isEmpty());QVERIFY(v.execute("startCharging",{}).isEmpty());
        QVERIFY(!v.execute("setGear",{{"value","N"}}).isEmpty());QCOMPARE(v.snapshot()["gear"].toString(),QString("P"));QVERIFY(v.snapshot()["charging"].toBool());
        QVERIFY(!v.execute("setPlugged",{{"value",false}}).isEmpty());QVERIFY(!v.execute("setPort",{{"value",false}}).isEmpty());
        QVERIFY(v.execute("setPlugged",{{"value",true}}).isEmpty());QVERIFY(v.execute("setPort",{{"value",true}}).isEmpty());
        const auto soc=v.snapshot()["soc"].toDouble();v.execute("setChargeTarget",{{"value",50}});QVERIFY(!v.snapshot()["charging"].toBool());QCOMPARE(v.snapshot()["soc"].toDouble(),soc);QVERIFY(!v.commandError("startCharging").isEmpty());
        v.execute("setChargeTarget",{{"value",100}});QVERIFY(v.commandError("startCharging").isEmpty());v.execute("startCharging",{});v.advance(3600);QCOMPARE(v.snapshot()["soc"].toDouble(),100.0);QVERIFY(!v.snapshot()["charging"].toBool());
    }
    void lowBatteryCannotOvercountDistance(){
        SimVehicle v;v.inject("soc",{{"value",0.1}});v.execute("startDriving",{});v.execute("setSpeed",{{"value",100}});v.advance(3600);
        QVERIFY(std::abs(v.snapshot()["tripKm"].toDouble()-0.4)<0.00001);QCOMPARE(v.snapshot()["soc"].toDouble(),0.0);QCOMPARE(v.snapshot()["speed"].toInt(),0);
    }
    void resetAndNewControlsCancelDelayedCommands(){
        SimServer server;QVERIFY(server.start("qml-cockpit-scheduled-"+QUuid::createUuid().toString(QUuid::WithoutBraces)));
        QLocalSocket socket;QByteArray buffer;QString session;QHash<QString,QString> results;
        connect(&socket,&QLocalSocket::readyRead,&socket,[&]{buffer+=socket.readAll();QList<QJsonObject> messages;QString error;if(!VehicleProtocol::decode(buffer,messages,error))return;for(const auto &m:messages){if(m["type"]=="hello")session=m["session"].toString();if(m["type"]=="result")results[m["id"].toString()]=m["status"].toString();}});
        socket.connectToServer(server.name());QTRY_COMPARE(socket.state(),QLocalSocket::ConnectedState);socket.write(VehicleProtocol::encode({{"type","hello"},{"role","cockpit"}}));QTRY_VERIFY(!session.isEmpty());
        QSignalSpy log(&server,&SimServer::logLine);
        auto request=[&](const QString &id,const QString &kind,const QJsonObject &args){socket.write(VehicleProtocol::encode({{"type","command"},{"session",session},{"id",id},{"kind",kind},{"args",args}}));};
        server.control("faults",{{"delayMs",450}});int count=log.count();request("reset-case","setWindow",{{"index",0},{"target",100}});QTRY_VERIFY(log.count()>count);server.control("reset",{});QTRY_COMPARE(results["reset-case"],QString("cancelled"));QTest::qWait(550);QCOMPARE(server.model()->snapshot()["windows"].toArray()[0].toDouble(),0.0);
        count=log.count();request("stop-case","setWindow",{{"index",0},{"target",100}});QTRY_VERIFY(log.count()>count);server.control("stopWindow",{{"index",0}});QTRY_COMPARE(results["stop-case"],QString("cancelled"));QTest::qWait(550);QCOMPARE(server.model()->snapshot()["windows"].toArray()[0].toDouble(),0.0);
        server.control("setPort",{{"value",true}});server.control("setPlugged",{{"value",true}});count=log.count();request("charge-case","startCharging",{});QTRY_VERIFY(log.count()>count);server.control("stopCharging",{});QTRY_COMPARE(results["charge-case"],QString("cancelled"));QTest::qWait(550);QVERIFY(!server.model()->snapshot()["charging"].toBool());socket.disconnect(this);socket.abort();
    }
    void framing(){
        const auto a=VehicleProtocol::encode({{"type","hello"},{"role","cockpit"}}),b=VehicleProtocol::encode({{"type","hello"},{"role","operator"}});QByteArray buffer=a.left(3);QList<QJsonObject> out;QString error;
        QVERIFY(VehicleProtocol::decode(buffer,out,error));QVERIFY(out.isEmpty());buffer+=a.mid(3)+b;QVERIFY(VehicleProtocol::decode(buffer,out,error));QCOMPARE(out.size(),2);QVERIFY(buffer.isEmpty());
        buffer=QByteArray::fromHex("7fffffff");QVERIFY(!VehicleProtocol::decode(buffer,out,error));
    }
    void windowActualTargetAndStop(){
        SimVehicle v;QVERIFY(v.execute("setWindow",{{"index",0},{"target",100}}).isEmpty());QCOMPARE(v.snapshot()["windows"].toArray()[0].toDouble(),0.0);QTest::qWait(220);const double at=v.snapshot()["windows"].toArray()[0].toDouble();QVERIFY(at>5&&at<100);
        QVERIFY(v.execute("stopWindow",{{"index",0}}).isEmpty());const auto stopped=v.snapshot()["windows"];QTest::qWait(120);QCOMPARE(v.snapshot()["windows"],stopped);
        QVERIFY(!v.execute("setWindow",{{"index",8},{"target",20}}).isEmpty());QVERIFY(!v.execute("setWindow",{{"index",0},{"target",101}}).isEmpty());
    }
    void driveChargeAndSchema(){
        SimVehicle v;QVERIFY(v.execute("setDoor",{{"index",0},{"open",true}}).isEmpty());QVERIFY(!v.execute("startDriving",{}).isEmpty());v.execute("closeDoors",{});QTest::qWait(750);QVERIFY(v.execute("startDriving",{}).isEmpty());v.advance(60);QVERIFY(v.snapshot()["tripKm"].toDouble()>0.6);QVERIFY(v.snapshot()["soc"].toDouble()<82);
        v.execute("setGear",{{"value","R"}});QVERIFY(v.snapshot()["speed"].toInt()<=15);v.execute("park",{});v.execute("setPort",{{"value",true}});v.execute("setPlugged",{{"value",true}});v.execute("startCharging",{});v.advance(3600);QCOMPARE(v.snapshot()["soc"].toDouble(),90.0);QVERIFY(!v.snapshot()["charging"].toBool());QVERIFY(VehicleProtocol::validState(v.snapshot()));
    }
};
QTEST_GUILESS_MAIN(SimulatorTests)
#include "SimulatorTests.moc"
