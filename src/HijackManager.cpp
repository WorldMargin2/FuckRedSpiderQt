#include "HijackManager.h"
#include "Log.h"
#include "WindowsApi.h"
#include <QWidget>

HijackManager::HijackManager(QObject *parent)
    : QObject(parent)
    , m_targetWindow(nullptr)
    , m_logger(nullptr)
{
}

void HijackManager::setLogger(Log *logger)
{
    m_logger = logger;
}

void HijackManager::setFullWindowClassName(const QString &name)
{
    m_fullWindowClassName = name;
}

void HijackManager::setNormalWindowClassName(const QString &name)
{
    m_normalWindowClassName = name;
}

bool HijackManager::attachToHost(QWidget *hostWidget)
{
    if (!hostWidget) return false;

    // 已附加且父窗口非空，直接返回
    if (m_targetWindow != nullptr &&
        WindowsApi::getParent(m_targetWindow) != nullptr) {
        return true;
    }

    HWND hostHwnd = (HWND)hostWidget->winId();

    // ---- 尝试全屏窗口 ----
    HWND fullWnd = WindowsApi::findWindow(m_fullWindowClassName);
    if (fullWnd != nullptr) {
        if (tryAttachWindow(fullWnd, hostHwnd, hostWidget, QStringLiteral("全屏"))) {
            return true;
        }
    }

    // ---- 尝试普通窗口 ----
    HWND normalWnd = WindowsApi::findWindow(m_normalWindowClassName);
    if (normalWnd != nullptr) {
        if (tryAttachWindow(normalWnd, hostHwnd, hostWidget, QStringLiteral("普通"))) {
            return true;
        }
    }

    return false;
}

bool HijackManager::tryAttachWindow(HWND targetWnd, HWND hostHwnd, QWidget *hostWidget, const QString &windowTypeDesc)
{
    DWORD pid;
    WindowsApi::getWindowThreadProcessId(targetWnd, &pid);

    if (!WindowsApi::isSameProcess(targetWnd, pid)) {
        return false;
    }

    // 保存原始信息（仅第一次）
    if (!m_hijacked.contains(targetWnd)) {
        HijackInfo info;
        info.originalParent = WindowsApi::getParent(targetWnd);
        info.originalStyle = WindowsApi::getWindowLong(targetWnd, GWL_STYLE);
        ::GetWindowRect(targetWnd, &info.originalRect);
        m_hijacked[targetWnd] = info;
    }

    hostWidget->show();
    WindowsApi::setChildWindow(targetWnd, hostHwnd);
    WindowsApi::setWindowPos(targetWnd, hostHwnd, 0, 0,
                             hostWidget->width(), hostWidget->height(),
                             SWP_NOZORDER | SWP_NOACTIVATE);

    m_targetWindow = targetWnd;

    if (m_logger) {
        m_logger->info(QString("嵌入%1窗口: 0x%2")
                       .arg(windowTypeDesc)
                       .arg((qulonglong)targetWnd, 0, 16));
    }

    return true;
}

void HijackManager::restoreHijackedWindow(HWND h)
{
    auto it = m_hijacked.find(h);
    if (it == m_hijacked.end()) {
        return;
    }

    const HijackInfo &info = it.value();

    // 先从当前父窗口脱离
    WindowsApi::setParent(h, info.originalParent);

    // 恢复原始样式
    WindowsApi::setWindowLong(h, GWL_STYLE, info.originalStyle);

    // 恢复原始位置和大小
    int width = info.originalRect.right - info.originalRect.left;
    int height = info.originalRect.bottom - info.originalRect.top;
    WindowsApi::setWindowPos(h, nullptr,
                             info.originalRect.left, info.originalRect.top,
                             width, height,
                             SWP_NOZORDER | SWP_NOACTIVATE);

    m_hijacked.remove(h);

    if (m_logger) {
        m_logger->info(QString("已恢复劫持窗口: 0x%1").arg((qulonglong)h, 0, 16));
    }
}

void HijackManager::restoreAllHijackedWindows()
{
    QList<HWND> keys = m_hijacked.keys();
    for (HWND h : keys) {
        restoreHijackedWindow(h);
    }
    m_targetWindow = nullptr;
}

HWND HijackManager::targetWindow() const
{
    return m_targetWindow;
}

void HijackManager::resetTargetWindow()
{
    m_targetWindow = nullptr;
}
