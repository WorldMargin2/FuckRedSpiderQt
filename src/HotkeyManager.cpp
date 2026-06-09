#include "HotkeyManager.h"
#include "Log.h"
#include <QWidget>
#include <QShortcut>
#include <QKeyEvent>

// 默认热键配置
static const QMap<QString, QString> DEFAULT_HOTKEYS = {
    {QStringLiteral("置顶"), QStringLiteral("Ctrl+U")},
    {QStringLiteral("自动关闭目标窗体"), QStringLiteral("Ctrl+I")},
    {QStringLiteral("自动隐藏目标窗体"), QStringLiteral("Ctrl+O")},
    {QStringLiteral("劫持目标窗体"), QStringLiteral("Ctrl+P")},
    {QStringLiteral("捕获当前窗体"), QStringLiteral("Ctrl+[")}
};

HotkeyManager::HotkeyManager(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , m_recordingButton(nullptr)
    , m_parentWidget(parentWidget)
    , m_logger(nullptr)
{
}

HotkeyManager::~HotkeyManager()
{
    clearHotkeys();
}

void HotkeyManager::setLogger(Log *logger)
{
    m_logger = logger;
}

void HotkeyManager::setParentWidget(QWidget *widget)
{
    m_parentWidget = widget;
}

void HotkeyManager::setupDefaultHotkeys()
{
    m_defaultHotkeys = DEFAULT_HOTKEYS;
    for (auto it = m_defaultHotkeys.constBegin(); it != m_defaultHotkeys.constEnd(); ++it) {
        createOrReplaceShortcut(it.key(), stringToKeySequence(it.value()));
    }
}

QShortcut* HotkeyManager::createOrReplaceShortcut(const QString &actionName, const QKeySequence &seq)
{
    // 如果已存在同动作的快捷键，先删除
    if (m_hotkeys.contains(actionName)) {
        delete m_hotkeys.take(actionName);
    }

    QShortcut *shortcut = new QShortcut(seq, m_parentWidget);
    connect(shortcut, &QShortcut::activated, this, [this, actionName]() {
        emit hotkeyTriggered(actionName);
    });

    m_hotkeys[actionName] = shortcut;

    if (m_logger) {
        m_logger->info(QString("注册热键: %1 -> %2").arg(actionName, keySequenceToString(seq)));
    }

    return shortcut;
}

void HotkeyManager::clearHotkeys()
{
    for (QShortcut *shortcut : m_hotkeys) {
        delete shortcut;
    }
    m_hotkeys.clear();
}

void HotkeyManager::updateBinding(const QString &actionName, const QKeySequence &seq)
{
    createOrReplaceShortcut(actionName, seq);
}

void HotkeyManager::resetAllHotkeys()
{
    clearHotkeys();
    setupDefaultHotkeys();
}

void HotkeyManager::startRecording(QPushButton *button)
{
    m_recordingButton = button;
    emit recordingStarted(button);
}

void HotkeyManager::finishRecording()
{
    if (m_recordingButton) {
        emit recordingFinished();
    }
    m_recordingButton = nullptr;
}

bool HotkeyManager::isRecording() const
{
    return m_recordingButton != nullptr;
}

QString HotkeyManager::getButtonText(const QString &actionName) const
{
    if (m_hotkeys.contains(actionName)) {
        QShortcut *shortcut = m_hotkeys[actionName];
        return keySequenceToString(shortcut->key());
    }
    if (m_defaultHotkeys.contains(actionName)) {
        return m_defaultHotkeys[actionName];
    }
    return QString();
}

QKeySequence HotkeyManager::stringToKeySequence(const QString &str)
{
    return QKeySequence(str, QKeySequence::PortableText);
}

QString HotkeyManager::keySequenceToString(const QKeySequence &seq)
{
    return seq.toString(QKeySequence::PortableText);
}
