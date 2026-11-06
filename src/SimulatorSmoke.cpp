#include "SimulatorSmoke.h"
#include "SimulatorWindow.h"
#include "DriverConsole.h"
#include <QtWidgets>
#include <QtTest>
#include <QStandardItemModel>
#include <QQuickItem>
#include <QQuickWindow>
#include <QJsonArray>
#include <functional>
#include <stdexcept>

void runDriverSmoke(QApplication &app, QQuickWindow &window, SimServer &server) {
    QTimer::singleShot(200, &app, [&] {
        auto require = [](bool ok, const char *message) {
            if (!ok)
                throw std::runtime_error(message);
        };
        std::function<QQuickItem *(QQuickItem *,const QString &)> find;
        find = [&](QQuickItem *parent,const QString &name) -> QQuickItem * {
            if (parent->objectName()==name && parent->isVisible())
                return parent;
            for (auto *child : parent->childItems())
                if (auto *item=find(child,name))
                    return item;
            return nullptr;
        };
        auto position = [&](const char *name) {
            auto *item = find(window.contentItem(),QString::fromLatin1(name));
            require(item && item->isVisible() && item->isEnabled(), name);
            return item->mapToScene({item->width()/2, item->height()/2}).toPoint();
        };
        auto click = [&](const char *name) {
            QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, position(name));
            QTest::qWait(65);
        };
        auto state = [&] { return server.model()->snapshot(); };
          const QString output = qEnvironmentVariable("SIMULATOR_SMOKE_DIR", "artifacts/driver");
          auto capture = [&](const QString &name) {
              QTest::qWait(120);
              const auto frame = window.grabWindow();
              require(!frame.isNull() && frame.save(output + "/" + name + ".png"), "driver screen capture");
          };
        try {
            require(!window.icon().isNull(), "simulator window has the branded icon");
              QDir().mkpath(output);
              capture("cockpit");
              click("driverThemeToggle");
              capture("cockpit-night");
              click("driverThemeToggle");
            click("driverFullscreen");
            QTest::qWait(300);
            require(window.visibility() == QWindow::FullScreen, "enter full screen");
            capture("cockpit-fullscreen");
            QTest::keyClick(&window, Qt::Key_Escape);
            QTest::qWait(300);
            require(window.visibility() != QWindow::FullScreen, "Escape leaves full screen");
            QTest::keyClick(&window, Qt::Key_F1);
            QTest::qWait(170);
            click("driverKeyWS");
            click("driverBindthrottle");
            QTest::keyClick(&window, Qt::Key_S);
            auto *feedback = find(window.contentItem(), "driverKeyFeedback");
            require(feedback && feedback->property("text").toString().contains(QStringLiteral("已被占用")),
                    "duplicate key is rejected");
            QTest::keyClick(&window, Qt::Key_T);
            {
                DriverConsole reloaded(server);
                const auto keys = reloaded.keyLabels();
                require(keys.value("throttle").toString() == "T" && keys.value("brake").toString() == "S",
                        "custom key and preset are persisted");
            }
            capture("key-settings");
            click("driverCloseKeys");
            click("driverGearD");
            require(state()["gear"] == "P", "brake interlock");
            QTest::keyPress(&window, Qt::Key_S);
            QTest::keyClick(&window, Qt::Key_4);
            QTest::keyRelease(&window, Qt::Key_S);
            require(state()["gear"] == "D", "held brake and keyboard shift enable D");
            QTest::keyPress(&window, Qt::Key_T);
            QTest::qWait(300);
            require(server.model()->throttlePressed() && state()["speed"].toInt() > 0, "custom throttle accelerates");
            require(find(window.contentItem(), "driverRoad")->property("distanceKm").toDouble() > 0,
                    "vehicle distance advances the road");
            QEvent deactivate(QEvent::WindowDeactivate);
            QCoreApplication::sendEvent(&window, &deactivate);
            require(!server.model()->throttlePressed(), "focus loss releases throttle");
            QTest::keyRelease(&window, Qt::Key_T);
            const auto throttle = position("driverThrottle");
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, throttle);
            QTest::qWait(850);
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, throttle);
            require(state()["speed"].toInt() >= 10, "hold throttle accelerates");
            QTest::keyPress(&window, Qt::Key_S);
            QTest::qWait(900);
            QTest::keyRelease(&window, Qt::Key_S);
            require(state()["speed"].toInt() == 0, "brake stops vehicle");
            auto *road = find(window.contentItem(), "driverRoad");
            require(road, "road view");
            auto roadFrame = [&] {
                const auto frame = window.grabWindow();
                const double scale = double(frame.width()) / window.width();
                const auto area = road->mapRectToScene({road->width() * 0.2, road->height() * 0.32,
                                                       road->width() * 0.6, road->height() * 0.5});
                return frame.copy(QRect(qRound(area.x() * scale), qRound(area.y() * scale),
                                        qRound(area.width() * scale), qRound(area.height() * scale)))
                    .convertToFormat(QImage::Format_RGB32);
            };
            auto difference = [&](const QImage &before, const QImage &after) {
                require(!before.isNull() && before.size() == after.size(), "road frames have valid geometry");
                double sum = 0;
                int samples = 0;
                for (int y = 0; y < before.height(); y += 3) {
                    for (int x = 0; x < before.width(); x += 3) {
                        const auto a = before.pixel(x, y), b = after.pixel(x, y);
                        sum += std::abs(qRed(a) - qRed(b)) + std::abs(qGreen(a) - qGreen(b))
                             + std::abs(qBlue(a) - qBlue(b));
                        ++samples;
                    }
                }
                return sum / (samples * 3);
            };
            const auto stoppedFrame = roadFrame();
            QTest::qWait(350);
            require(difference(stoppedFrame, roadFrame()) < 0.01, "road stays still at zero speed");
            double slowTravel = 0, fastTravel = 0;
            for (int speed : {10, 40, 80, 120}) {
                require(server.control("setSpeed", {{"value", speed}}).isEmpty(), "set road test speed");
                QTest::qWait(120);
                const auto first = roadFrame();
                first.save(output + QString("/road-%1-a.png").arg(speed));
                const double start = road->property("distanceKm").toDouble();
                QTest::qWait(450);
                const auto second = roadFrame();
                second.save(output + QString("/road-%1-b.png").arg(speed));
                const double travel = (road->property("distanceKm").toDouble() - start) * 1000;
                const double pixels = difference(first, second);
                qInfo() << "Road speed" << speed << "km/h: travel" << travel << "m, pixel difference" << pixels;
                require(pixels > 0.02 && travel > 0, "road visibly moves while driving");
                if (speed == 10) slowTravel = travel;
                if (speed == 80) fastTravel = travel;
            }
            require(fastTravel > slowTravel * 6 && fastTravel < slowTravel * 10,
                    "road displacement follows vehicle speed");
            server.control("setSpeed", {{"value", 0}});
            server.control("setGear", {{"value", "R"}});
            server.control("setSpeed", {{"value", 10}});
            const double reverseStart = road->property("distanceKm").toDouble();
            QTest::qWait(350);
            require(road->property("distanceKm").toDouble() < reverseStart, "reverse moves the road backwards");
            server.control("setSpeed", {{"value", 0}});
            click("driverGearP");
            require(state()["gear"] == "P", "park after stopping");
            QTest::keyPress(&window, Qt::Key_X);
            QTest::qWait(220);
            click("driverBodyDrawer");
            QTest::qWait(280);
            require(!state()["windowMoving"].toArray().at(0).toBool(), "opening drawer releases keyboard window hold");
            QTest::keyRelease(&window, Qt::Key_X);
            const auto rocker = position("windowDown0");
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, rocker);
            QTest::qWait(290);
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, rocker);
            const auto opening = state()["windows"].toArray().at(0).toDouble();
            require(opening > 5 && opening < 100, "rocker moves the window");
            QTest::qWait(150);
            require(state()["windows"].toArray().at(0).toDouble() == opening, "rocker release stops the window");
            capture("body-drawer");
            click("driverCloseDrawer");
            QTest::qWait(280);
            require(server.control("setGear", {{"value", "D"}}).isEmpty(), "drive gear for quick controls");
            click("driverBodyDrawer");
            QTest::qWait(180);
            for (const auto *name : {"quickLockAction", "quickMirrorAction"}) {
                const auto *action = find(window.contentItem(), name);
                require(action && !action->isEnabled(), "park-only actions disabled in drive");
            }
            capture("body-drawer-drive");
            QTest::keyClick(&window, Qt::Key_Escape);
            QTest::qWait(180);
            require(!find(window.contentItem(), "driverCloseDrawer"), "Escape closes quick controls");
            require(server.control("setGear", {{"value", "P"}}).isEmpty(), "restore park gear");
            click("driverTab1");
            capture("body");
            click("driverTab2");
            click("driverChargePort");
            click("driverChargePlug");
            click("driverChargeStart");
            require(state()["charging"].toBool(), "charging starts");
              capture("charging");
            click("driverChargeAdvance");
            click("driverChargeAdvance");
            require(!state()["charging"].toBool() && state()["soc"].toDouble() == 90, "charge target reached");
              click("driverChargeTarget");
              require(state()["chargeTarget"].toInt() == 75, "target slider commits on release");
            auto *target = find(window.contentItem(), "driverChargeTarget");
            auto *track = qvariant_cast<QQuickItem *>(target->property("background"));
            require(track && qAbs(track->height() - 4) < 0.1, "charging slider keeps a thin track");
            click("driverTab3");
              click("driverPauseTelemetry");
              require(server.telemetryPaused(), "driver diagnostics pauses telemetry");
              capture("diagnostics");
              click("driverPauseTelemetry");
              require(!server.telemetryPaused(), "driver diagnostics resumes telemetry");
              window.resize(740, 640);
              QTest::qWait(250);
            click("driverTab0");
              capture("cockpit-compact");
              const auto pedal = position("driverThrottle");
              if (!QRect(QPoint(), window.size()).contains(pedal))
                  qWarning() << "compact pedal position:" << pedal << "window:" << window.size();
              require(QRect(QPoint(), window.size()).contains(pedal),
                      "pedal controls remain reachable in a compact window");
            click("driverBodyDrawer");
            QTest::qWait(180);
            const QRect compactWindow(QPoint(), window.size());
            require(compactWindow.contains(position("windowDown3"))
                    && compactWindow.contains(position("quickMirrorAction")),
                    "quick controls remain reachable in a compact window");
            capture("body-drawer-compact");
            click("driverCloseDrawer");
            QTest::qWait(180);
            click("driverTab1");
            capture("body-compact");
            click("driverTab2");
            capture("charging-compact-top");
            auto *scroll = window.findChild<QObject *>("driverPageScroll");
            require(scroll, "page scroll container");
            auto *flickable = qvariant_cast<QQuickItem *>(scroll->property("contentItem"));
            require(flickable, "page flickable");
            flickable->setProperty("contentY", std::max(0.0, flickable->property("contentHeight").toDouble() - flickable->height()));
            QTest::qWait(120);
            auto *startControl = find(window.contentItem(), "driverChargeStart");
            require(startControl, "compact charging button");
            const auto startButton = startControl->mapToScene({startControl->width() / 2, startControl->height() / 2}).toPoint();
            require(QRect(QPoint(), window.size()).contains(startButton), "compact charging actions are reachable by scrolling");
            capture("charging-compact-controls");
            qInfo() << "PASS driver cockpit: fullscreen, key remapping, focus loss, window drawer, charging and diagnostics";
            app.exit(0);
        } catch (const std::exception &error) {
            qCritical() << "Driver cockpit test failed:" << error.what();
            app.exit(2);
        }
    });
}

