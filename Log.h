#ifndef LOG_H
#define LOG_H

#include <QTextBrowser>
#include <QString>
#include <QDateTime>

enum class LogLevel {
    Info,
    Warning,
    Error
};

struct LogEntry {
    QString message;
    LogLevel level;
    QDateTime timestamp;
    int count; // 用于记录重复次数
    int skipThreshold; // 跳过阈值，达到此值后才显示
    int nextDisplayCount; // 下一次显示需要的计数

    LogEntry(const QString& msg, LogLevel lvl, const QDateTime& time)
        : message(msg), level(lvl), timestamp(time), count(1), skipThreshold(1), nextDisplayCount(1) {}
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
    void invalidate();  // 新增：标记日志对象为无效状态

private:
    QTextBrowser* textboxLog;
    int ignoreTimes;
    LogEntry* lastLogEntry; // 存储上一条日志，用于检测重复
    bool m_isValid = true;  // 新增：有效性标志
    bool shouldIgnore();
    QString formatLogEntry(const QString& message, LogLevel level = LogLevel::Info);
    QString formatLogEntryWithCount(const QString& message, LogLevel level, int count);
    void appendLogEntry(const QString& message, LogLevel level);
};

#endif // LOG_H