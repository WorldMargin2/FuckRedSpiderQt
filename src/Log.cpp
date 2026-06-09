#include "Log.h"
#include <QTextCursor>
#include <QScrollBar>
#include <algorithm>

Log::Log(QTextBrowser* textBox, int ignoreTimes)
    : textboxLog(textBox), ignoreTimes(ignoreTimes)
{
}

Log::~Log()
{
    invalidate();
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
    lastLogEntry.reset();
}

void Log::add(const QString& message)
{
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
    }
    if (ignoreTimes < 0) {
        return true;
    }
    return false;
}

void Log::appendLogEntry(const QString& message, LogLevel level)
{
    if (!m_isValid || !textboxLog) return;
    if (shouldIgnore()) return;

    QDateTime now = QDateTime::currentDateTime();

    // 检查是否与上一条日志相同（重复日志合并）
    if (lastLogEntry &&
        lastLogEntry->message == message &&
        lastLogEntry->level == level) {

        lastLogEntry->count++;

        // 智能跳过机制：指数退避避免刷屏
        if (lastLogEntry->count < lastLogEntry->nextDisplayCount) {
            if (lastLogEntry->count >= lastLogEntry->skipThreshold) {
                lastLogEntry->skipThreshold = qMin(lastLogEntry->skipThreshold * 2, 100);
                lastLogEntry->nextDisplayCount = lastLogEntry->count + lastLogEntry->skipThreshold;
            }
            return;
        }

        QString html = formatMessage(message, level, lastLogEntry->count);
        textboxLog->append(html);

        lastLogEntry->skipThreshold = 1;
        lastLogEntry->nextDisplayCount = lastLogEntry->count + 1;
    } else {
        lastLogEntry.reset(new LogEntry(message, level, now));
        QString html = formatMessage(message, level);
        textboxLog->append(html);
    }
}

QString Log::formatMessage(const QString& message, LogLevel level, int repeatCount)
{
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("yyyy-M-d");
    QString timeStr = now.toString("h:mm:ss");

    QString levelPrefix;
    QString colorStyle;

    switch (level) {
    case LogLevel::Warning:
        levelPrefix = "[警告] ";
        colorStyle = "color:#ff9900;";
        break;
    case LogLevel::Error:
        levelPrefix = "[错误] ";
        colorStyle = "color:#ff0000;";
        break;
    default:
        break;
    }

    QString repeatTag;
    if (repeatCount > 1) {
        repeatTag = QString(" <span style=\"color:gray;\">(重复 %1 次)</span>").arg(repeatCount);
    }

    if (!colorStyle.isEmpty()) {
        return QString("[<span style=\"color:#00ff7f;\">%1</span> <span style=\"color:#0055ff;\">%2</span>] "
                       "%3<span style=\"%4\">%5</span>%6")
            .arg(dateStr).arg(timeStr).arg(levelPrefix).arg(colorStyle).arg(message).arg(repeatTag);
    } else {
        return QString("[<span style=\"color:#00ff7f;\">%1</span> <span style=\"color:#0055ff;\">%2</span>] "
                       "%3%4%5")
            .arg(dateStr).arg(timeStr).arg(levelPrefix).arg(message).arg(repeatTag);
    }
}
