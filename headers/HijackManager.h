#ifndef HIJACKMANAGER_H
#define HIJACKMANAGER_H

#include <QObject>
#include <QMap>
#include <windows.h>

class QWidget;
class Log;

// 存储被劫持窗口的原始信息，用于恢复
struct HijackInfo {
    HWND originalParent = nullptr;
    LONG originalStyle = 0;
    RECT originalRect = {};
};

class HijackManager : public QObject
{
    Q_OBJECT

public:
    explicit HijackManager(QObject *parent = nullptr);

    void setLogger(Log *logger);
    void setFullWindowClassName(const QString &name);
    void setNormalWindowClassName(const QString &name);

    // 尝试将目标窗口嵌入到 hostWidget 中
    bool attachToHost(QWidget *hostWidget);

    // 恢复单个被劫持的窗口
    void restoreHijackedWindow(HWND h);

    // 恢复所有已被嵌入的窗口
    void restoreAllHijackedWindows();

    // 获取当前目标窗口句柄
    HWND targetWindow() const;
    void resetTargetWindow();

private:
    bool tryAttachWindow(HWND targetWnd, HWND hostHwnd, QWidget *hostWidget, const QString &windowTypeDesc);

    QString m_fullWindowClassName;
    QString m_normalWindowClassName;
    HWND m_targetWindow;
    QMap<HWND, HijackInfo> m_hijacked;
    Log *m_logger;
};

#endif // HIJACKMANAGER_H
