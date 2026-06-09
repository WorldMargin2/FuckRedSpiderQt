#include "mainwindow.h"
#include "PrivilegeHelper.h"
#include <QApplication>
#include <QDebug>

#ifdef QT_STATICPLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

int main(int argc, char *argv[])
{
    if (!PrivilegeHelper::isRunningAsAdmin()) {
        if (PrivilegeHelper::runAsAdmin()) {
            return 0;
        }
        qDebug() << "未能以管理员权限启动";
    }

    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return QApplication::exec();
}
