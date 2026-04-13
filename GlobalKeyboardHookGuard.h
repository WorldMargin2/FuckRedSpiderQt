#ifndef GLOBALKEYBOARDHOOKGUARD_H
#define GLOBALKEYBOARDHOOKGUARD_H

#include <QObject>
#include <QKeyEvent>
#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <windows.h>

class GlobalKeyboardHookGuard : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GlobalKeyboardHookGuard(QObject *parent = nullptr);
    ~GlobalKeyboardHookGuard();

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

    // 匹配 C# 版的接口
    void start();
    void stop();
    bool isStarted() const;

signals:
    void keyPressed(int key);
    void guardStarted();
    void guardStopped();
    void errorOccurred(const QString &errorMessage);

private:
    HHOOK keyboardHook;
    bool started;
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static GlobalKeyboardHookGuard *instance;
};

#endif // GLOBALKEYBOARDHOOKGUARD_H