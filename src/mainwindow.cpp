#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GlobalKeyboardHookGuard.h"
#include "AboutPage.h"
#include "Log.h"
#include "ProcessManager.h"
#include "HijackManager.h"
#include "HotkeyManager.h"
#include "CaptureManager.h"
#include "ImmersiveModeController.h"
#include "WindowsApi.h"
#include "OverlayWidget.h"

#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , keyboardGuard(nullptr)
    , aboutPage(nullptr)
    , logger(nullptr)
    , processManager(nullptr)
    , hijackManager(nullptr)
    , hotkeyManager(nullptr)
    , captureManager(nullptr)
    , overlay(nullptr)
    , immersiveController(nullptr)
    , listenerTimer(nullptr)
{
    ui->setupUi(this);

    initManagers();
    initUI();
    setupConnections();
    setupTimers();

    setWindowTitle("红蜘蛛终结者");
    setWindowIcon(QIcon(":/Resources/FuckRedSpider.ico"));

    logger->info("程序启动");

    QTimer::singleShot(LISTENER_DELAY_MS, this, [this]() {
        if (listenerTimer) {
            listenerTimer->start(LISTENER_INTERVAL_MS);
        } else {
            logger->error("监听定时器未初始化，无法启动后台任务");
        }
    });
}

MainWindow::~MainWindow()
{
    // 停止定时器
    if (listenerTimer) {
        listenerTimer->stop();
        delete listenerTimer;
        listenerTimer = nullptr;
    }

    // 停止键盘钩子
    if (keyboardGuard) {
        keyboardGuard->stop();
        delete keyboardGuard;
        keyboardGuard = nullptr;
    }

    // 恢复所有被嵌入的窗口
    if (hijackManager) {
        hijackManager->restoreAllHijackedWindows();
        delete hijackManager;
        hijackManager = nullptr;
    }

    // 清理日志
    if (logger) {
        logger->invalidate();
        delete logger;
        logger = nullptr;
    }

    // 清理热键
    if (hotkeyManager) {
        delete hotkeyManager;
        hotkeyManager = nullptr;
    }

    // 清理窗口捕获
    if (captureManager) {
        delete captureManager;
        captureManager = nullptr;
    }

    if (overlay) {
        delete overlay;
        overlay = nullptr;
    }

    // 清理沉浸式模式控制器
    if (immersiveController) {
        delete immersiveController;
        immersiveController = nullptr;
    }

    // 清理进程管理器（恢复被重命名的文件）
    if (processManager) {
        processManager->restoreExecutableFromBackup();
        delete processManager;
        processManager = nullptr;
    }

    delete ui;
}

// ===== 初始化 =====

void MainWindow::initManagers()
{
    // 日志系统
    logger = new Log(ui->textBrowser, 0);

    // 进程管理器
    processManager = new ProcessManager(this);
    processManager->setLogger(logger);

    // 窗口劫持管理器
    hijackManager = new HijackManager(this);
    hijackManager->setLogger(logger);
    hijackManager->setFullWindowClassName("DIBFullViewClass");
    hijackManager->setNormalWindowClassName("RedEagle.Monitor");

    // 热键管理器
    hotkeyManager = new HotkeyManager(this, this);
    hotkeyManager->setLogger(logger);
    hotkeyManager->setupDefaultHotkeys();

    // 窗口捕获管理器
    overlay = new OverlayWidget(this);
    captureManager = new CaptureManager(this);
    captureManager->setOverlay(overlay);
    captureManager->setLogger(logger);

    // 沉浸式模式控制器
    immersiveController = new ImmersiveModeController(this, this);
    immersiveController->setLogger(logger);

    // 键盘钩子守护
    keyboardGuard = new GlobalKeyboardHookGuard(this);
}

