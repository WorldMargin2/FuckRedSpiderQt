#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GlobalKeyboardHookGuard.h"
#include "AboutPage.h"
#include <array>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>
#include <QFontDatabase>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , keyboardGuard(nullptr)
    , keyboardGuardRunning(false)
    , aboutPage(nullptr)
    , listenerTimer(nullptr)
    , attachedTargetActive(false)
    , targetPanel(nullptr)
    , lastWindowState(Qt::WindowNoState)
    , targetWindow(nullptr)
    , logger(nullptr)
{
    ui->setupUi(this);

    logger = new Log(ui->textBrowser, 0);

    int fontId = QFontDatabase::addApplicationFont(":/Resources/AlimamaShuHeiTi-Bold.ttf");
    if(fontId != -1)
    {
        logger->info("字体加载成功");
    }else{
        logger->error("字体加载失败");
    }

    keyboardGuard = new GlobalKeyboardHookGuard(this);

    thisProcessName = QCoreApplication::applicationName();
    if (thisProcessName.isEmpty()) {
        thisProcessName = QApplication::applicationFilePath();
        thisProcessName = thisProcessName.section('/', -1).section('.', 0, 0);
    }

    targetProcessName = "REDAgent";
    fullWindowClassName = "DIBFullViewClass";
    normalWindowClassName = "RedEagle.Monitor";


    setupTimers();
    setupConnections();

    ui->edit_target_process->setText(targetProcessName);
    ui->edit_full_window_class_name->setText(fullWindowClassName);
    ui->edit_normal_window_class_name->setText(normalWindowClassName);

    if (keyboardGuard) {
        connect(keyboardGuard, &GlobalKeyboardHookGuard::keyPressed, this, &MainWindow::onKeyPressed);
    }

    setWindowTitle("红蜘蛛终结者");
    setWindowIcon(QIcon(":/Resources/FuckRedSpider.ico"));

    logger->info("程序启动");

    QTimer::singleShot(LISTENER_DELAY_MS, this, [this]() {
        if (listenerTimer) {
            listenerTimer->start(LISTENER_INTERVAL_MS);
            logger->info("后台任务已启动");
        } else {
            logger->error("监听定时器未初始化，无法启动后台任务");
        }
    });

}

MainWindow::~MainWindow()
{
    if (listenerTimer) {
        if (listenerTimer->isActive()) {
            listenerTimer->stop();
        }
        delete listenerTimer;
        listenerTimer = nullptr;
    }

    if (keyboardGuard) {
        keyboardGuard->stop();
        delete keyboardGuard;
        keyboardGuard = nullptr;
    }

    if (logger) {
        logger->invalidate();
        delete logger;
        logger = nullptr;
    }

    if (!origin_executeable_path.isEmpty()) {
        QString backupPath = origin_executeable_path + ".bak";
        if (QFile::exists(backupPath)) {
            QFile::rename(backupPath, origin_executeable_path);
        }
    }

    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (windowState() != lastWindowState) {
        lastWindowState = windowState();
        onResizeEnd();
    }
}

