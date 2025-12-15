#include <thread>
#include <QFile>
#include <QTimer>
#include <QDateTime>
#include <QStorageInfo>

#include "Downloader.h"
#include "BaseDownload.h"

#define __DOWNLOADER__ "Downloader<=>Module"

struct DownloadChunk {
    int64_t begin_byte;
    int64_t end_byte;
    std::atomic_int64_t finish_byte;
    uint32_t retry;
    bool completed;

    DownloadChunk()
        : begin_byte(0)
        , end_byte(0)
        , finish_byte(0)
        , retry(0)
        , completed(false)
    {}

    DownloadChunk(int64_t v1, int64_t v2, int64_t v3, uint32_t v4, bool v5)
        : begin_byte(v1)
        , end_byte(v2)
        , finish_byte(v3)
        , retry(v4)
        , completed(v5)
    {}

    DownloadChunk(const DownloadChunk& other)
        : begin_byte(other.begin_byte)
        , end_byte(other.end_byte)
        , finish_byte(other.finish_byte.load())
        , retry(other.retry)
        , completed(other.completed)
    {}

    void operator=(DownloadChunk&& other) {
        begin_byte = other.begin_byte;
        end_byte = other.end_byte;
        finish_byte = other.finish_byte.load();
        retry = other.retry;
        completed = other.completed;
    }
};

struct DownloadTask {
    QString url;
    QString path;
    int64_t start_time;
    int64_t file_size;
    std::atomic_int64_t finished_size;
    bool accept_range;
    std::unordered_map<QString, DownloadChunk> chunks;

    std::unique_ptr<QFile> file;
};



class Downloader::DownloaderImpl : public QObject {
    using BaseDownPtr = std::unique_ptr<BaseDownload>;
    using TimerPtr = std::unique_ptr<QTimer>;

public:
    DownloaderImpl(Downloader* q_ptr)
        : q_ptr_(q_ptr)
        , base_down_(std::make_unique<BaseDownload>())
        , t_timer_(std::make_unique<QTimer>())
        , t_msec_(0)
    {
        connect(base_down_.get(), &BaseDownload::SigDownloadInfo, this, [this](int64_t file_size, bool accept_range) {
            InitChunks(file_size, accept_range);
        });
        connect(base_down_.get(), &BaseDownload::SigDownloadFinished, this, [this](const QString& uid, bool result, const QString& error) {
            DownloadFinished(uid, result, error);
        });
        base_down_->SetReadyReadCallback(&DownloaderImpl::ChunkDataReaded, this);

        connect(t_timer_.get(), &QTimer::timeout, this, [this]() {
            EmitFinished(false, "download timeout");
        });
    }

    ~DownloaderImpl() {

    }

