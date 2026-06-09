#include "AboutPage.h"
#include "ui_AboutPage.h"
#include <QPixmap>
#include <QIcon>

AboutPage::AboutPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutPage)
{
    ui->setupUi(this);

    // 确保图标正确加载
    ui->bilibiliLink->setIcon(QIcon(":/Resources/bilibili-text.png"));
    ui->officialLink->setIcon(QIcon(":/Resources/WorldMargin.png"));
    ui->githubLink->setIcon(QIcon(":/Resources/github.png"));

    // 设置按钮图标大小
    QSize iconSize(24, 24); // 设置图标大小为24x24像素
    ui->bilibiliLink->setIconSize(iconSize);
    ui->officialLink->setIconSize(iconSize);
    ui->githubLink->setIconSize(iconSize);

    // 连接按钮点击信号到槽函数
    connect(ui->githubLink, &QPushButton::clicked, this, &AboutPage::on_githubLink_clicked);
    connect(ui->officialLink, &QPushButton::clicked, this, &AboutPage::on_officialLink_clicked);
    connect(ui->bilibiliLink, &QPushButton::clicked, this, &AboutPage::on_bilibiliLink_clicked);

    // 设置对话框的最小尺寸以确保布局正确显示
    this->setMinimumSize(400, 300);
    this->setMaximumSize(800, 600);
}

AboutPage::~AboutPage()
{
    delete ui;
}

void AboutPage::on_githubLink_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/WorldMargin2/FuckRedSpider"));
}

void AboutPage::on_officialLink_clicked()
{
    QDesktopServices::openUrl(QUrl("http://worldmargin.top"));
}

void AboutPage::on_bilibiliLink_clicked()
{
    QDesktopServices::openUrl(QUrl("https://space.bilibili.com/3546734785464807"));
}