void MainWindow::initUI()
{
    // 加载自定义字体
    int fontId = QFontDatabase::addApplicationFont(":/Resources/AlimamaShuHeiTi-Bold.ttf");
    if (fontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QApplication::setFont(QFont(families.at(0)));
            logger->info("字体加载成功");
        }
    } else {
        logger->error("字体加载失败");
    }

    // 获取自身进程名
    thisProcessName = QCoreApplication::applicationName();
    if (thisProcessName.isEmpty()) {
        thisProcessName = QApplication::applicationFilePath();
        thisProcessName = thisProcessName.section('/', -1).section('.', 0, 0);
    }

    // 设置默认值
    processManager->setTargetProcessName("REDAgent");
    ui->edit_target_process->setText(processManager->targetProcessName());
    ui->edit_full_window_class_name->setText(hijackManager ? "DIBFullViewClass" : "");
    ui->edit_normal_window_class_name->setText(hijackManager ? "RedEagle.Monitor" : "");

    // 布局策略
    ui->Topest_tab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->tab_config->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->group_log->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 沉浸式模式拖拽控件初始化
    QLabel *labelMoveWindow = ui->label_move_window;
    QLabel *labelResizeWindow = ui->label_resize_window;
    labelMoveWindow->setParent(ui->centralwidget);
    labelResizeWindow->setParent(ui->centralwidget);
    labelMoveWindow->setAttribute(Qt::WA_TranslucentBackground);
    labelResizeWindow->setAttribute(Qt::WA_TranslucentBackground);
    labelMoveWindow->setStyleSheet("background: transparent;");
    labelResizeWindow->setStyleSheet("background: transparent;");
    labelMoveWindow->setGeometry(0, 0, 23, 25);
    labelResizeWindow->setGeometry(this->width() - 27, this->height() - 27, 25, 25);
    labelMoveWindow->hide();
    labelResizeWindow->hide();
    labelMoveWindow->installEventFilter(this);
    labelResizeWindow->installEventFilter(this);

    immersiveController->setMoveLabel(labelMoveWindow);
    immersiveController->setResizeLabel(labelResizeWindow);

    // 工具栏 Z 序 + 底部居中
    ui->widget_toolbar->raise();
    ui->widget_toolbar->installEventFilter(this);
    int toolbarWidth = ui->widget_toolbar->width();
    int toolbarHeight = ui->widget_toolbar->height();
    ui->widget_toolbar->setGeometry(
        (this->width() - toolbarWidth) / 2,
        this->height() - toolbarHeight - 4,
        toolbarWidth, toolbarHeight);
    if (labelMoveWindow) labelMoveWindow->raise();
    if (labelResizeWindow) labelResizeWindow->raise();

    // 显示主界面
    ui->stacked_main->setCurrentWidget(ui->page_main_ui);

    // 键盘钩子信号连接
    if (keyboardGuard) {
        connect(keyboardGuard, &GlobalKeyboardHookGuard::keyPressed,
                this, &MainWindow::onKeyPressed);
    }
}

void MainWindow::setupConnections()
{
    // 复选框状态变化
    connect(ui->check_auto_kill, &QCheckBox::stateChanged, this, &MainWindow::on_auto_kill_stateChanged);
    connect(ui->check_auto_hide, &QCheckBox::stateChanged, this, &MainWindow::on_auto_hide_stateChanged);
    connect(ui->check_attached_handle, &QCheckBox::stateChanged, this, &MainWindow::on_attached_target_stateChanged);
    connect(ui->check_keyboard_guard_in_time, &QCheckBox::stateChanged, this, &MainWindow::on_keyboard_guard_in_time_stateChanged);
    connect(ui->check_immersive_mode, &QCheckBox::stateChanged, this, &MainWindow::on_immersive_mode_stateChanged);
    connect(ui->check_keep_ratio, &QCheckBox::stateChanged, this, &MainWindow::on_check_keep_ratio_stateChanged);

    // 进程名和类名编辑框
    connect(ui->edit_target_process, &QLineEdit::textChanged, this, &MainWindow::on_process_name_textChanged);
    connect(ui->edit_full_window_class_name, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (hijackManager) hijackManager->setFullWindowClassName(text.trimmed());
    });
    connect(ui->edit_normal_window_class_name, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (hijackManager) hijackManager->setNormalWindowClassName(text.trimmed());
    });

    // 窗口捕获按钮
    connect(ui->button_start_capture, &QPushButton::clicked, this, &MainWindow::on_button_start_capture_clicked);
    connect(ui->button_apply_capture, &QPushButton::clicked, this, &MainWindow::on_button_apply_capture_clicked);
    ui->button_start_capture->installEventFilter(this);

    // 热键触发信号
    connect(hotkeyManager, &HotkeyManager::hotkeyTriggered, this, &MainWindow::onHotkeyTriggered);
}

void MainWindow::setupTimers()
{
    listenerTimer = new QTimer(this);
    connect(listenerTimer, &QTimer::timeout, this, &MainWindow::listen);
}

// ===== 后台任务控制 =====

