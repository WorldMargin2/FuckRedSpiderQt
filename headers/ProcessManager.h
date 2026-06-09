#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QString>
#include <QList>

class Log;

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject *parent = nullptr);

    void setTargetProcessName(const QString &name);
    void setLogger(Log *logger);

    QString targetProcessName() const;

    QList<qulonglong> getCurrentPids() const;
    bool isRunning() const;

    // 获取目标进程可执行文件路径（仅首次获取有效）
    QString resolveExecutablePath();

    // 重命名/恢复可执行文件（用于"自动关闭"功能）
    bool renameExecutableToBackup();
    bool restoreExecutableFromBackup();

    // 终止目标进程
    bool killTarget();

    QString executablePath() const;
    void clearExecutablePath();

signals:
    void processStatusChanged(bool running);
    void executablePathResolved(const QString &path);

private:
    QString m_targetProcessName;
    QString m_executablePath;
    Log *m_logger;
};

#endif // PROCESSMANAGER_H
