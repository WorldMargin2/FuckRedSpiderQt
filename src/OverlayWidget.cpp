#include "OverlayWidget.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QtMath>

OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent)
    , m_highlightRect()
{

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);  // WS_EX_TRANSPARENT: 鼠标事件穿透到下层窗口
    setAttribute(Qt::WA_ShowWithoutActivating);       // WS_EX_NOACTIVATE: 不抢焦点


    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);    // 改为 true，禁止系统绘制背景

    // 覆盖整个虚拟屏幕（对应 Bounds = SystemInformation.VirtualScreen）
    QRect virtualGeometry;
    for (QScreen *screen : QGuiApplication::screens()) {
        virtualGeometry = virtualGeometry.united(screen->geometry());
    }
    if (virtualGeometry.isEmpty()) {
        virtualGeometry = QApplication::desktop()->geometry();
    }
    setGeometry(virtualGeometry);
}

OverlayWidget::~OverlayWidget()
{
}

void OverlayWidget::highlightRect(const QRect &rect)
{
    // 避免重复更新相同区域
    if (m_highlightRect == rect) {
        return;
    }

    // 计算需要重绘的区域（旧区域 + 新区域的并集）
    QRect oldRect = m_highlightRect;
    m_highlightRect = rect;

    if (!isVisible()) {
        show();
        // 首次显示需要全屏重绘
        update();
    } else {
        // 局部更新：只重绘变化的区域，避免全屏闪烁
        if (!oldRect.isEmpty() && !rect.isEmpty()) {
            // 合并旧区域和新区域进行重绘
            QRegion updateRegion = QRegion(oldRect).united(QRegion(rect));
            // 扩展更新区域以覆盖边框
            updateRegion = updateRegion + QRegion(oldRect.adjusted(-10, -10, 10, 10)) +
                          QRegion(rect.adjusted(-10, -10, 10, 10));
            update(updateRegion);
        } else if (!rect.isEmpty()) {
            // 只有新区域
            update(rect.adjusted(-10, -10, 10, 10));
        } else {
            // 清除高亮
            update();
        }
    }
}

void OverlayWidget::clear()
{
    m_highlightRect = QRect();
    update();
    if (isVisible()) {
        hide();
    }
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);


    if (!m_highlightRect.isEmpty()) {
        // 方法：先绘制全屏半透明遮罩，然后用 Clear 模式挖掉高亮区域
        
        // 1. 绘制全屏半透明灰色遮罩
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), QColor(128, 128, 128, 80));  // alpha=80，淡淡的遮罩
        
        // 2. 清除高亮区域，让它完全透明
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(m_highlightRect, Qt::transparent);
        
        // 3. 恢复正常绘制模式
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        
        // 4. 绘制青色边框
        const qreal penWidth = 6.0;
        QPen pen(QColor(0, 255, 255, 255), penWidth);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        int inflate = qCeil(penWidth / 2.0);
        painter.drawRect(m_highlightRect.adjusted(-inflate, -inflate, inflate, inflate));
    } else {
        // 没有高亮区域时，绘制全屏半透明遮罩
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), QColor(128, 128, 128, 80));
    }
}

// 鼠标/键盘事件：由于设置了 WA_TransparentForMouseEvents，
// 这些事件会自动穿透到下层窗口，不需要手动处理。
// 但保留空实现以防万一有直接发送到此 widget 的事件。
void OverlayWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void OverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void OverlayWidget::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}
