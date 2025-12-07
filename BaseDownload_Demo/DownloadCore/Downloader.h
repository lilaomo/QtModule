#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <memory>
#include <QObject>

class Downloader : public QObject
{
    Q_OBJECT
    class DownloaderImpl;
    using Impl = std::unique_ptr<DownloaderImpl>;

public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader();

    bool Download(const QString& url, const QString& path);
    void Stop();

signals:
    void SigDownloadFinish(const QString& url, bool result, const QString& error);

private:
    Impl impl_;
};

#endif // DOWNLOADER_H