    void SetTimeout(uint32_t msec) {
        t_msec_ = msec;
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

        TimeoutMonitor();
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

    QString SavePath() const {
        return task_.path;
    }

    int64_t FileSize() const {
        return task_.file_size;
    }

    int64_t FinishedSize() const {
        return task_.finished_size;
    }

private:
    void TimeoutMonitor() {
        if (t_msec_ != 0) {
            t_timer_->start(t_msec_);
            qDebug() << __DOWNLOADER__ << "start timeout timer" << t_msec_;
        }
    }

    bool InitTask(const QString& url, const QString& path) {
        task_.url = url;
        task_.path = path;
        task_.start_time = QDateTime::currentMSecsSinceEpoch();
        task_.file_size = 0;
        task_.finished_size = 0;
        task_.accept_range = false;
        task_.chunks.clear();

        task_.file = std::make_unique<QFile>(path);
        return task_.file->open(QIODevice::WriteOnly);
    }

    void InitChunks(int64_t file_size, bool accept_range) {
        task_.file_size = file_size;
        task_.accept_range = accept_range;
        qDebug() << __DOWNLOADER__ << "init chuns" << file_size << accept_range;

        if (!StorageEnough()) {
            EmitFinished(false, "lack of space");
            return;
        }

        int64_t chunk_count = 1, chunk_size = 0;
        if (file_size != 0 && accept_range) {
            chunk_count = AutoChunkCount(file_size);
            chunk_size = file_size / chunk_count;
        }

        for (int i = 0; i < chunk_count; ++i) {
            DownloadChunk chunk {
                i * chunk_size,
                (i == chunk_count - 1) ? file_size - 1 : (i + 1) * chunk_size - 1,
                0,
                0,
                false
            };

            QString uid = base_down_->Download(chunk.begin_byte, chunk.end_byte);
            task_.chunks[uid] = std::move(chunk);

            qDebug() << __DOWNLOADER__ << "chunk info" << i << chunk.begin_byte << chunk.end_byte << uid;
        }
    }

    void ChunkDataReaded(const QString& uid, QByteArray&& bytes) {
        if (!task_.file->isOpen()) {
            return;
        }

        auto& chunk = task_.chunks[uid];
        task_.file->seek(chunk.begin_byte + chunk.finish_byte);
        int64_t write_size = task_.file->write(bytes);
        if (write_size > 0) {
            chunk.finish_byte += write_size;
            task_.finished_size += write_size;
            EmitProgress();
        }
    }

    void DownloadFinished(const QString& uid, bool result, const QString& error) {
        if (!result) {
            qDebug() << __DOWNLOADER__ << "uid:" << uid << "download fail:" << error;
            DownloadChunk chunk = task_.chunks[uid];
            if (chunk.retry < 1) {
                ++chunk.retry;
                task_.chunks.erase(uid);
                QString new_uid = base_down_->Download(chunk.begin_byte, chunk.end_byte);
                task_.chunks[new_uid] = std::move(chunk);
                qDebug() << __DOWNLOADER__ << "uid:" << uid << "retry => new uid:" << new_uid;
            }
            else {
                qDebug() << __DOWNLOADER__ << "uid:" << uid << "all retry fail, end downloading...";
                EmitFinished(false, error);
            }
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
        EmitFinished(true, "");
    }

    int64_t AutoChunkCount(int64_t file_size) const {
        if (file_size <= 0) {
            return 1;
        }

        constexpr double fsize_weight = 0.75;
        constexpr double cpu_weight = 0.25;

        double fsize_score = 0.0;
        if (file_size < 1 * 1024 * 1024) {
            fsize_score = 0.0;
        }
        else if (file_size < 5 * 1024 * 1024) {  // <5MB
            fsize_score = 0.25;
        }
        else if (file_size < 50 * 1024 * 1024) {  // 5-50MB
            fsize_score = 0.3;
        }
        else if (file_size < 200 * 1024 * 1024) {  // 50-200MB
            fsize_score = 0.6;
        }
        else if (file_size < 500 * 1024 * 1024) {  // 200-500MB
            fsize_score = 0.7;
        }
        else if (file_size < 1024 * 1024 * 1024) {  // 500MB-1GB
            fsize_score = 0.8;
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

    bool StorageEnough() const {
        QFileInfo file(task_.path);
        QStorageInfo storage(file.absolutePath());
        return storage.bytesAvailable() >= (task_.file_size * 3);
    }

    void EmitFinished(bool result, const QString& error) {
        t_timer_->stop();
        if (!result) {
            base_down_->Clear();
        }
        task_.file->close();
        task_.file.reset();
        if (!result) {
            QFile::remove(task_.path);
        }
        emit q_ptr_->SigDownloadFinish(task_.url, result, error);
        qDebug() << __DOWNLOADER__ << "download finished" << task_.path << result << error;
    }

    void EmitProgress() {
        int64_t spc_time = QDateTime::currentMSecsSinceEpoch() - task_.start_time;
        if (spc_time == 0) {
            return;
        }

        double bps = task_.finished_size / spc_time * 1000.0;
        double time_left = (task_.file_size - task_.finished_size) / bps;

        if (task_.file_size == 0) {
            double fsize = 500.0 * 1024 * 1024;
            if (task_.finished_size >= fsize) {
                emit q_ptr_->SigProgressChanged(task_.url, 99.99, bps, 0.0);
            }
            else {
                emit q_ptr_->SigProgressChanged(task_.url, task_.finished_size / fsize, bps, time_left);
            }
        }
        else {
            emit q_ptr_->SigProgressChanged(task_.url, task_.finished_size * 1.0 / task_.file_size, bps, time_left);
        }
    }

private:
    Downloader* q_ptr_;
    BaseDownPtr base_down_;
    DownloadTask task_;

    TimerPtr t_timer_;
    uint32_t t_msec_;
};



Downloader::Downloader(QObject *parent)
    : QObject{parent}
    , impl_(std::make_unique<DownloaderImpl>(this))
{}

Downloader::~Downloader() {

}

void Downloader::SetTimeout(uint32_t msec) {
    impl_->SetTimeout(msec);
}

bool Downloader::Download(const QString &url, const QString &path) {
    return impl_->Download(url, path);
}

void Downloader::Stop() {
    impl_->Stop();
}

QString Downloader::SavePath() const {
    return impl_->SavePath();
}

int64_t Downloader::FileSize() const {
    return impl_->FileSize();
}

int64_t Downloader::FinishedSize() const {
    return impl_->FinishedSize();
}
