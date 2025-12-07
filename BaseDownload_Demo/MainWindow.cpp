#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "DownloadCore/Downloader.h"
#include "DownloadCore/BaseDownload.h"

#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Downloader* downloader = new Downloader(this);
    // connect(downloader, &Downloader::SigDownloadFinish, [](const QString& url, bool ret, const QString& err) {
    //     qDebug() << url << ret << err;
    //     qDebug() << "chunk" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    // });
    // qDebug() << "chunk" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    // downloader->Download("https://hitpaw-vikpeapc-prod.oss-accelerate.aliyuncs.com/vikpea-pc%2Fcloud-preview-ori-video%2F20251202%2Fc6b3dd28-63ec-4467-8d81-c8cbb7d9aa15.mp4", "D:/test.mp4");

    BaseDownload* base1 = new BaseDownload("https://hitpaw-vikpeapc-prod.oss-accelerate.aliyuncs.com/vikpea-pc%2Fcloud-preview-ori-video%2F20251202%2Fc6b3dd28-63ec-4467-8d81-c8cbb7d9aa15.mp4", this);
    connect(base1, &BaseDownload::SigDownloadFinished, [](const QString&, bool, const QString&) {
        qDebug() << "base1" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    });
    qDebug() << "base1" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    base1->Download(0, 6144884);

    BaseDownload* base2 = new BaseDownload("https://hitpaw-vikpeapc-prod.oss-accelerate.aliyuncs.com/vikpea-pc%2Fcloud-preview-ori-video%2F20251202%2Fc6b3dd28-63ec-4467-8d81-c8cbb7d9aa15.mp4", this);
    connect(base2, &BaseDownload::SigDownloadFinished, [](const QString&, bool, const QString&) {
        qDebug() << "base2" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    });
    qDebug() << "base2" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    base2->Download(6144885, 12289769);

    BaseDownload* base3 = new BaseDownload("https://hitpaw-vikpeapc-prod.oss-accelerate.aliyuncs.com/vikpea-pc%2Fcloud-preview-ori-video%2F20251202%2Fc6b3dd28-63ec-4467-8d81-c8cbb7d9aa15.mp4", this);
    connect(base3, &BaseDownload::SigDownloadFinished, [](const QString&, bool, const QString&) {
        qDebug() << "base3" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    });
    qDebug() << "base3" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    base3->Download(12289770, 18434654);

    BaseDownload* base4 = new BaseDownload("https://hitpaw-vikpeapc-prod.oss-accelerate.aliyuncs.com/vikpea-pc%2Fcloud-preview-ori-video%2F20251202%2Fc6b3dd28-63ec-4467-8d81-c8cbb7d9aa15.mp4", this);
    connect(base4, &BaseDownload::SigDownloadFinished, [](const QString&, bool, const QString&) {
        qDebug() << "base4" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    });
    qDebug() << "base4" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    base4->Download(0, 0);
}

MainWindow::~MainWindow()
{
    delete ui;
}
