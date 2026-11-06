#pragma once
class QApplication;class SimulatorWindow;class SimServer;
class QQuickWindow;
void runSimulatorSmoke(QApplication &,SimulatorWindow &,SimServer &);
void runDriverSmoke(QApplication &,QQuickWindow &,SimServer &);
