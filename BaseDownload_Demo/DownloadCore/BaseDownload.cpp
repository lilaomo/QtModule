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

    refle_[reply] = "main_reply";
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
    refle_[reply] = uid;

    return uid;
}



void BaseDownload::StopRequest() {
    StopDownload("main_reply");
}



void BaseDownload::StopDownload(const QString &uid) {
    for (auto iter = refle_.begin(); iter != refle_.end(); ++iter) {
        if (iter->second != uid) {
            continue;
        }

        iter->first->abort();
        iter->first->deleteLater();
        iter = refle_.erase(iter);
        return;
    }
}



void BaseDownload::Clear() {
    for (auto& iter : refle_) {
        iter.first->abort();
        iter.first->deleteLater();
    }
    refle_.clear();
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
        cb_(iter->second, iter->first->readAll());
    }
}



void BaseDownload::SlotDownloadFinished() {
    auto iter = refle_.find(qobject_cast<QNetworkReply*>(sender()));
    if (iter == refle_.end()) {
        qDebug() << __FUNCTION__ << "reply error";
        emit SigReplyError();
        return;
    }

    iter->first->deleteLater();
    QString uid = std::move(iter->second);
    bool result = iter->first->error() == QNetworkReply::NoError;
    QString error =  iter->first->errorString();
    refle_.erase(iter);

    emit SigDownloadFinished(uid, result, error);
}



QString BaseDownload::CreateUid() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
