#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QProcess>
#include <QLabel>
#include <QResizeEvent>
#include <QList>
#include "GlobalKeyboardHookGuard.h"
#include "AboutPage.h"
#include "WindowsApi.h"
#include "Log.h"

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
    static constexpr int ERROR_LOG_THRESHOLD = 100;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    GlobalKeyboardHookGuard *keyboardGuard;
    bool keyboardGuardRunning;
    AboutPage *aboutPage;
    QTimer *listenerTimer;
    QString thisProcessName;
    QString targetProcessName;
    QString fullWindowClassName;
    QString normalWindowClassName;
    QStringList lastPids;
    bool attachedTargetActive;
    QWidget *targetPanel;
    Qt::WindowStates lastWindowState;
    HWND targetWindow;
    Log *logger;
    static QString origin_executeable_path;

    void setupConnections();
    void setupTimers();
    void updateProcessStatus();
    void killTargetProcesses();
    void hideTargetWindows();
    void attachTargetWindow();
    void validateProcessName(const QString &processName);
    void closeHandle(HWND h);
    void resizeEvent(QResizeEvent *event) override;
    void onResizeEnd();

    void startBackgroundTasks();
    void stopBackgroundTasks();
    bool isBackgroundTaskRunning() const;

private slots:
    void onKeyPressed(int key);
    void on_button_about_clicked();
    void on_check_keep_topest_stateChanged(int arg1);
    void on_auto_kill_stateChanged(int arg1);
    void on_auto_hide_stateChanged(int arg1);
    void on_attached_target_stateChanged(int arg1);
    void on_process_name_textChanged();
    void on_label2_doubleClicked();
    void on_f_w_c_l_doubleClicked();
    void on_n_w_c_l_doubleClicked();
    void listen();
};
#endif // MAINWINDOW_H
