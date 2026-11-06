#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVariantMap>
#include <array>

class SimServer;
class QWindow;

class DriverConsole : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ running NOTIFY connectionChanged)
    Q_PROPERTY(int cockpitCount READ cockpitCount NOTIFY connectionChanged)
    Q_PROPERTY(bool telemetryPaused READ telemetryPaused NOTIFY connectionChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool warning READ warning NOTIFY messageChanged)
    Q_PROPERTY(QStringList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(QVariantList eventRows READ eventRows NOTIFY eventsChanged)
    Q_PROPERTY(QVariantMap faults READ faults NOTIFY connectionChanged)
    Q_PROPERTY(QString serviceName READ serviceName CONSTANT)
    Q_PROPERTY(QVariantMap keyLabels READ keyLabels NOTIFY keysChanged)
    Q_PROPERTY(QString captureAction READ captureAction NOTIFY captureChanged)
    Q_PROPERTY(QString keyFeedback READ keyFeedback NOTIFY captureChanged)
public:
    explicit DriverConsole(SimServer &server);
    void bindWindow(QWindow &window);
    QVariantMap state() const;
    bool running() const;
    int cockpitCount() const;
    bool telemetryPaused() const;
    QString message() const { return m_message; }
    bool warning() const { return m_warning; }
    QStringList events() const { return m_events; }
    QVariantList eventRows() const;
    QVariantMap faults() const;
    QString serviceName() const;
    QVariantMap keyLabels() const;
    QString captureAction() const { return m_captureAction; }
    QString keyFeedback() const { return m_keyFeedback; }

    Q_INVOKABLE void shift(const QString &gear);
    Q_INVOKABLE void pedal(bool brake, bool keyboard, bool pressed);
    Q_INVOKABLE quint32 beginWindow(int index, bool opening);
    Q_INVOKABLE void endWindow(int index, quint32 hold);
    Q_INVOKABLE void releaseInputs();
    Q_INVOKABLE void command(const QString &action, const QVariantMap &args = {});
    Q_INVOKABLE void openConsole();
    Q_INVOKABLE void captureKey(const QString &action);
    Q_INVOKABLE void applyKeyPreset(bool wasd);

signals:
    void stateChanged();
    void connectionChanged();
    void messageChanged();
    void eventsChanged();
    void consoleRequested();
    void keysChanged();
    void captureChanged();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void saveKeys() const;
    void report(const QString &message, bool warning = false);
    SimServer &m_server;
    QString m_message;
    QStringList m_events;
    std::array<quint32,4> m_holds{};
    QHash<QString, int> m_keys;
    QHash<int, QString> m_heldKeys;
    QHash<int, quint32> m_keyWindowHolds;
    QString m_captureAction;
    QString m_keyFeedback;
    int m_throttle=0,m_brake=0;
    bool m_warning=false;
};