void MainWindow::startBackgroundTasks()
{
    if (!listenerTimer->isActive()) {
        listenerTimer->start(BACKGROUND_TASK_INTERVAL_MS);
    }
}

void MainWindow::stopBackgroundTasks()
{
    if (listenerTimer->isActive()) {
        listenerTimer->stop();
    }
}

bool MainWindow::isBackgroundTaskRunning() const
{
    return listenerTimer->isActive();
}

void MainWindow::restartKeyboardGuard()
{
    if (keyboardGuard) {
        keyboardGuard->stop();
        keyboardGuard->start();
    }
}

// ===== 监听循环 =====

void MainWindow::listen()
{
    try {
        if (!ui || !logger) return;

        // 保持置顶
        if (ui->check_keep_topest->isChecked()) {
            setWindowFlag(Qt::WindowStaysOnTopHint, true);
            raise();
            activateWindow();
        }

        // 实时键盘守护：每周期重启钩子防止被劫持
        if (ui->check_keyboard_guard_in_time->isChecked()) {
            restartKeyboardGuard();
        }

        // 查询目标进程
        QList<qulonglong> currentPids = processManager->getCurrentPids();
        QStringList currentPidsStr;
        for (qulonglong pid : currentPids) {
            currentPidsStr << QString::number(static_cast<quint64>(pid));
        }

        // 首次获取可执行文件路径
        if (processManager->executablePath().isEmpty() && !currentPids.isEmpty()) {
            processManager->resolveExecutablePath();
        }

        // 自动关闭功能：重命名/恢复可执行文件
        if (!processManager->executablePath().isEmpty()) {
            if (ui->check_auto_kill->isChecked()) {
                processManager->renameExecutableToBackup();
            } else {
                processManager->restoreExecutableFromBackup();
            }
        }

        // 更新UI状态
        if (!currentPids.isEmpty()) {
            ui->label_is_running->setText("运行中");
            ui->label_is_running->setStyleSheet("color: green;");
            ui->label_pid->setText(currentPidsStr.first());
            ui->widget_pid->show();

            bool pidsChanged = (lastPids != currentPidsStr);
            if (pidsChanged) {
                lastPids = currentPidsStr;
            }

            // 根据选中模式执行对应操作
            if (ui->check_auto_kill->isChecked()) {
                processManager->killTarget();
            } else if (ui->check_auto_hide->isChecked()) {
                hideTargetWindows();
            } else if (ui->check_attached_handle->isChecked()) {
                hijackManager->attachToHost(ui->page_target_panel);
                restartKeyboardGuard();
            }
        } else {
            ui->label_is_running->setText("未运行");
            ui->label_is_running->setStyleSheet("color: red;");
            ui->label_pid->setText("None");
            ui->widget_pid->hide();
        }
    } catch (const std::exception& e) {
        static int exceptionCount = 0;
        if (exceptionCount < 10) {
            logger->error(QString("listen()函数发生异常: %1").arg(e.what()));
        }
        exceptionCount++;
    } catch (...) {
        static int unknownExceptionCount = 0;
        if (unknownExceptionCount < 10) {
            logger->error("listen()函数发生未知异常");
        }
        unknownExceptionCount++;
    }
}

void MainWindow::hideTargetWindows()
{
    QList<qulonglong> targetPids = processManager->getCurrentPids();

    // 尝试隐藏全屏控屏窗口
    HWND h = WindowsApi::findWindow(hijackManager ? "DIBFullViewClass" : QString());
    if (h != nullptr) {
        DWORD pid;
        WindowsApi::getWindowThreadProcessId(h, &pid);
        if (targetPids.contains(pid)) {
            logger->info(QString("尝试隐藏全屏控屏窗口: 0x%1").arg((qulonglong)h, 0, 16));
            WindowsApi::sendMessage(h, WM_CLOSE, 0, 0);
            return;
        }
    }

    // 尝试隐藏普通控屏窗口
    h = WindowsApi::findWindow(hijackManager ? "RedEagle.Monitor" : QString());
    if (h != nullptr) {
        DWORD pid;
        WindowsApi::getWindowThreadProcessId(h, &pid);
        if (targetPids.contains(pid)) {
            logger->info(QString("尝试隐藏普通控屏窗口: 0x%1").arg((qulonglong)h, 0, 16));
            WindowsApi::sendMessage(h, WM_CLOSE, 0, 0);
        }
    }
}

// ===== UI 槽函数 =====

void MainWindow::onKeyPressed(int key)
{
    Q_UNUSED(key);
}

