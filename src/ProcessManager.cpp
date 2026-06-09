#include "ProcessManager.h"
#include "Log.h"
#include "WindowsApi.h"
#include <QFile>
#include <array>
#include <windows.h>
#include <psapi.h>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
    , m_logger(nullptr)
{
}

void ProcessManager::setTargetProcessName(const QString &name)
{
    m_targetProcessName = name;
}

void ProcessManager::setLogger(Log *logger)
{
    m_logger = logger;
}

QString ProcessManager::targetProcessName() const
{
    return m_targetProcessName;
}

QList<qulonglong> ProcessManager::getCurrentPids() const
{
    return WindowsApi::getProcessIdsByName(m_targetProcessName);
}

bool ProcessManager::isRunning() const
{
    return !getCurrentPids().isEmpty();
}

QString ProcessManager::resolveExecutablePath()
{
    if (!m_executablePath.isEmpty()) {
        return m_executablePath;
    }

    QList<qulonglong> pids = getCurrentPids();
    if (pids.isEmpty()) {
        return QString();
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pids.first());
    if (hProcess != nullptr) {
        std::array<WCHAR, MAX_PATH> processPath{};
        processPath.fill(L'\0');
        if (GetModuleFileNameExW(hProcess, nullptr, processPath.data(), static_cast<DWORD>(processPath.size()))) {
            m_executablePath = QString::fromWCharArray(processPath.data());
            if (m_logger) {
                m_logger->info(QString("获取到目标进程路径 %1").arg(m_executablePath));
            }
            emit executablePathResolved(m_executablePath);
        }
        CloseHandle(hProcess);
    } else if (m_logger) {
        m_logger->warning(QString("无法打开目标进程 (PID: %1)，请检查权限").arg(pids.first()));
    }

    return m_executablePath;
}

bool ProcessManager::renameExecutableToBackup()
{
    if (m_executablePath.isEmpty()) {
        return false;
    }

    if (QFile::exists(m_executablePath)) {
        QString backupPath = m_executablePath + ".bak";
        if (QFile::rename(m_executablePath, backupPath)) {
            if (m_logger) {
                m_logger->info(QString("已将可执行文件重命名为备份文件 %1 -> %2").arg(m_executablePath, backupPath));
            }
            return true;
        } else {
            if (m_logger) {
                m_logger->error(QString("重命名可执行文件失败: %1").arg(m_executablePath));
            }
            return false;
        }
    }
    return true; // 文件不存在说明已经重命名过了
}

bool ProcessManager::restoreExecutableFromBackup()
{
    if (m_executablePath.isEmpty()) {
        return false;
    }

    QString backupPath = m_executablePath + ".bak";
    if (QFile::exists(backupPath)) {
        if (QFile::rename(backupPath, m_executablePath)) {
            if (m_logger) {
                m_logger->info(QString("已将备份文件重命名回原文件: %1 -> %2").arg(backupPath, m_executablePath));
            }
            return true;
        } else {
            if (m_logger) {
                m_logger->error(QString("重命名备份文件失败: %1").arg(backupPath));
            }
            return false;
        }
    }
    return true; // 备份不存在说明已恢复
}

bool ProcessManager::killTarget()
{
    bool success = WindowsApi::terminateProcessByName(m_targetProcessName);
    if (m_logger) {
        if (success) {
            m_logger->info(QString("成功终止进程: %1").arg(m_targetProcessName));
        } else {
            m_logger->error(QString("终止进程失败: %1").arg(m_targetProcessName));
        }
    }
    return success;
}

QString ProcessManager::executablePath() const
{
    return m_executablePath;
}

void ProcessManager::clearExecutablePath()
{
    m_executablePath.clear();
}