void runSimulatorSmoke(QApplication &app,SimulatorWindow &window,SimServer &server) {
    QTimer::singleShot(250,&app,[&]{
        const QString output=qEnvironmentVariable("SIMULATOR_SMOKE_DIR","artifacts/charging/ui");QDir().mkpath(output);
        auto require=[](bool ok,const char *text){if(!ok)throw std::runtime_error(text);};
        auto *scroll=window.findChild<QScrollArea*>("consoleScroll");
        auto get=[&](const char *name){auto *w=window.findChild<QWidget*>(name);require(w,name);return w;};
        auto click=[&](const char *name){auto *w=get(name);require(w->isEnabled(),name);scroll->ensureWidgetVisible(w);QTest::qWait(30);QTest::mouseClick(w,Qt::LeftButton,Qt::NoModifier,QPoint(9,w->height()/2));QTest::qWait(120);};
        auto edit=[&](const char *name,const char *text){auto *box=qobject_cast<QAbstractSpinBox*>(get(name));scroll->ensureWidgetVisible(box);box->setFocus();box->selectAll();QTest::keyClicks(box,text);QTest::keyClick(box,Qt::Key_Return);QTest::qWait(130);};
        auto capture=[&](const QString &name){QTest::qWait(120);require(window.grab().save(output+"/"+name+".png"),"save UI capture");};
        auto textFits=[&]{for(auto *b:window.findChildren<QPushButton*>()){
            QStyleOptionButton option;option.initFrom(b);option.text=b->text();
            const auto needed=b->style()->sizeFromContents(QStyle::CT_PushButton,&option,QSize(b->fontMetrics().horizontalAdvance(b->text()),b->fontMetrics().height()),b);
            require(b->height()>=needed.height()&&b->width()>=needed.width(),qPrintable("button text clipped: "+b->text()));
        }};
        try {
            require(scroll,"scroll area");require(!get("simChargeStart")->isEnabled()&&!get("simChargeStop")->isEnabled(),"initial charging buttons");
            require(!get("speedInput")->isEnabled()&&!get("simPark")->isEnabled()&&!get("simCloseWindows")->isEnabled(),"initial redundant actions disabled");
            edit("socInput","63.5");require(server.model()->snapshot()["soc"].toDouble()==63.5,"SOC edit");server.control("soc",{{"value",41}});get("socInput")->clearFocus();QTest::qWait(150);require(server.model()->snapshot()["soc"].toDouble()==41,"unmodified focus loss overwrites remote SOC");
            edit("windowInput0","35");require(server.model()->snapshot()["windows"].toArray()[0].toDouble()==35,"window actual injection");click("simCloseWindows");QTest::qWait(800);require(server.model()->snapshot()["windows"].toArray()[0].toDouble()==0,"close all windows");
            click("doorInput0");require(!get("simDrive")->isEnabled(),"open door disables driving");QTest::qWait(700);click("doorInput0");QTest::qWait(700);click("simDrive");require(server.model()->snapshot()["speed"].toInt()==40&&get("speedInput")->isEnabled(),"drive action");click("simPark");
            click("bodyPart1");require(!get("simDrive")->isEnabled(),"open hood disables driving");QTest::qWait(800);click("bodyPart1");QTest::qWait(800);require(get("simDrive")->isEnabled(),"closed hood restores driving");click("bodyPart3");require(server.model()->snapshot()["mirrors"].toBool(),"mirror control");click("bodyPart3");
            click("simPort");require(get("simPlug")->isEnabled()&&!get("simChargeStart")->isEnabled(),"port-open charge conditions");click("simPlug");require(get("simChargeStart")->isEnabled(),"plug enables start");click("simChargeStart");
            require(!get("simChargeStart")->isEnabled()&&get("simChargeStop")->isEnabled()&&!get("simPlug")->isEnabled()&&!get("simPort")->isEnabled(),"charging action interlocks");
            auto *gear=qobject_cast<QComboBox*>(get("gearInput"));auto *model=qobject_cast<QStandardItemModel*>(gear->model());require(!model->item(2)->isEnabled()&&!model->item(3)->isEnabled(),"charging prevents N and D");textFits();capture("01-charging");
            click("simChargeStop");require(get("simChargeStart")->isEnabled()&&!get("simChargeStop")->isEnabled()&&get("simPlug")->isEnabled(),"paused charging action states");click("simChargeStart");server.model()->advance(3600);QTest::qWait(150);require(!get("simChargeStart")->isEnabled()&&!get("simChargeStop")->isEnabled()&&get("simPlug")->isEnabled(),"target reached button states");capture("02-target-reached");
            edit("simChargeTarget","95");require(get("simChargeStart")->isEnabled(),"raised target enables resume");edit("simChargeTarget","50");require(!get("simChargeStart")->isEnabled()&&server.model()->snapshot()["soc"].toDouble()>=90,"lower target does not reduce actual SOC");
            server.control("faults",{{"pauseTelemetry",true},{"reject",true},{"delayMs",1000}});QTest::qWait(150);click("simClearFaults");require(!server.telemetryPaused()&&!server.rejecting()&&server.delayMs()==0&&!get("simClearFaults")->isEnabled(),"clear faults");
            click("toggleServer");require(!server.running()&&!get("simDisconnect")->isEnabled(),"stop service");click("toggleServer");require(server.running(),"restart service");click("simReset");scroll->verticalScrollBar()->setValue(0);textFits();capture("03-normal");window.grab().save("artifacts/simulator-console.png");
            window.resize(820,620);QTest::qWait(250);textFits();require(scroll->horizontalScrollBar()->maximum()==0,"compact layout creates horizontal clipping");capture("04-compact-top");scroll->ensureWidgetVisible(get("simChargeStart"));capture("05-compact-charge");
            qInfo()<<"PASS simulator UI: text fit, compact scroll, charge states, interlocks, target, focus protection and faults";app.exit(0);
        }catch(const std::exception &error){window.grab().save(output+"/failure.png");qCritical()<<"SIM UI FAIL:"<<error.what();app.exit(2);}
    });
}
