// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FileListReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FileListReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FileList list;
};

FileListReply::FileListReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                             QObject *parent)
    : RestReplyBase(*new FileListReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::FileList FileListReply::list() const
{
    Q_D(const FileListReply);
    return d->list;
}

bool FileListReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FileListReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->list = Core::FileList::fromJson(object);
    Q_EMIT finished(d->list);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
