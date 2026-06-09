#ifndef PRIVILEGEHELPER_H
#define PRIVILEGEHELPER_H

#include <QString>

class PrivilegeHelper
{
public:
    static bool isRunningAsAdmin();
    static bool runAsAdmin();
};

#endif // PRIVILEGEHELPER_H
