#include "PrivilegeHelper.h"
#include <array>
#include <windows.h>
#include <shellapi.h>
#include <QDebug>

bool PrivilegeHelper::isRunningAsAdmin()
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

bool PrivilegeHelper::runAsAdmin()
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
