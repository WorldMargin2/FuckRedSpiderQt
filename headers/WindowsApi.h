#ifndef WINDOWSAPI_H
#define WINDOWSAPI_H

#include <windows.h>
#include <psapi.h>
#include <QPoint>
#include <QString>

class WindowsApi
{
public:
    static bool showWindow(HWND hwnd, int nCmdShow);
    static bool isIconic(HWND hWnd);
    static bool sendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static bool setWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
    static HWND findWindow(const QString &lpClassName, const QString &lpWindowName = QString());
    static int getWindowThreadProcessId(HWND hWnd, DWORD *lpdwProcessId);
    static HWND setParent(HWND hWndChild, HWND hWndNewParent);
    static LONG setWindowLong(HWND hWnd, int nIndex, LONG dwNewLong);
    static LONG getWindowLong(HWND hWnd, int nIndex);
    static HWND getParent(HWND hWnd);
    static void removeWindowTitle(HWND vHandle);
    static void setChildWindow(HWND child, HWND parent);
    static bool isSameProcess(HWND windowHandle, DWORD targetPid);
    static bool terminateProcessByName(const QString &processName);
    static QList<qulonglong> getProcessIdsByName(const QString &processName);
};

#endif // WINDOWSAPI_H
