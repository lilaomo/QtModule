#include <thread>
#include <QFile>

#include "Downloader.h"
#include "BaseDownload.h"

#define __DOWNLOADER__ "Downloader<=>Module"

struct DownloadChunk {
    QString uid;
    int64_t begin_byte;
    int64_t end_byte;
    int64_t finish_byte;
    bool completed;
};

struct DownloadTask {
    QString url;
    QString path;
    int64_t file_size;
    std::atomic_int64_t finished_size;
    bool accept_range;
    std::unordered_map<QString, DownloadChunk> chunks;

    std::unique_ptr<QFile> file;
};



class Downloader::DownloaderImpl : public QObject {
    using BaseDownPtr = std::unique_ptr<BaseDownload>;

public:
    DownloaderImpl(Downloader* q_ptr)
        : q_ptr_(q_ptr)
        , base_down_(std::make_unique<BaseDownload>())
    {
        connect(base_down_.get(), &BaseDownload::SigDownloadInfo, this, [this](int64_t file_size, bool accept_range) {
            InitChunks(file_size, accept_range);
        });
        connect(base_down_.get(), &BaseDownload::SigDownloadFinished, this, [this](const QString& uid, bool result, const QString& error) {
            DownloadFinished(uid, result, error);
        });
        base_down_->SetReadyReadCallback(&DownloaderImpl::ChunkDataReaded, this);
    }

    ~DownloaderImpl() {

    }

    bool Download(const QString& url, const QString& path) {
        if (task_.file) {
            qDebug() << __DOWNLOADER__ << "is downloading now";
            return false;
        }

        if (!InitTask(url, path)) {
            qDebug() << __DOWNLOADER__ << "create file error";
            return false;
        }

        base_down_->SetDownloadUrl(url);
        base_down_->ReqDownloadInfo();
        qDebug() << __DOWNLOADER__ << "start download" << url << path;

        return true;
    }

    void Stop() {
        base_down_->Clear();
        if (task_.file) {
            task_.file->close();
            task_.file.reset();
            QFile::remove(task_.path);
        }
    }

private:
    bool InitTask(const QString& url, const QString& path) {
        task_.url = url;
        task_.path = path;
        task_.file_size = 0;
        task_.finished_size = 0;
        task_.accept_range = false;
        task_.chunks.clear();

        task_.file = std::make_unique<QFile>(path);
        return task_.file->open(QIODevice::WriteOnly);
    }

    void InitChunks(int64_t file_size, bool accept_range) {
        int64_t chunk_count = 1, chunk_size = 0;
        if (file_size != 0 && accept_range) {
            chunk_count = AutoChunkCount(file_size);
            chunk_size = file_size / chunk_count;
        }

        for (int i = 0; i < chunk_count; ++i) {
            DownloadChunk chunk {
                "",
                i * chunk_size,
                (i == chunk_count - 1) ? file_size - 1 : (i + 1) * chunk_size - 1,
                0,
                false
            };

            QString uid = base_down_->Download(chunk.begin_byte, chunk.end_byte);
            task_.chunks[uid] = std::move(chunk);

            qDebug() << __DOWNLOADER__ << "chunk info" << chunk.begin_byte << chunk.end_byte;
        }
    }

    void ChunkDataReaded(const QString& uid, QByteArray&& bytes) {
        if (!task_.file->isOpen()) {
            return;
        }

        qDebug() << uid << "read" << bytes.size();

        auto& chunk = task_.chunks[uid];
        task_.file->seek(chunk.begin_byte + chunk.finish_byte);
        int64_t write_size = task_.file->write(bytes);
        qDebug() << uid << "write" << write_size;
        if (write_size > 0) {
            chunk.finish_byte += write_size;
            task_.finished_size += write_size;
        }
    }

    void DownloadFinished(const QString& uid, bool result, const QString& error) {
        if (!result) {
            qDebug() << __DOWNLOADER__ << "uid:" << uid << "download fail:" << error;
            DownloadChunk chunk = task_.chunks[uid];
            task_.chunks.erase(uid);

            QString uid = base_down_->Download(chunk.begin_byte, chunk.end_byte);
            task_.chunks[uid] = std::move(chunk);
        }
        else {
            qDebug() << "finished" << uid << task_.chunks[uid].finish_byte;
            task_.chunks[uid].completed = true;
            CheckAllCompleted();
        }
    }

    void CheckAllCompleted() {
        for (const auto& iter : task_.chunks) {
            if (!iter.second.completed) {
                return;
            }
        }

        task_.file->close();
        task_.file.reset();
        emit q_ptr_->SigDownloadFinish(task_.url, true, "");
    }

    int64_t AutoChunkCount(int64_t file_size) const {
        if (file_size <= 0) {
            return 1;
        }

        constexpr double fsize_weight = 0.75;
        constexpr double cpu_weight = 0.25;

        double fsize_score = 0.0;
        if (file_size < 2 * 1024 * 1024) {
            fsize_score = 0.0;
        }
        else if (file_size < 5 * 1024 * 1024) {  // <5MB
            fsize_score = 0.2;
        }
        else if (file_size < 50 * 1024 * 1024) {  // 5-50MB
            fsize_score = 0.3;
        }
        else if (file_size < 200 * 1024 * 1024) {  // 50-200MB
            fsize_score = 0.5;
        }
        else if (file_size < 1024 * 1024 * 1024) {  // 200MB-1GB
            fsize_score = 0.7;
        }
        else {  // >1GB
            fsize_score = 1.0;
        }

        double cpu_score = qMin(std::thread::hardware_concurrency() / 8.0, 1.0);

        double total_score = fsize_score * fsize_weight + cpu_score * cpu_weight;
        if (total_score < 0.3) {
            return 1;
        }
        else if (total_score < 0.4) {
            return 2;
        }
        else if (total_score < 0.5) {
            return 3;
        }
        else if (total_score < 0.6) {
            return 4;
        }
        else if (total_score < 0.7) {
            return 5;
        }
        else if (total_score < 0.8) {
            return 6;
        }
        else if (total_score < 0.9) {
            return 7;
        }
        else {
            return 8;
        }
    }

private:
    Downloader* q_ptr_;
    BaseDownPtr base_down_;
    DownloadTask task_;
};



Downloader::Downloader(QObject *parent)
    : QObject{parent}
    , impl_(std::make_unique<DownloaderImpl>(this))
{}

Downloader::~Downloader() {

}

bool Downloader::Download(const QString &url, const QString &path) {
    return impl_->Download(url, path);
}

void Downloader::Stop() {
    impl_->Stop();
}
