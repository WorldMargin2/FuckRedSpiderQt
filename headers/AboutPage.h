#ifndef AboutPage_H
#define AboutPage_H

#include <QDialog>
#include <QDesktopServices>
#include <QUrl>

namespace Ui {
class AboutPage;
}

class AboutPage : public QDialog
{
    Q_OBJECT

public:
    explicit AboutPage(QWidget *parent = nullptr);
    ~AboutPage();

private slots:
    void on_githubLink_clicked();
    void on_officialLink_clicked();
    void on_bilibiliLink_clicked();

private:
    Ui::AboutPage *ui;
};

#endif // AboutPage_H
