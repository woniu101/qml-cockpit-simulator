#include "DriverConsole.h"
#include "SimServer.h"
#include <QKeyEvent>
#include <QKeySequence>
#include <QSet>
#include <QSettings>
#include <QWindow>

namespace {
QHash<QString, int> defaultKeys(bool wasd) {
    return {{"brake", wasd ? Qt::Key_S : Qt::Key_Left}, {"throttle", wasd ? Qt::Key_W : Qt::Key_Right},
            {"P", Qt::Key_1}, {"R", Qt::Key_2}, {"N", Qt::Key_3}, {"D", Qt::Key_4},
            {"windowUp", Qt::Key_Z}, {"windowDown", Qt::Key_X}};
}

bool canBind(int key) {
    return (key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9)
        || (key >= Qt::Key_Left && key <= Qt::Key_Down) || key == Qt::Key_Space;
}

QString keyLabel(int key) {
    switch (key) {
    case Qt::Key_Left: return QStringLiteral("←");
    case Qt::Key_Right: return QStringLiteral("→");
    case Qt::Key_Up: return QStringLiteral("↑");
    case Qt::Key_Down: return QStringLiteral("↓");
    case Qt::Key_Space: return QStringLiteral("空格");
    default: return QKeySequence(key).toString(QKeySequence::NativeText);
    }
}
}

DriverConsole::DriverConsole(SimServer &server) : m_server(server), m_keys(defaultKeys(false)) {
    QSettings settings;
    auto saved = m_keys;
    QSet<int> used;
    for (auto it = saved.begin(); it != saved.end(); ++it) {
        const int key = settings.value("driverKeys/" + it.key(), it.value()).toInt();
        if (!canBind(key) || used.contains(key))
            break;
        used.insert(key);
        it.value() = key;
    }
    if (used.size() == saved.size())
        m_keys = saved;
    m_message = server.running() ? QStringLiteral("车辆已就绪") : QStringLiteral("服务未启动，请检查联调控制台");
    m_warning = !server.running();
    connect(server.model(), &SimVehicle::changed, this, [this] {
        if (!m_server.model()->throttlePressed())
            m_throttle = 0;
        if (!m_server.model()->brakePressed())
            m_brake = 0;
        emit stateChanged();
    });
    connect(&server, &SimServer::statusChanged, this, [this] {
        if (!running())
            releaseInputs();
        emit connectionChanged();
    });
    connect(&server, &SimServer::logLine, this, [this](const QString &line) {
        m_events.prepend(line);
        if (m_events.size() > 60)
            m_events.removeLast();
        emit eventsChanged();
    });
}

QVariantMap DriverConsole::state() const {
    auto value = m_server.model()->snapshot().toVariantMap();
    value.insert("throttle", m_server.model()->throttlePressed());
    value.insert("brake", m_server.model()->brakePressed());
    value.insert("driveBlock", m_server.model()->commandError("setGear", {{"value", "D"}}));
    return value;
}

bool DriverConsole::running() const { return m_server.running(); }
int DriverConsole::cockpitCount() const { return m_server.cockpitCount(); }
bool DriverConsole::telemetryPaused() const { return m_server.telemetryPaused(); }
QString DriverConsole::serviceName() const { return m_server.name(); }

QVariantMap DriverConsole::faults() const {
    return {{"reject", m_server.rejecting()}, {"drop", m_server.dropping()}, {"delayMs", m_server.delayMs()}};
}

QVariantList DriverConsole::eventRows() const {
    QVariantList rows;
    for (const auto &line : m_events) {
        const auto detail = line.mid(12).trimmed();
        QString category = QStringLiteral("事件");
        if (detail.contains("rejected") || detail.contains("cancelled"))
            category = QStringLiteral("拒绝/取消");
        else if (detail.contains("completed"))
            category = QStringLiteral("已完成");
        else if (detail.contains("accepted"))
            category = QStringLiteral("已受理");
        else if (detail.contains(QStringLiteral("控制台")))
            category = QStringLiteral("模拟器");
        else if (detail.contains(QStringLiteral("连接")) || detail.contains(QStringLiteral("客户端")))
            category = QStringLiteral("连接");
        rows.append(QVariantMap{{"time", line.left(12)}, {"category", category}, {"detail", detail}});
    }
    return rows;
}

void DriverConsole::bindWindow(QWindow &window) {
    window.installEventFilter(this);
}

QVariantMap DriverConsole::keyLabels() const {
    QVariantMap labels;
    for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it)
        labels.insert(it.key(), keyLabel(it.value()));
    return labels;
}

void DriverConsole::saveKeys() const {
    QSettings settings;
    for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it)
        settings.setValue("driverKeys/" + it.key(), it.value());
}