void MainWindow::onResizeEnd()
{
    if (attachedTargetActive) {
        if (targetWindow != nullptr) {
            QWidget *hostWidget = ui->inject_tab;
            if (hostWidget) {
                WindowsApi::setWindowPos(targetWindow, nullptr, 0, 0,
                                         hostWidget->width(), hostWidget->height(),
                                         SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }
}

void MainWindow::setupTimers()
{
    listenerTimer = new QTimer(this);
    connect(listenerTimer, &QTimer::timeout, this, &MainWindow::listen);
}

void MainWindow::startBackgroundTasks()
{
    if (!listenerTimer->isActive()) {
        listenerTimer->start(BACKGROUND_TASK_INTERVAL_MS);
        logger->info("后台任务已启动");
    }
}

void MainWindow::stopBackgroundTasks()
{
    if (listenerTimer->isActive()) {
        listenerTimer->stop();
        logger->info("后台任务已停止");
    }
}

bool MainWindow::isBackgroundTaskRunning() const
{
    return listenerTimer->isActive();
}

void MainWindow::setupConnections()
{
    connect(ui->check_auto_kill, &QCheckBox::stateChanged, this, &MainWindow::on_auto_kill_stateChanged);
    connect(ui->check_auto_hide, &QCheckBox::stateChanged, this, &MainWindow::on_auto_hide_stateChanged);
    connect(ui->check_attached_handle, &QCheckBox::stateChanged, this, &MainWindow::on_attached_target_stateChanged);
    connect(ui->edit_target_process, &QLineEdit::textChanged, this, &MainWindow::on_process_name_textChanged);
}

void MainWindow::onKeyPressed(int key)
{
    Q_UNUSED(key);
}

void MainWindow::on_button_about_clicked()
{
    if (!aboutPage) {
        aboutPage = new AboutPage(this);
        aboutPage->setAttribute(Qt::WA_DeleteOnClose);
        connect(aboutPage, &QDialog::finished, [this]() {
            aboutPage = nullptr;
        });
    }

    aboutPage->show();
    aboutPage->raise();
    aboutPage->activateWindow();
}

void MainWindow::on_check_keep_topest_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked){
        this->setWindowFlag(Qt::WindowStaysOnTopHint,true);
        this->show();
    }else{
        this->setWindowFlag(Qt::WindowStaysOnTopHint,false);
        this->show();
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
    attachedTargetActive = (arg1 == Qt::Checked);
    if (attachedTargetActive) {
        ui->check_auto_hide->setChecked(false);
        ui->check_auto_kill->setChecked(false);
        keyboardGuard->start();
    } else {
        keyboardGuard->stop();
        targetWindow = nullptr;
    }
}

void MainWindow::on_process_name_textChanged()
{
    if (!origin_executeable_path.isEmpty()) {
        QString backupPath = origin_executeable_path + ".bak";
        if (QFile::exists(backupPath)) {
            if (QFile::rename(backupPath, origin_executeable_path)) {
                logger->info(QString("已恢复被重命名的原文件: %1 -> %2").arg(backupPath, origin_executeable_path));
            } else {
                logger->error(QString("恢复被重命名的原文件失败: %1").arg(backupPath));
            }
        }
        origin_executeable_path = "";
    }
    
    QString tmp=ui->edit_target_process->text().trimmed();
    if(tmp==thisProcessName){
        ui->check_auto_hide->setEnabled(false);
        ui->check_auto_kill->setEnabled(false);
        ui->check_attached_handle->setEnabled(false);
        logger->warning("目标进程与自身名称相同，禁用危险操作");
    } else {
        targetProcessName = tmp;
        ui->check_auto_hide->setEnabled(true);
        ui->check_auto_kill->setEnabled(true);
        ui->check_attached_handle->setEnabled(true);
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
}

void MainWindow::on_n_w_c_l_doubleClicked()
{
    ui->edit_normal_window_class_name->setText("RedEagle.Monitor");
}

QString MainWindow::origin_executeable_path="";
void MainWindow::listen()
{
    try {
        if (!ui || !logger) return;

        if (ui->check_keep_topest->isChecked()) {
            this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
            this->raise();
            this->activateWindow();
        }

        QList<qulonglong> currentPids = WindowsApi::getProcessIdsByName(targetProcessName);

        QStringList currentPidsStr;
        for (qulonglong pid : currentPids) {
            currentPidsStr << QString::number(static_cast<quint64>(pid));
        }

        if (origin_executeable_path.isEmpty() && !currentPids.isEmpty()) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, currentPids.first());
            if (hProcess != nullptr) {
                std::array<WCHAR, MAX_PATH> processPath{};
                processPath.fill(L'\0');
                if (GetModuleFileNameExW(hProcess, nullptr, processPath.data(), static_cast<DWORD>(processPath.size()))) {
                    origin_executeable_path = QString::fromWCharArray(processPath.data());
                    logger->info(QString("获取到目标进程路径: %1").arg(origin_executeable_path));
                }
                CloseHandle(hProcess);
            } else {
                static int errorCount = 0;
                if (errorCount % ERROR_LOG_THRESHOLD == 0) {
                    logger->warning(QString("无法打开目标进程 (PID: %1)，请检查权限").arg(currentPids.first()));
                }
                errorCount++;
            }
        }

        if (!origin_executeable_path.isEmpty()) {
            if (ui->check_auto_kill->isChecked()) {
                QString backupPath = origin_executeable_path + ".bak";
                if (QFile::exists(origin_executeable_path)) {
                    if (QFile::rename(origin_executeable_path, backupPath)) {
                        logger->info(QString("已将可执行文件重命名为备份文件: %1 -> %2").arg(origin_executeable_path, backupPath));
                    } else {
                        logger->error(QString("重命名可执行文件失败: %1").arg(origin_executeable_path));
                    }
                }
            } else {
                QString backupPath = origin_executeable_path + ".bak";
                if (QFile::exists(backupPath)) {
                    if (QFile::rename(backupPath, origin_executeable_path)) {
                        logger->info(QString("已将备份文件重命名回原文件: %1 -> %2").arg(backupPath, origin_executeable_path));
                    } else {
                        logger->error(QString("重命名备份文件失败: %1").arg(backupPath));
                    }
                }
            }
        }

        if (!currentPids.isEmpty()) {
            ui->label_is_running->setText("运行中");
            ui->label_is_running->setStyleSheet("color: green;");

            ui->label_pid->setText(currentPidsStr.first());
            ui->widget_pid->show();

            bool pidsChanged = (lastPids != currentPidsStr);
            if (pidsChanged) {
                lastPids = currentPidsStr;
            }

            if (ui->check_auto_kill->isChecked()) {
                killTargetProcesses();
            } else if (ui->check_auto_hide->isChecked()) {
                hideTargetWindows();
            } else if (ui->check_attached_handle->isChecked()) {
                attachTargetWindow();
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

void MainWindow::updateProcessStatus()
{
}

void MainWindow::killTargetProcesses()
{
    bool success = WindowsApi::terminateProcessByName(targetProcessName);

    if (success) {
        logger->info(QString("成功终止进程: %1").arg(targetProcessName));
    } else {
        logger->error(QString("终止进程失败: %1").arg(targetProcessName));
    }
}

void MainWindow::hideTargetWindows()
{
    QList<qulonglong> targetPids = WindowsApi::getProcessIdsByName(targetProcessName);

    HWND h = WindowsApi::findWindow(fullWindowClassName);
    if (h != nullptr) {
        DWORD pid;
        WindowsApi::getWindowThreadProcessId(h, &pid);

        if (targetPids.contains(pid)) {
            logger->info(QString("尝试隐藏全屏控屏窗口: 0x%1")
                                    .arg((qulonglong)h, 0, 16));
            closeHandle(h);
            return;
        }
    }

    h = WindowsApi::findWindow(normalWindowClassName);
    if (h != nullptr) {
        DWORD pid;
        WindowsApi::getWindowThreadProcessId(h, &pid);

        if (targetPids.contains(pid)) {
            logger->info(QString("尝试隐藏普通控屏窗口: 0x%1")
                                    .arg((qulonglong)h, 0, 16));
            closeHandle(h);
            return;
        }
    }
}

void MainWindow::attachTargetWindow()
{
    if (targetWindow != nullptr &&
        WindowsApi::getParent(targetWindow) != nullptr) {
        QWidget *hostWidget = ui->inject_tab;
        if (hostWidget && targetWindow != nullptr) {
            WindowsApi::setWindowPos(targetWindow, nullptr, 0, 0,
                                     hostWidget->width(), hostWidget->height(),
                                     SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return;
    }

    HWND tempWindow = WindowsApi::findWindow(fullWindowClassName);
    if (tempWindow != nullptr) {
        DWORD pid;
        WindowsApi::getWindowThreadProcessId(tempWindow, &pid);

        if (WindowsApi::isSameProcess(tempWindow, pid)) {
            QWidget *hostWidget = ui->inject_tab;

            if (hostWidget) {
                HWND hostHwnd = (HWND)hostWidget->winId();

                hostWidget->show();

                WindowsApi::setChildWindow(tempWindow, hostHwnd);

                targetWindow = tempWindow;

                WindowsApi::setWindowPos(targetWindow, nullptr, 0, 0,
                                         hostWidget->width(), hostWidget->height(),
                                         SWP_NOZORDER | SWP_NOACTIVATE);

                logger->info(QString("成功附加全屏窗口: 0x%1")
                                        .arg((qulonglong)targetWindow, 0, 16));

                keyboardGuard->stop();
                keyboardGuard->start();

                return;
            }
        }
    }

    tempWindow = WindowsApi::findWindow(normalWindowClassName);
    if (tempWindow != nullptr) {
        DWORD pid;
        WindowsApi::getWindowThreadProcessId(tempWindow, &pid);

        if (WindowsApi::isSameProcess(tempWindow, pid)) {
            QWidget *hostWidget = ui->inject_tab;

            if (hostWidget) {
                HWND hostHwnd = (HWND)hostWidget->winId();

                hostWidget->show();

                WindowsApi::setChildWindow(tempWindow, hostHwnd);

                targetWindow = tempWindow;

                WindowsApi::setWindowPos(targetWindow, nullptr, 0, 0,
                                         hostWidget->width(), hostWidget->height(),
                                         SWP_NOZORDER | SWP_NOACTIVATE);

                logger->info(QString("成功附加普通窗口: 0x%1")
                                        .arg((qulonglong)targetWindow, 0, 16));

                return;
            }
        }
    }

    if (targetWindow != nullptr) {
        targetWindow = nullptr;
    }
}

void MainWindow::closeHandle(HWND h)
{
    WindowsApi::setWindowPos(h, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (!WindowsApi::isIconic(h)) {
        WindowsApi::showWindow(h, SW_MINIMIZE);
        WindowsApi::showWindow(h, SW_HIDE);
    }

    WindowsApi::sendMessage(h, WM_CLOSE, 0, 0);
}

void MainWindow::validateProcessName(const QString &processName)
{
    if (processName == thisProcessName) {
        logger->warning("目标进程与自身名称相同，禁用危险操作");
        QMessageBox::warning(this, "警告", "目标进程与自身名称相同，这可能导致程序自杀！");
    }
}
