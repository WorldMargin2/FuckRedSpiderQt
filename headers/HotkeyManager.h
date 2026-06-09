#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QMap>
#include <QShortcut>
#include <QKeySequence>
#include <QString>
#include <QPushButton>

class QWidget;
class Log;

class HotkeyManager : public QObject
{
    Q_OBJECT

public:
    explicit HotkeyManager(QWidget *parentWidget, QObject *parent = nullptr);
    ~HotkeyManager();

    void setLogger(Log *logger);
    void setParentWidget(QWidget *widget);

    // 注册默认热键
    void setupDefaultHotkeys();

    // 清除所有热键
    void clearHotkeys();

    // 更新某个动作的热键绑定
    void updateBinding(const QString &actionName, const QKeySequence &seq);

    // 重置所有热键为默认值
    void resetAllHotkeys();

    // 热键录制
    void startRecording(QPushButton *button);
    void finishRecording();
    bool isRecording() const;

    // 获取按钮对应的热键文本
    QString getButtonText(const QString &actionName) const;

    // 工具方法
    static QKeySequence stringToKeySequence(const QString &str);
    static QString keySequenceToString(const QKeySequence &seq);

signals:
    void hotkeyTriggered(const QString &actionName);
    void recordingStarted(QPushButton *button);
    void recordingFinished();

private:
    QMap<QString, QShortcut*> m_hotkeys;
    QMap<QString, QString> m_defaultHotkeys;
    QMap<QPushButton*, QString> m_hotkeyButtonMap;
    QPushButton *m_recordingButton;
    QWidget *m_parentWidget;
    Log *m_logger;

    QShortcut* createOrReplaceShortcut(const QString &actionName, const QKeySequence &seq);
};

#endif // HOTKEYMANAGER_H
