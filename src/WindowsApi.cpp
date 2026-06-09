#include "WindowsApi.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <array>
#include <QVector>
#include <QList>

bool WindowsApi::showWindow(HWND hwnd, int nCmdShow)
{
    return ::ShowWindow(hwnd, nCmdShow);
}

bool WindowsApi::isIconic(HWND hWnd)
{
    return ::IsIconic(hWnd);
}

bool WindowsApi::sendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return ::SendMessage(hWnd, Msg, wParam, lParam);
}

bool WindowsApi::setWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    return ::SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

HWND WindowsApi::findWindow(const QString &lpClassName, const QString &lpWindowName)
{
    return ::FindWindowW(reinterpret_cast<LPCWSTR>(lpClassName.utf16()),
                         lpWindowName.isEmpty() ? nullptr : reinterpret_cast<LPCWSTR>(lpWindowName.utf16()));
}

int WindowsApi::getWindowThreadProcessId(HWND hWnd, DWORD *lpdwProcessId)
{
    return ::GetWindowThreadProcessId(hWnd, lpdwProcessId);
}

HWND WindowsApi::setParent(HWND hWndChild, HWND hWndNewParent)
{
    return ::SetParent(hWndChild, hWndNewParent);
}

LONG WindowsApi::setWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
    return ::SetWindowLongW(hWnd, nIndex, dwNewLong);
}

LONG WindowsApi::getWindowLong(HWND hWnd, int nIndex)
{
    return ::GetWindowLongW(hWnd, nIndex);
}

HWND WindowsApi::getParent(HWND hWnd)
{
    return ::GetParent(hWnd);
}

void WindowsApi::removeWindowTitle(HWND vHandle)
{
    LONG style = getWindowLong(vHandle, GWL_STYLE);
    style &= ~WS_CAPTION;
    setWindowLong(vHandle, GWL_STYLE, style);
}

void WindowsApi::setChildWindow(HWND child, HWND parent)
{
    showWindow(child, SW_HIDE);
    setParent(child, parent);
    removeWindowTitle(child);
    showWindow(child, SW_SHOWMAXIMIZED);
}

bool WindowsApi::isSameProcess(HWND windowHandle, DWORD targetPid)
{
    DWORD pid;
    getWindowThreadProcessId(windowHandle, &pid);

    // 检查PID是否匹配
    if (pid == targetPid) {
        return true;
    }

    // 如果PID不直接匹配，通过进程名进一步验证
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != nullptr) {
        std::array<WCHAR, MAX_PATH> processName{};
        processName.fill(L'\0');
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseNameW(hProcess, hMod, processName.data(), static_cast<DWORD>(processName.size()));

            QString qProcessName = QString::fromWCharArray(processName.data());

            // 检查进程名是否匹配目标进程名
            if (qProcessName.contains("REDAgent", Qt::CaseInsensitive) ||
                qProcessName.contains("RedEagle", Qt::CaseInsensitive)) {
                CloseHandle(hProcess);
                return true;
            }
        }

        CloseHandle(hProcess);
    }

    return false;
}

bool WindowsApi::terminateProcessByName(const QString &processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return false;
    }

    bool foundAndTerminated = false;
    do {
        QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
        // 检查进程名是否匹配（不区分大小写）
        if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0 ||
            currentProcessName.section(".", 0, 0).compare(processName, Qt::CaseInsensitive) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProcess != nullptr) {
                // 尝试终止进程
                BOOL result = TerminateProcess(hProcess, 1);
                if (!result) {
                    // 如果直接终止失败，尝试提升权限后终止
                    DWORD err = GetLastError();
                    if (err == ERROR_ACCESS_DENIED) {
                        // 尝试获取调试权限
                        HANDLE hToken;
                        TOKEN_PRIVILEGES tp;

                        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
                            LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
                            tp.PrivilegeCount = 1;
                            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);

                            CloseHandle(hToken);

                            // 再次尝试打开进程并终止
                            hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
                            if (hProcess != nullptr) {
                                result = TerminateProcess(hProcess, 1);
                            }
                        }
                    }
                }

                CloseHandle(hProcess);
                if (result) {
                    foundAndTerminated = true;
                }
            }
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return foundAndTerminated;
}

QList<qulonglong> WindowsApi::getProcessIdsByName(const QString &processName)
{
    QList<qulonglong> pids;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return pids;
    }

    do {
        QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
        // 检查进程名是否匹配（不区分大小写）
        if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0) {
            pids.append(pe32.th32ProcessID);
        }
        // 也检查不带扩展名的进程名
        else if (currentProcessName.section(".", 0, 0).compare(processName, Qt::CaseInsensitive) == 0) {
            pids.append(pe32.th32ProcessID);
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return pids;
}