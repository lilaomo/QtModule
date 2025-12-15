#include <QUuid>
#include "BaseDownload.h"

BaseDownload::BaseDownload(QObject *parent)
    : QObject{parent}
    , net_mng_(std::make_unique<QNetworkAccessManager>())
{

}

BaseDownload::BaseDownload(const QString &url, QObject *parent)
    : QObject{parent}
    , net_mng_(std::make_unique<QNetworkAccessManager>())
    , url_(url)
{

}

BaseDownload::~BaseDownload() {
    Clear();
}



void BaseDownload::SetDownloadUrl(const QString &url) {
    url_ = url;
}



void BaseDownload::ReqDownloadInfo() {
    QNetworkRequest request(QUrl{ url_ });
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    QNetworkReply* reply = net_mng_->head(request);
    connect(reply, &QNetworkReply::finished, this, &BaseDownload::SlotDownloadInfo);

    refle_[reply] = { "main_reply", false };
}



QString BaseDownload::Download() {
    return Download(0, 0);
}



QString BaseDownload::Download(int64_t begin, int64_t end) {
    QNetworkRequest request(QUrl{ url_ });
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    if (end > 0) {
        QString range = QString("bytes=%1-%2").arg(begin).arg(end);
        request.setRawHeader("Range", range.toUtf8());
    }

    QNetworkReply* reply = net_mng_->get(request);
    connect(reply, &QNetworkReply::readyRead, this, &BaseDownload::SlotReadyRead);
    connect(reply, &QNetworkReply::finished, this, &BaseDownload::SlotDownloadFinished);

    QString uid = CreateUid();
    refle_[reply] = { uid, false };

    return uid;
}



void BaseDownload::StopRequest() {
    StopDownload("main_reply");
}



void BaseDownload::StopDownload(const QString &uid) {
    for (auto iter = refle_.begin(); iter != refle_.end(); ++iter) {
        if (iter->second.uid != uid) {
            continue;
        }
        if (!iter->second.is_stop) {
            AbortReply(iter->first, iter->second);
        }
        return;
    }
}



void BaseDownload::Clear() {
    for (auto& [reply, info] : refle_) {
        if (!info.is_stop) {
            AbortReply(reply, info);
        }
    }
}



void BaseDownload::SlotDownloadInfo() {
    auto iter = refle_.find(qobject_cast<QNetworkReply*>(sender()));
    if (iter == refle_.end()) {
        qDebug() << __FUNCTION__ << "reply error";
        emit SigReplyError();
        return;
    }

    QNetworkReply* reply = iter->first;
    refle_.erase(iter);
    reply->deleteLater();

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        qDebug() << __FUNCTION__ << "active trigger stop";
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << __FUNCTION__ << "not allowed head:" << reply->errorString();
        emit SigDownloadInfo(0, false);
        return;
    }

    // 获取文件大小
    QVariant content_len = reply->header(QNetworkRequest::ContentLengthHeader);
    int64_t file_size = content_len.isValid() ? content_len.toLongLong() : 0;

    // 检查是否支持断点续传
    QByteArray accept_range = reply->rawHeader("Accept-Ranges");
    bool is_support = accept_range.contains("bytes");

    emit SigDownloadInfo(file_size, is_support);
}



void BaseDownload::SlotReadyRead() {
    auto iter = refle_.find(qobject_cast<QNetworkReply*>(sender()));
    if (iter == refle_.end()) {
        qDebug() << __FUNCTION__ << "reply error";
        emit SigReplyError();
        return;
    }

    if (cb_) {
        cb_(iter->second.uid, iter->first->readAll());
    }
}



void BaseDownload::SlotDownloadFinished() {
    auto iter = refle_.find(qobject_cast<QNetworkReply*>(sender()));
    if (iter == refle_.end()) {
        qDebug() << __FUNCTION__ << "reply error";
        emit SigReplyError();
        return;
    }

    QNetworkReply* reply = iter->first;
    QString uid = std::move(iter->second.uid);

    refle_.erase(iter);
    reply->deleteLater();

    QNetworkReply::NetworkError err = reply->error();
    if (err == QNetworkReply::OperationCanceledError) {
        qDebug() << __FUNCTION__ << "active trigger stop";
        return;
    }
    emit SigDownloadFinished(uid, err == QNetworkReply::NoError, reply->errorString());
}



QString BaseDownload::CreateUid() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}



void BaseDownload::AbortReply(QNetworkReply *reply, ReplyInfo& info) {
    info.is_stop = true;
    reply->abort();
}
