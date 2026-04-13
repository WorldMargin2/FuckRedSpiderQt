#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <array>
#include <windows.h>
#include <shellapi.h>

bool IsRunningAsAdmin()
{
    BOOL fIsRunAsAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID pAdminSID = nullptr;

    if (AllocateAndInitializeSid(
            &NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &pAdminSID))
    {
        if (!CheckTokenMembership(nullptr, pAdminSID, &fIsRunAsAdmin)) {
            fIsRunAsAdmin = FALSE;
        }
        FreeSid(pAdminSID);
        pAdminSID = nullptr;
    }

    return fIsRunAsAdmin != FALSE;
}

bool RunAsAdmin()
{
    std::array<wchar_t, MAX_PATH> szPath{};
    if (GetModuleFileNameW(nullptr, szPath.data(), static_cast<DWORD>(szPath.size())))
    {
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"runas";
        sei.lpFile = szPath.data();
        sei.hwnd = nullptr;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteExW(&sei))
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_CANCELLED) {
                return false;
            }
        }
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (!IsRunningAsAdmin()) {
        if (RunAsAdmin()) {
            return 0;
        } 
            qDebug() << "Failed to run as administrator, some features may not work properly.";
        
    }

    QApplication a(argc, argv);

    a.addLibraryPath(QCoreApplication::applicationDirPath() + "/platforms");
    a.addLibraryPath(QCoreApplication::applicationDirPath() + "/styles");

    QString fontPath = QDir::currentPath() + "/ALIMAMASHUHEITI-BOLD.TTF";

    if (QFileInfo::exists(fontPath)) {
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            qDebug() << "Successfully loaded font from:" << fontPath;
        } else {
            qDebug() << "Failed to load font from:" << fontPath;
        }
    } else {
        QString appDirPath = QCoreApplication::applicationDirPath() + "/ALIMAMASHUHEITI-BOLD.TTF";
        if (QFileInfo::exists(appDirPath)) {
            int fontId = QFontDatabase::addApplicationFont(appDirPath);
            if (fontId != -1) {
                qDebug() << "Successfully loaded font from:" << appDirPath;
            } else {
                qDebug() << "Failed to load font from:" << appDirPath;
            }
        } else {
            qDebug() << "Font file not found in current directory or application directory.";
            qDebug() << "Current dir font path:" << fontPath;
            qDebug() << "App dir font path:" << appDirPath;
        }
    }

    MainWindow w;
    w.show();
    return QApplication::exec();
}
