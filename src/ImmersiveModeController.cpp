#include "ImmersiveModeController.h"
#include "Log.h"
#include <QMainWindow>
#include <QLabel>
#include <QMouseEvent>
#include <windows.h>

ImmersiveModeController::ImmersiveModeController(QMainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_labelMoveWindow(nullptr)
    , m_labelResizeWindow(nullptr)
    , m_logger(nullptr)
    , m_active(false)
    , m_isResizing(false)
    , m_isMoving(false)
    , m_keepRatio(false)
    , m_aspectRatio(0.0)
{
}

void ImmersiveModeController::setLogger(Log *logger)
{
    m_logger = logger;
}

void ImmersiveModeController::setMoveLabel(QLabel *label)
{
    m_labelMoveWindow = label;
}

void ImmersiveModeController::setResizeLabel(QLabel *label)
{
    m_labelResizeWindow = label;
}

void ImmersiveModeController::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;

    if (active) {
        if (m_labelMoveWindow) m_labelMoveWindow->show();
        if (m_labelResizeWindow) m_labelResizeWindow->show();
        if (m_mainWindow) {
            // 设置无边框窗口样式
            m_mainWindow->setWindowFlags(m_mainWindow->windowFlags() | Qt::FramelessWindowHint);
            m_mainWindow->show();
        }
        if (m_logger) {
            m_logger->info("沉浸式模式已启用");
        }
    } else {
        if (m_labelMoveWindow) m_labelMoveWindow->hide();
        if (m_labelResizeWindow) m_labelResizeWindow->hide();
        if (m_mainWindow) {
            // 恢复有边框窗口样式
            m_mainWindow->setWindowFlags(m_mainWindow->windowFlags() & ~Qt::FramelessWindowHint);
            m_mainWindow->show();
        }
        if (m_logger) {
            m_logger->info("沉浸式模式已禁用");
        }
    }

    emit modeChanged(active);
}

bool ImmersiveModeController::isActive() const
{
    return m_active;
}

void ImmersiveModeController::setKeepRatio(bool enable, int width, int height)
{
    m_keepRatio = enable;
    if (enable && height > 0) {
        m_aspectRatio = static_cast<double>(width) / static_cast<double>(height);
        // 如果当前窗口已有尺寸，以当前窗口比例为准
        if (m_mainWindow && m_aspectRatio <= 0) {
            m_aspectRatio = static_cast<double>(m_mainWindow->width()) / static_cast<double>(m_mainWindow->height());
        }
    }
}

bool ImmersiveModeController::isKeepRatio() const
{
    return m_keepRatio;
}

void ImmersiveModeController::onMainWindowResized(int width, int height)
{
    if (m_labelResizeWindow) {
        m_labelResizeWindow->setGeometry(width - 27, height - 27, 25, 25);
    }
}

bool ImmersiveModeController::handleMoveMousePress(QMouseEvent *event)
{
    if (!m_active) return false;

    m_isMoving = true;
    m_dragStartPos = event->globalPos();
    m_dragStartBounds = m_mainWindow->geometry();
    return true;
}

bool ImmersiveModeController::handleMoveMouseMove(QMouseEvent *event)
{
    if (!m_isMoving || !m_active) return false;

    QPoint delta = event->globalPos() - m_dragStartPos;
    m_mainWindow->setGeometry(m_dragStartBounds.translated(delta));
    return true;
}

bool ImmersiveModeController::handleMoveMouseRelease(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!m_isMoving) return false;

    m_isMoving = false;
    return true;
}

bool ImmersiveModeController::handleResizeMousePress(QMouseEvent *event)
{
    if (!m_active) return false;

    m_isResizing = true;
    m_dragStartPos = event->globalPos();
    m_dragStartBounds = m_mainWindow->geometry();
    return true;
}

bool ImmersiveModeController::handleResizeMouseMove(QMouseEvent *event)
{
    if (!m_isResizing || !m_active) return false;

    QPoint delta = event->globalPos() - m_dragStartPos;

    int newWidth = qMax(m_dragStartBounds.width() + delta.x(), 200);
    int newHeight = qMax(m_dragStartBounds.height() + delta.y(), 150);

    if (m_keepRatio && m_aspectRatio > 0) {
        // 按较大变化方向计算，保持宽高比
        double ratioW = static_cast<double>(newWidth) / m_dragStartBounds.width();
        double ratioH = static_cast<double>(newHeight) / m_dragStartBounds.height();
        if (ratioW > ratioH) {
            newHeight = qMax(static_cast<int>(newWidth / m_aspectRatio), 150);
        } else {
            newWidth = qMax(static_cast<int>(newHeight * m_aspectRatio), 200);
        }
    }

    m_mainWindow->setGeometry(m_dragStartBounds.x(), m_dragStartBounds.y(), newWidth, newHeight);
    return true;
}

bool ImmersiveModeController::handleResizeMouseRelease(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!m_isResizing) return false;

    m_isResizing = false;
    return true;
}
