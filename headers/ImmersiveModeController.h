#ifndef IMMERSIVEMODECONTROLLER_H
#define IMMERSIVEMODECONTROLLER_H

#include <QObject>
#include <QLabel>
#include <QPoint>
#include <QRect>

class QMainWindow;
class Log;

class ImmersiveModeController : public QObject
{
    Q_OBJECT

public:
    explicit ImmersiveModeController(QMainWindow *mainWindow, QObject *parent = nullptr);

    void setLogger(Log *logger);

    // 设置拖拽控件引用
    void setMoveLabel(QLabel *label);
    void setResizeLabel(QLabel *label);

    // 切换沉浸式模式
    void setActive(bool active);
    bool isActive() const;

    // 保持比例设置
    void setKeepRatio(bool enable, int width = 0, int height = 0);
    bool isKeepRatio() const;

    // 窗口大小变化时更新控件位置
    void onMainWindowResized(int width, int height);

    // 鼠标事件处理（由 MainWindow 的 eventFilter 转发）
    bool handleMoveMousePress(QMouseEvent *event);
    bool handleMoveMouseMove(QMouseEvent *event);
    bool handleMoveMouseRelease(QMouseEvent *event);
    bool handleResizeMousePress(QMouseEvent *event);
    bool handleResizeMouseMove(QMouseEvent *event);
    bool handleResizeMouseRelease(QMouseEvent *event);

signals:
    void modeChanged(bool active);

private:
    QMainWindow *m_mainWindow;
    QLabel *m_labelMoveWindow;
    QLabel *m_labelResizeWindow;
    Log *m_logger;

    bool m_active;
    bool m_isResizing;
    bool m_isMoving;
    bool m_keepRatio;
    double m_aspectRatio;
    QPoint m_dragStartPos;
    QRect m_dragStartBounds;
};

#endif // IMMERSIVEMODECONTROLLER_H
