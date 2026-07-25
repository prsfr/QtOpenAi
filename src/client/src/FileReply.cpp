// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/FileReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class FileReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::FileObject file;
};

FileReply::FileReply(std::function<QNetworkReply *()> requestFactory, RetryPolicy policy,
                     QObject *parent)
    : RestReplyBase(*new FileReplyPrivate, std::move(requestFactory), std::move(policy), parent)
{ }

Core::FileObject FileReply::file() const
{
    Q_D(const FileReply);
    return d->file;
}

bool FileReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(FileReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->file = Core::FileObject::fromJson(object);
    Q_EMIT finished(d->file);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
