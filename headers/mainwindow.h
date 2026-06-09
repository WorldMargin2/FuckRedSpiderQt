#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QStringList>

// 前向声明：管理器组件
class GlobalKeyboardHookGuard;
class AboutPage;
class Log;
class OverlayWidget;
class ProcessManager;
class HijackManager;
class HotkeyManager;
class CaptureManager;
class ImmersiveModeController;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static constexpr int LISTENER_DELAY_MS = 100;
    static constexpr int LISTENER_INTERVAL_MS = 14;
    static constexpr int BACKGROUND_TASK_INTERVAL_MS = 1000;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // ===== 核心管理器组件 =====
    GlobalKeyboardHookGuard *keyboardGuard;
    AboutPage *aboutPage;
    Log *logger;

    // 进程管理
    ProcessManager *processManager;

    // 窗口劫持管理
    HijackManager *hijackManager;

    // 热键管理
    HotkeyManager *hotkeyManager;

    // 窗口捕获管理
    CaptureManager *captureManager;
    OverlayWidget *overlay;

    // 沉浸式模式控制
    ImmersiveModeController *immersiveController;

    // ===== 定时器与状态 =====
    QTimer *listenerTimer;
    QString thisProcessName;
    QStringList lastPids;

    // ===== 初始化方法 =====
    void initManagers();
    void initUI();
    void setupConnections();
    void setupTimers();

    // ===== 后台任务 =====
    void startBackgroundTasks();
    void stopBackgroundTasks();
    bool isBackgroundTaskRunning() const;
    void restartKeyboardGuard();

    // ===== 监听循环（核心逻辑）=====
    void listen();

    // 隐藏目标窗口（通过类名查找）
    void hideTargetWindows();

    // ===== UI 槽函数 =====
    void on_button_about_clicked();
    void on_check_keep_topest_stateChanged(int arg1);
    void on_check_keep_ratio_stateChanged(int arg1);
    void on_auto_kill_stateChanged(int arg1);
    void on_auto_hide_stateChanged(int arg1);
    void on_attached_target_stateChanged(int arg1);
    void on_keyboard_guard_in_time_stateChanged(int arg1);
    void on_process_name_textChanged();
    void on_label2_doubleClicked();
    void on_f_w_c_l_doubleClicked();
    void on_n_w_c_l_doubleClicked();
    void on_immersive_mode_stateChanged(int arg1);

    // 热键触发槽
    void onHotkeyTriggered(const QString &actionName);

    // 窗口捕获槽
    void on_button_start_capture_clicked();
    void on_button_apply_capture_clicked();
    void on_button_open_exe_dir_clicked();

    // 热键配置槽
    void on_button_hk_topmost_clicked(bool checked);
    void on_button_hk_autokill_clicked(bool checked);
    void on_button_hk_autohide_clicked(bool checked);
    void on_button_hk_attach_clicked(bool checked);
    void on_button_hk_capture_clicked(bool checked);
    void on_button_reset_hotkeys_clicked();
    void on_check_attached_handle_stateChanged(int arg1);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintPtr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private slots:
    void onKeyPressed(int key);
};

#endif // MAINWINDOW_H
