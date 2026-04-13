#include "Log.h"
#include <QTextCursor>
#include <QTextBlock>
#include <QDateTime>
#include <QScrollBar>
#include <algorithm>

Log::Log(QTextBrowser* textBox, int ignoreTimes)
    : textboxLog(textBox), ignoreTimes(ignoreTimes), lastLogEntry(nullptr)
{
}

Log::~Log()
{
    m_isValid = false;
    if (lastLogEntry) {
        delete lastLogEntry;
        lastLogEntry = nullptr;
    }
}

void Log::invalidate()
{
    m_isValid = false;
    textboxLog = nullptr;
}

void Log::clear()
{
    if (textboxLog) {
        textboxLog->clear();
    }
    if (lastLogEntry) {
        delete lastLogEntry;
        lastLogEntry = nullptr;
    }
}

void Log::add(const QString& message)
{
    // 默认为info级别
    appendLogEntry(message, LogLevel::Info);
}

void Log::info(const QString& message)
{
    appendLogEntry(message, LogLevel::Info);
}

void Log::warning(const QString& message)
{
    appendLogEntry(message, LogLevel::Warning);
}

void Log::error(const QString& message)
{
    appendLogEntry(message, LogLevel::Error);
}

void Log::appendLogEntry(const QString& message, LogLevel level)
{
    if (!m_isValid || !textboxLog) return;

    if (shouldIgnore()) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();

    // 检查是否与上一条日志相同
    if (lastLogEntry &&
        lastLogEntry->message == message &&
        lastLogEntry->level == level) {
        // 如果是相同的日志，增加计数
        lastLogEntry->count++;

        // 实现智能跳过机制
        // 如果当前计数小于下次显示需要的数量，则跳过显示
        if (lastLogEntry->count < lastLogEntry->nextDisplayCount) {
            // 跳过显示，但更新跳过阈值
            if (lastLogEntry->count >= lastLogEntry->skipThreshold) {
                // 如果达到了当前阈值，增加跳过阈值并计算下次显示的时机
                lastLogEntry->skipThreshold = qMin(lastLogEntry->skipThreshold * 2, 100); // 最大100
                lastLogEntry->nextDisplayCount = lastLogEntry->count + lastLogEntry->skipThreshold;
            }
            return; // 跳过此次显示
        }

        // 达到显示条件，显示带计数的日志
        QString html = formatLogEntryWithCount(message, level, lastLogEntry->count);
        textboxLog->append(html);

        // 重置显示参数
        lastLogEntry->skipThreshold = 1;
        lastLogEntry->nextDisplayCount = lastLogEntry->count + 1;
    } else {
        // 如果是新日志，保存它
        
            delete lastLogEntry;
        
        lastLogEntry = new LogEntry(message, level, now);

        QString html = formatLogEntry(message, level);
        textboxLog->append(html);
    }
}

void Log::ignore(int times)
{
    ignoreTimes = times;
}

void Log::unignore()
{
    ignoreTimes = 0;
}

void Log::setIgnoreTimes(int times)
{
    ignoreTimes = times;
}

bool Log::shouldIgnore()
{
    if (ignoreTimes > 0) {
        ignoreTimes--;
        return true;
    } if (ignoreTimes < 0) {
        return true;
    }
    return false;
}

QString Log::formatLogEntry(const QString& message, LogLevel level)
{
    // 格式化时间戳
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("yyyy-M-d");  
    QString timeStr = now.toString("h:mm:ss"); 

    // 根据日志级别设置颜色
    QString levelPrefix;
    QString messageColor;

    switch (level) {
        case LogLevel::Info:
            levelPrefix = "";
            messageColor = ""; // 默认颜色
            break;
        case LogLevel::Warning:
            levelPrefix = "[警告] ";
            messageColor = "color:#ff9900;"; // 橙色
            break;
        case LogLevel::Error:
            levelPrefix = "[错误] ";
            messageColor = "color:#ff0000;"; // 红色
            break;
    }

    // 构建HTML格式的日志条目
    QString html;
    if (!messageColor.isEmpty()) {
        html = QString(
            R"([<span style=" color:#00ff7f;">%1</span> <span style=" color:#0055ff;">%2</span>] %3<span style="%4">%5</span>)"
        ).arg(dateStr).arg(timeStr).arg(levelPrefix).arg(messageColor).arg(message);
    } else {
        html = QString(
            R"([<span style=" color:#00ff7f;">%1</span> <span style=" color:#0055ff;">%2</span>] %3%4)"
        ).arg(dateStr).arg(timeStr).arg(levelPrefix).arg(message);
    }

    return html;
}

QString Log::formatLogEntryWithCount(const QString& message, LogLevel level, int count)
{
    // 格式化时间戳
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("yyyy-M-d");  
    QString timeStr = now.toString("h:mm:ss");   

    // 根据日志级别设置颜色
    QString levelPrefix;
    QString messageColor;

    switch (level) {
        case LogLevel::Info:
            levelPrefix = "";
            messageColor = ""; // 默认颜色
            break;
        case LogLevel::Warning:
            levelPrefix = "[警告] ";
            messageColor = "color:#ff9900;"; // 橙色
            break;
        case LogLevel::Error:
            levelPrefix = "[错误] ";
            messageColor = "color:#ff0000;"; // 红色
            break;
    }

    // 构建HTML格式的日志条目，包含重复次数
    QString html;
    if (!messageColor.isEmpty()) {
        html = QString(
            "[<span style=\" color:#00ff7f;\">%1</span> <span style=\" color:#0055ff;\">%2</span>] %3<span style=\"%4\">%5</span> <span style=\"color:gray;\">(重复 %6 次)</span>"
        ).arg(dateStr).arg(timeStr).arg(levelPrefix).arg(messageColor).arg(message).arg(count);
    } else {
        html = QString(
            "[<span style=\" color:#00ff7f;\">%1</span> <span style=\" color:#0055ff;\">%2</span>] %3%4 <span style=\"color:gray;\">(重复 %5 次)</span>"
        ).arg(dateStr).arg(timeStr).arg(levelPrefix).arg(message).arg(count);
    }

    return html;
}