void MainWindow::on_button_about_clicked()
{
    if (!aboutPage) {
        aboutPage = new AboutPage(this);
        aboutPage->setAttribute(Qt::WA_DeleteOnClose);
        connect(aboutPage, &QDialog::finished, [this]() { aboutPage = nullptr; });
    }
    aboutPage->show();
    aboutPage->raise();
    aboutPage->activateWindow();
}

void MainWindow::on_check_keep_topest_stateChanged(int arg1)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, arg1 == Qt::Checked);
    show();
}

void MainWindow::on_check_keep_ratio_stateChanged(int arg1)
{
    bool enabled = (arg1 == Qt::Checked);
    ui->spin_ratio_width->setEnabled(enabled);
    ui->spin_ratio_height->setEnabled(enabled);

    // 同步到沉浸式模式控制器
    if (immersiveController) {
        int w = enabled ? ui->spin_ratio_width->value() : 0;
        int h = enabled ? ui->spin_ratio_height->value() : 0;
        immersiveController->setKeepRatio(enabled, w, h);
    }
}

void MainWindow::on_auto_kill_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked) {
        ui->check_auto_hide->setChecked(false);
        ui->check_attached_handle->setChecked(false);
    }
}

void MainWindow::on_auto_hide_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked) {
        ui->check_auto_kill->setChecked(false);
        ui->check_attached_handle->setChecked(false);
    }
}

void MainWindow::on_attached_target_stateChanged(int arg1)
{
    bool active = (arg1 == Qt::Checked);

    ui->check_keep_ratio->setEnabled(active);
    if (!active) {
        ui->spin_ratio_width->setEnabled(false);
        ui->spin_ratio_height->setEnabled(false);
    } else {
        bool keepRatio = ui->check_keep_ratio->isChecked();
        ui->spin_ratio_width->setEnabled(keepRatio);
        ui->spin_ratio_height->setEnabled(keepRatio);
    }

    if (active) {
        ui->check_auto_hide->setChecked(false);
        ui->check_auto_kill->setChecked(false);
        keyboardGuard->start();
    } else {
        keyboardGuard->stop();
        hijackManager->restoreAllHijackedWindows();
        hijackManager->resetTargetWindow();
    }
}

void MainWindow::on_keyboard_guard_in_time_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    if (arg1 == Qt::Unchecked && !ui->check_attached_handle->isChecked()) {
        keyboardGuard->stop();
    }
}

void MainWindow::on_process_name_textChanged()
{
    // 先恢复可能被重命名的文件
    processManager->restoreExecutableFromBackup();
    processManager->clearExecutablePath();

    QString tmp = ui->edit_target_process->text().trimmed();
    bool isSelf = (tmp == thisProcessName);

    ui->check_auto_hide->setEnabled(!isSelf);
    ui->check_auto_kill->setEnabled(!isSelf);
    ui->check_attached_handle->setEnabled(!isSelf);

    if (isSelf) {
        logger->warning("目标进程与自身名称相同，禁用危险操作");
    } else {
        processManager->setTargetProcessName(tmp);
    }
}

void MainWindow::on_label2_doubleClicked()
{
    ui->edit_target_process->setText("REDAgent");
    on_process_name_textChanged();
}

void MainWindow::on_f_w_c_l_doubleClicked()
{
    ui->edit_full_window_class_name->setText("DIBFullViewClass");
    if (hijackManager) hijackManager->setFullWindowClassName("DIBFullViewClass");
}

void MainWindow::on_n_w_c_l_doubleClicked()
{
    ui->edit_normal_window_class_name->setText("RedEagle.Monitor");
    if (hijackManager) hijackManager->setNormalWindowClassName("RedEagle.Monitor");
}

void MainWindow::on_immersive_mode_stateChanged(int arg1)
{
    bool active = (arg1 == Qt::Checked);

    if (immersiveController) {
        immersiveController->setActive(active);
    }

    // 沉浸式模式启用时切换到目标面板页面
    if (active) {
        ui->stacked_main->setCurrentWidget(ui->page_target_panel);
    } else {
        ui->stacked_main->setCurrentWidget(ui->page_main_ui);
    }
}

// ===== 热键触发处理 =====

