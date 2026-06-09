#ifndef LOG_H
#define LOG_H

#include <QTextBrowser>
#include <QString>
#include <QDateTime>
#include <memory>

enum class LogLevel {
    Info,
    Warning,
    Error
};

struct LogEntry {
    QString message;
    LogLevel level;
    QDateTime timestamp;
    int count = 1;
    int skipThreshold = 1;
    int nextDisplayCount = 1;

    LogEntry() = default;
    LogEntry(const QString& msg, LogLevel lvl, const QDateTime& time)
        : message(msg), level(lvl), timestamp(time) {}
};

class Log
{
public:
    Log(QTextBrowser* textBox, int ignoreTimes = 0);
    ~Log();

    void clear();
    void add(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void ignore(int times = 1);
    void unignore();
    void setIgnoreTimes(int times);
    void invalidate();

private:
    QTextBrowser* textboxLog;
    int ignoreTimes;
    std::unique_ptr<LogEntry> lastLogEntry;
    bool m_isValid = true;

    bool shouldIgnore();
    void appendLogEntry(const QString& message, LogLevel level);

    // 统一格式化方法
    QString formatMessage(const QString& message, LogLevel level, int repeatCount = 0);
};

#endif // LOG_H
