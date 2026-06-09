QT       += core gui widgets

DEFINES += QT_DEPRECATED_WARNINGS

msvc {
    QMAKE_CXXFLAGS += /utf-8
}

INCLUDEPATH += headers

SOURCES += \
    src/main.cpp \
    src/AboutPage.cpp \
    src/GlobalKeyboardHookGuard.cpp \
    src/Log.cpp \
    src/mainwindow.cpp \
    src/OverlayWidget.cpp \
    src/WindowsApi.cpp \
    src/PrivilegeHelper.cpp \
    src/ProcessManager.cpp \
    src/HijackManager.cpp \
    src/HotkeyManager.cpp \
    src/CaptureManager.cpp \
    src/ImmersiveModeController.cpp

HEADERS += \
    headers/AboutPage.h \
    headers/GlobalKeyboardHookGuard.h \
    headers/Log.h \
    headers/mainwindow.h \
    headers/OverlayWidget.h \
    headers/WindowsApi.h \
    headers/PrivilegeHelper.h \
    headers/ProcessManager.h \
    headers/HijackManager.h \
    headers/HotkeyManager.h \
    headers/CaptureManager.h \
    headers/ImmersiveModeController.h

FORMS += \
    forms/AboutPage.ui \
    forms/mainwindow.ui

RESOURCES += resources.qrc

TARGET = FuckRedSpiderQt

_PRO_BASE_ = $$dirname(_PRO_FILE_)
CONFIG(debug, debug|release) {
    DESTDIR = $$_PRO_BASE_/build/debug
    OBJECTS_DIR = $$_PRO_BASE_/build/debug
} else {
    DESTDIR = $$_PRO_BASE_/build/release
    OBJECTS_DIR = $$_PRO_BASE_/build/release
}

MOC_DIR = $$_PRO_BASE_/build
RCC_DIR = $$_PRO_BASE_/build
UI_DIR = $$_PRO_BASE_/build

win32 {
    !exists($$_PRO_BASE_/build) {
        system(mkdir "$$_PRO_BASE_\\build")
    }
} else {
    !exists($$_PRO_BASE_/build) {
        system(mkdir -p "$$_PRO_BASE_/build")
    }
}

RC_ICONS = Resources/WorldMargin.ico

LIBS += -lpsapi

win32 {
    static {
        LIBS += -L$$[QT_INSTALL_PLUGINS]/platforms -lqwindows
        DEFINES += QT_STATICPLUGIN
        QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_RELEASE += -Os

        # 构建前自动终止正在运行的进程（避免 Permission denied）
        win32: QMAKE_PRE_LINK = $$quote(cmd /c "taskkill /f /IM $${TARGET}.exe >nul 2>&1" || exit 0)

        STRIP_TARGET = $$shell_path($$DESTDIR/$${TARGET}.exe)
        win32: QMAKE_POST_LINK = strip --strip-all \"$$STRIP_TARGET\"
    }
}
