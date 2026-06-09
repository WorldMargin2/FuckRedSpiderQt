#include "GlobalKeyboardHookGuard.h"
#include <QCoreApplication>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

GlobalKeyboardHookGuard *GlobalKeyboardHookGuard::instance = nullptr;

GlobalKeyboardHookGuard::GlobalKeyboardHookGuard(QObject *parent) : QObject(parent) {
    if (instance == nullptr) {
        instance = this;
    }
    keyboardHook = nullptr;
    started = false;
}

GlobalKeyboardHookGuard::~GlobalKeyboardHookGuard() {
    stop();
    if (instance == this) {
        instance = nullptr;
    }
}

void GlobalKeyboardHookGuard::start() {
    if (keyboardHook != nullptr) {
        return;
    }

    HMODULE hMod = GetModuleHandle(nullptr);
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hMod, 0);

    if (!keyboardHook) {
        DWORD errorCode = GetLastError();
        QString errorMsg = QString("安装钩子失败，错误代码: %1。请确保以管理员身份运行。").arg(errorCode);
        emit errorOccurred(errorMsg);
    } else {
        started = true;
        emit guardStarted();
    }
}

void GlobalKeyboardHookGuard::stop() {
    started = false;

    if (keyboardHook != nullptr) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
        emit guardStopped();
    }
}

bool GlobalKeyboardHookGuard::isStarted() const {
    return started;
}

LRESULT CALLBACK GlobalKeyboardHookGuard::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (instance == nullptr || !instance->started) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    if (instance->keyboardHook == nullptr) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    if (nCode >= 0) {
        auto *pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

        if (pKeyboard->flags & LLKHF_INJECTED) {
            return CallNextHookEx(instance->keyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            emit instance->keyPressed(pKeyboard->vkCode);

            INPUT input;
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = pKeyboard->vkCode;
            input.ki.wScan = pKeyboard->scanCode;
            input.ki.dwFlags = 0;
            input.ki.time = 0;
            input.ki.dwExtraInfo = pKeyboard->dwExtraInfo;

            SendInput(1, &input, sizeof(INPUT));

            return 1;
        }
        if (wParam == WM_KEYUP || WM_SYSKEYUP) {
            INPUT input;
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = pKeyboard->vkCode;
            input.ki.wScan = pKeyboard->scanCode;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            input.ki.time = 0;
            input.ki.dwExtraInfo = pKeyboard->dwExtraInfo;

            SendInput(1, &input, sizeof(INPUT));

            return 1;
        }
    }

    return CallNextHookEx(instance->keyboardHook, nCode, wParam, lParam);
}

bool GlobalKeyboardHookGuard::nativeEventFilter(const QByteArray &eventType, void *message, long *result) {
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) {
        emit keyPressed(static_cast<int>(msg->wParam));
    }
    return false;
}
