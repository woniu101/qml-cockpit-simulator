#include "SimulatorWindow.h"
#include "SimulatorSmoke.h"
#include <QtWidgets>
#include <QtTest>
#include <QCommandLineParser>
#include <memory>

int main(int argc,char *argv[]){
    bool headless=false;for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--headless")headless=true;
    std::unique_ptr<QCoreApplication> app;if(headless)app=std::make_unique<QCoreApplication>(argc,argv);else app=std::make_unique<QApplication>(argc,argv);
    app->setApplicationName("QmlCockpitSimulator");app->setOrganizationName("QmlCockpit");
    QCommandLineParser parser;parser.addHelpOption();parser.addOption({"server","Local server name","name","qml-cockpit.vehicle.v1"});parser.addOption({"headless","Run without UI"});parser.addOption({"ui-smoke","Test real console controls"});parser.process(*app);
    SimServer server;if(headless)QObject::connect(&server,&SimServer::logLine,app.get(),[](const QString &line){qInfo().noquote()<<line;});const bool started=server.start(parser.value("server"));if(headless){if(!started)return 1;return app->exec();}
    SimulatorWindow window(&server,parser.value("server"));window.show();
    if(parser.isSet("ui-smoke"))runSimulatorSmoke(*static_cast<QApplication*>(app.get()),window,server);
    return app->exec();
}
