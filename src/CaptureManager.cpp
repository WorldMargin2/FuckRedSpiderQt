#include "CaptureManager.h"
#include "OverlayWidget.h"
#include "Log.h"
#include "WindowsApi.h"
#include <QWidget>
#include <QCursor>
#include <QApplication>
#include <array>
#include <windows.h>
#include <psapi.h>

CaptureManager::CaptureManager(QObject *parent)
    : QObject(parent)
    , m_isCapturing(false)
    , m_overlay(nullptr)
    , m_capturedWindow(nullptr)
    , m_logger(nullptr)
{
}

CaptureManager::~CaptureManager()
{
    stopCapture();
}

void CaptureManager::setOverlay(OverlayWidget *overlay)
{
    m_overlay = overlay;
}

void CaptureManager::setLogger(Log *logger)
{
    m_logger = logger;
}

bool CaptureManager::isCapturing() const
{
    return m_isCapturing;
}

void CaptureManager::startCapture()
{
    if (!m_overlay || m_isCapturing) {
        return;
    }

    m_isCapturing = true;
    m_origin = QCursor::pos();
    m_capturedWindow = nullptr;
    m_capturedClassName.clear();
    m_capturedExePath.clear();

    m_overlay->highlightRect(QRect());
    emit captureStarted();
}

void CaptureManager::stopCapture()
{
    if (!m_isCapturing) {
        return;
    }

    m_isCapturing = false;

    if (m_overlay) {
        m_overlay->clear();
    }

    emit captureStopped();
}

void CaptureManager::updateCaptureAtCursor(const QPoint &globalPos)
{
    if (!m_isCapturing || !m_overlay) {
        return;
    }

    POINT pt;
    pt.x = globalPos.x();
    pt.y = globalPos.y();

    HWND hwnd = ::WindowFromPoint(pt);
    if (hwnd == nullptr) {
        m_overlay->highlightRect(QRect());
        return;
    }

    // 跳过自身窗口和覆盖层窗口
    if (hwnd == (HWND)QApplication::activeWindow()->winId() ||
        hwnd == (HWND)m_overlay->winId()) {
        m_overlay->highlightRect(QRect());
        return;
    }

    RECT rect;
    ::GetWindowRect(hwnd, &rect);
    QRect windowRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    m_overlay->highlightRect(windowRect);

    m_capturedWindow = hwnd;

    // 获取窗口类名
    std::array<wchar_t, 256> className{};
    className.fill(L'\0');
    ::GetClassNameW(hwnd, className.data(), static_cast<int>(className.size()));
    m_capturedClassName = QString::fromWCharArray(className.data());

    // 获取进程路径
    DWORD pid;
    ::GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != nullptr) {
        std::array<WCHAR, MAX_PATH> processPath{};
        processPath.fill(L'\0');
        if (GetModuleFileNameExW(hProcess, nullptr, processPath.data(), static_cast<DWORD>(processPath.size()))) {
            m_capturedExePath = QString::fromWCharArray(processPath.data());
        }
        CloseHandle(hProcess);
    }
}

void CaptureManager::applyCapturedWindow()
{
    if (!m_capturedWindow || m_capturedClassName.isEmpty()) {
        if (m_logger) {
            m_logger->warning("没有可用的捕获结果");
        }
        return;
    }

    stopCapture();

    if (m_logger) {
        m_logger->info(QString("应用捕获窗口: 0x%1 (%2)")
                       .arg((qulonglong)m_capturedWindow, 0, 16)
                       .arg(m_capturedClassName));
    }

    emit captureApplied(m_capturedWindow, m_capturedClassName, m_capturedExePath);
}

HWND CaptureManager::capturedWindow() const
{
    return m_capturedWindow;
}

QString CaptureManager::capturedClassName() const
{
    return m_capturedClassName;
}

QString CaptureManager::capturedExePath() const
{
    return m_capturedExePath;
}
