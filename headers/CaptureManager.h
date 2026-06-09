#ifndef CAPTUREMANAGER_H
#define CAPTUREMANAGER_H

#include <QObject>
#include <QPoint>
#include <windows.h>

class OverlayWidget;
class QWidget;
class Log;

class CaptureManager : public QObject
{
    Q_OBJECT

public:
    explicit CaptureManager(QObject *parent = nullptr);
    ~CaptureManager();

    void setOverlay(OverlayWidget *overlay);
    void setLogger(Log *logger);

    bool isCapturing() const;

    // 开始窗口捕获（显示覆盖层）
    void startCapture();

    // 停止窗口捕获（隐藏覆盖层）
    void stopCapture();

    // 更新光标位置的高亮区域
    void updateCaptureAtCursor(const QPoint &globalPos);

    // 应用当前捕获到的窗口
    void applyCapturedWindow();

    // 获取捕获结果
    HWND capturedWindow() const;
    QString capturedClassName() const;
    QString capturedExePath() const;

signals:
    void captureStarted();
    void captureStopped();
    void captureApplied(HWND window, const QString &className, const QString &exePath);

private:
    bool m_isCapturing;
    OverlayWidget *m_overlay;
    QPoint m_origin;
    HWND m_capturedWindow;
    QString m_capturedClassName;
    QString m_capturedExePath;
    Log *m_logger;
};

#endif // CAPTUREMANAGER_H
