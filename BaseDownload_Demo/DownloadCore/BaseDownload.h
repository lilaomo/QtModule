#ifndef BASEDOWNLOAD_H
#define BASEDOWNLOAD_H

#include <memory>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BaseDownload : public QObject
{
    Q_OBJECT
    struct ReplyInfo {
        QString uid;
        bool is_stop;
    };
    using NetPtr = std::unique_ptr<QNetworkAccessManager>;
    using ReplyReflect = std::unordered_map<QNetworkReply*, ReplyInfo>;
    using ReadyReadCb = std::function<void(const QString&, QByteArray&&)>;

public:
    explicit BaseDownload(QObject *parent = nullptr);
    explicit BaseDownload(const QString& url, QObject *parent = nullptr);
    ~BaseDownload();

    void SetDownloadUrl(const QString& url);

    template<typename Function>
    void SetReadyReadCallback(Function&& func) {
        cb_ = std::forward<Function>(func);
    }
    template<typename RetType, typename ClassType, typename Object>
    std::enable_if_t<std::is_same_v<ClassType*, Object>>
    SetReadyReadCallback(RetType ClassType::* mem_func, Object obj) {
        using namespace std::placeholders;
        cb_ = std::bind(mem_func, obj, _1, _2);
    }

    void ReqDownloadInfo();
    QString Download();
    QString Download(int64_t begin, int64_t end);

    void StopRequest();
    void StopDownload(const QString& uid);
    void Clear();

signals:
    void SigDownloadInfo(int64_t file_size, bool accept_range);
    void SigDownloadFinished(const QString& uid, bool result, const QString& error);
    void SigReplyError();

private slots:
    void SlotDownloadInfo();
    void SlotReadyRead();
    void SlotDownloadFinished();

private:
    QString CreateUid() const;
    void AbortReply(QNetworkReply* reply, ReplyInfo& info);

private:
    NetPtr net_mng_;
    QString url_;
    ReadyReadCb cb_;
    ReplyReflect refle_;
};

#endif // BASEDOWNLOAD_H