void MainWindow::onHotkeyTriggered(const QString &actionName)
{
    if (actionName == QStringLiteral("置顶")) {
        ui->check_keep_topest->setChecked(!ui->check_keep_topest->isChecked());
    } else if (actionName == QStringLiteral("自动关闭目标窗体")) {
        ui->check_auto_kill->setChecked(!ui->check_auto_kill->isChecked());
    } else if (actionName == QStringLiteral("自动隐藏目标窗体")) {
        ui->check_auto_hide->setChecked(!ui->check_auto_hide->isChecked());
    } else if (actionName == QStringLiteral("劫持目标窗体")) {
        ui->check_attached_handle->setChecked(!ui->check_attached_handle->isChecked());
    } else if (actionName == QStringLiteral("捕获当前窗体")) {
        on_button_start_capture_clicked();
    }
}

// ===== 窗口捕获槽 =====

void MainWindow::on_button_start_capture_clicked()
{
    captureManager->startCapture();
}

void MainWindow::on_button_apply_capture_clicked()
{
    captureManager->applyCapturedWindow();
}

void MainWindow::on_button_open_exe_dir_clicked()
{
    QString exePath = captureManager->capturedExePath();
    if (!exePath.isEmpty()) {
        QFileInfo fi(exePath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
    }
}

// ===== 热键配置槽 =====

void MainWindow::on_button_hk_topmost_clicked(bool checked)
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (checked && btn) {
        hotkeyManager->startRecording(btn);
    }
}

void MainWindow::on_button_hk_autokill_clicked(bool checked)
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (checked && btn) {
        hotkeyManager->startRecording(btn);
    }
}

void MainWindow::on_button_hk_autohide_clicked(bool checked)
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (checked && btn) {
        hotkeyManager->startRecording(btn);
    }
}

void MainWindow::on_button_hk_attach_clicked(bool checked)
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (checked && btn) {
        hotkeyManager->startRecording(btn);
    }
}

void MainWindow::on_button_hk_capture_clicked(bool checked)
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (checked && btn) {
        hotkeyManager->startRecording(btn);
    }
}

void MainWindow::on_button_reset_hotkeys_clicked()
{
    hotkeyManager->resetAllHotkeys();
}

void MainWindow::on_check_attached_handle_stateChanged(int arg1)
{
    on_attached_target_stateChanged(arg1);
}

// ===== 事件过滤与事件处理 =====

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 沉浸式模式拖拽控件事件转发
    if (immersiveController && immersiveController->isActive()) {
        QLabel *moveLabel = ui->label_move_window;
        QLabel *resizeLabel = ui->label_resize_window;

        if (obj == moveLabel || obj == resizeLabel) {
            QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (!mouseEvent) return QMainWindow::eventFilter(obj, event);

            if (obj == moveLabel) {
                switch (event->type()) {
                case QEvent::MouseButtonPress:
                    return immersiveController->handleMoveMousePress(mouseEvent);
                case QEvent::MouseMove:
                    return immersiveController->handleMoveMouseMove(mouseEvent);
                case QEvent::MouseButtonRelease:
                    return immersiveController->handleMoveMouseRelease(mouseEvent);
                default:
                    break;
                }
            } else if (obj == resizeLabel) {
                switch (event->type()) {
                case QEvent::MouseButtonPress:
                    return immersiveController->handleResizeMousePress(mouseEvent);
                case QEvent::MouseMove:
                    return immersiveController->handleResizeMouseMove(mouseEvent);
                case QEvent::MouseButtonRelease:
                    return immersiveController->handleResizeMouseRelease(mouseEvent);
                default:
                    break;
                }
            }
        }
    }

    // 窗口捕获中的鼠标移动事件
    if (captureManager && captureManager->isCapturing() &&
        event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        captureManager->updateCaptureAtCursor(mouseEvent->globalPos());
    }

    // 热键录制中的按键释放事件
    if (hotkeyManager && hotkeyManager->isRecording() &&
        event->type() == QEvent::KeyRelease) {
        hotkeyManager->finishRecording();
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 窗口捕获中按 Escape 取消
    if (captureManager && captureManager->isCapturing()) {
        if (event->key() == Qt::Key_Escape) {
            captureManager->stopCapture();
            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintPtr *result)
#else
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    Q_UNUSED(message);
    return false;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 工具栏底部居中
    int tw = ui->widget_toolbar->width();
    int th = ui->widget_toolbar->height();
    ui->widget_toolbar->setGeometry((width() - tw) / 2, height() - th - 4, tw, th);

    if (immersiveController && immersiveController->isActive()) {
        immersiveController->onMainWindowResized(width(), height());
    }
}
