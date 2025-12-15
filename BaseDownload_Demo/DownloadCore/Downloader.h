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

    void SetTimeout(uint32_t msec);

    bool Download(const QString& url, const QString& path);
    void Stop();

    QString SavePath() const;
    int64_t FileSize() const;
    int64_t FinishedSize() const;

signals:
    void SigDownloadFinish(const QString& url, bool result, const QString& error);
    void SigProgressChanged(const QString& url, double progress, double bps, double time_left);

private:
    Impl impl_;
};

#endif // DOWNLOADER_H
