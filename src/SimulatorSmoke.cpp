#include "SimulatorSmoke.h"
#include "SimulatorWindow.h"
#include <QtWidgets>
#include <QtTest>
#include <QStandardItemModel>
#include <stdexcept>

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
