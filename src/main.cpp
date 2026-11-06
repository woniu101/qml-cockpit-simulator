#include "SimulatorWindow.h"
#include "SimulatorSmoke.h"
#include "DriverConsole.h"
#include <QtWidgets>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <memory>

int main(int argc,char *argv[]){
    bool headless=false;for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--headless")headless=true;
    std::unique_ptr<QCoreApplication> app;if(headless)app=std::make_unique<QCoreApplication>(argc,argv);else app=std::make_unique<QApplication>(argc,argv);
    app->setApplicationName("QmlCockpitSimulator");app->setOrganizationName("QmlCockpit");
    if (!headless) {
        auto *gui = static_cast<QApplication *>(app.get());
        gui->setWindowIcon(QIcon(":/branding/app.ico"));
    }
    QCommandLineParser parser;parser.addHelpOption();parser.addOption({"server","Local server name","name","qml-cockpit.vehicle.v1"});parser.addOption({"headless","Run without UI"});parser.addOption({"ui-smoke","Test real console controls"});parser.addOption({"console","Open the diagnostic console directly"});parser.addOption({"driver-smoke","Test driver cockpit controls"});parser.process(*app);
    SimServer server;if(headless)QObject::connect(&server,&SimServer::logLine,app.get(),[](const QString &line){qInfo().noquote()<<line;});const bool started=server.start(parser.value("server"));if(headless){if(!started)return 1;return app->exec();}
    if (parser.isSet("console") || parser.isSet("ui-smoke")) {
        SimulatorWindow window(&server,parser.value("server"));window.show();
        if(parser.isSet("ui-smoke"))runSimulatorSmoke(*static_cast<QApplication*>(app.get()),window,server);
        return app->exec();
    }
    QQuickStyle::setStyle("Basic");
    std::unique_ptr<QTemporaryDir> smokeSettings;
    if (parser.isSet("driver-smoke")) {
        smokeSettings = std::make_unique<QTemporaryDir>();
        if (!smokeSettings->isValid())
            return 1;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, smokeSettings->path());
    }
    std::unique_ptr<SimulatorWindow> console;
    DriverConsole driver(server);
    QQmlApplicationEngine engine;
    QObject::connect(&driver,&DriverConsole::consoleRequested,app.get(),[&] {
        if(!console)
            console=std::make_unique<SimulatorWindow>(&server,parser.value("server"));
        console->show();
        console->raise();
        console->activateWindow();
    });
    engine.rootContext()->setContextProperty("Driver", &driver);
    engine.loadFromModule("Cockpit.Simulator","DriverWindow");
    auto *window=engine.rootObjects().isEmpty()?nullptr:qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if(!window)
        return 1;
    driver.bindWindow(*window);
    if (parser.isSet("driver-smoke"))
        runDriverSmoke(*static_cast<QApplication*>(app.get()), *window, server);
    const int result=app->exec();
    driver.releaseInputs();
    return result;
}