void DriverConsole::captureKey(const QString &action) {
    releaseInputs();
    m_captureAction = m_keys.contains(action) ? action : QString();
    m_keyFeedback.clear();
    emit captureChanged();
}

void DriverConsole::applyKeyPreset(bool wasd) {
    captureKey({});
    m_keys = defaultKeys(wasd);
    saveKeys();
    emit keysChanged();
}

bool DriverConsole::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate) {
        releaseInputs();
        if (!m_captureAction.isEmpty())
            captureKey({});
    }
    const bool capture = !m_captureAction.isEmpty();
    const bool driving = watched->property("driveKeysEnabled").toBool();
    if (event->type() != QEvent::ShortcutOverride && event->type() != QEvent::KeyPress &&
        event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(watched, event);

    auto *key = static_cast<QKeyEvent *>(event);
    const int code = key->key();
    const bool plain = (key->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier;
    const QString action = plain ? m_keys.key(code) : QString();
    if (event->type() == QEvent::ShortcutOverride) {
        if (capture || (driving && !action.isEmpty())) {
            event->accept();
            return true;
        }
        return false;
    }
    const bool pressed = event->type() == QEvent::KeyPress;
    if (capture) {
        if (!pressed || key->isAutoRepeat())
            return true;
        if (code == Qt::Key_Escape) {
            captureKey({});
        } else if (!plain || !canBind(code)) {
            m_keyFeedback = QStringLiteral("请使用单个字母、数字、方向键或空格");
            emit captureChanged();
        } else if (!action.isEmpty() && action != m_captureAction) {
            m_keyFeedback = QStringLiteral("%1 已被占用，请另选一个按键").arg(keyLabel(code));
            emit captureChanged();
        } else {
            m_keys[m_captureAction] = code;
            saveKeys();
            captureKey({});
            emit keysChanged();
        }
        return true;
    }
    if (!driving)
        return false;
    if (pressed && action.isEmpty())
        return false;
    if (!pressed && !m_heldKeys.contains(code))
        return !action.isEmpty();
    if (key->isAutoRepeat())
        return true;

    if (pressed) {
        if (m_heldKeys.contains(code))
            return true;
        m_heldKeys.insert(code, action);
    }
    const QString input = pressed ? action : m_heldKeys.take(code);
    if (input == "brake" || input == "throttle") {
        pedal(input == "brake", true, pressed);
    } else if (input == "windowUp" || input == "windowDown") {
        if (pressed)
            m_keyWindowHolds[code] = beginWindow(0, input == "windowDown");
        else
            endWindow(0, m_keyWindowHolds.take(code));
    } else if (pressed) {
        shift(input);
    }
    return true;
}

void DriverConsole::shift(const QString &gear) {
    if (!running())
        return;
    const auto error = m_server.shiftFromDriver(gear);
    report(error.isEmpty() ? QStringLiteral("当前 %1 挡").arg(gear) : error, !error.isEmpty());
}

void DriverConsole::pedal(bool brake, bool keyboard, bool pressed) {
    if (pressed && !running())
        return;
    int &sources = brake ? m_brake : m_throttle;
    const int bit = keyboard ? 2 : 1;
    if (pressed)
        sources |= bit;
    else
        sources &= ~bit;
    m_server.model()->setPedals(m_throttle != 0, m_brake != 0);
}

quint32 DriverConsole::beginWindow(int index, bool opening) {
    if (!running() || index < 0 || index >= int(m_holds.size()))
        return 0;
    return m_holds[index] = m_server.beginWindowHold(index, opening);
}

void DriverConsole::endWindow(int index, quint32 hold) {
    if (index < 0 || index >= int(m_holds.size()) || m_holds[index] != hold)
        return;
    m_holds[index] = 0;
    m_server.endWindowHold(index, hold);
}

void DriverConsole::releaseInputs() {
    m_heldKeys.clear();
    m_keyWindowHolds.clear();
    m_throttle = m_brake = 0;
    m_server.model()->setPedals(false, false);
    for (int i = 0; i < int(m_holds.size()); ++i)
        endWindow(i, m_holds[i]);
}

void DriverConsole::command(const QString &action, const QVariantMap &args) {
    if (!running())
        return;
    const auto error = m_server.control(action, QJsonObject::fromVariantMap(args));
    report(error.isEmpty() ? QStringLiteral("操作已受理，实际状态见仪表") : error, !error.isEmpty());
}

void DriverConsole::openConsole() {
    releaseInputs();
    emit consoleRequested();
}

void DriverConsole::report(const QString &message, bool warning) {
    m_message = message;
    m_warning = warning;
    emit messageChanged();
}
