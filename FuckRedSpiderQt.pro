QT       += core gui widgets

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AboutPage.cpp \
    GlobalKeyboardHookGuard.cpp \
    Log.cpp \
    main.cpp \
    mainwindow.cpp \
    WindowsApi.cpp

HEADERS += \
    AboutPage.h \
    GlobalKeyboardHookGuard.h \
    Log.h \
    mainwindow.h \
    WindowsApi.h

FORMS += \
    AboutPage.ui \
    mainwindow.ui

TRANSLATIONS += \
    FuckRedSpiderQt_zh_CN.ts

RESOURCES += resources.qrc

# 设置目标文件输出路径为bin目录
TARGET = FuckRedSpiderQt
DESTDIR = $$PWD/out

# 部署规则
target.path = $$PWD/out
INSTALLS += target

# 设置应用程序图标
RC_ICONS = Resources/WorldMargin.ico

# 添加Windows系统库依赖
LIBS += -lpsapi
