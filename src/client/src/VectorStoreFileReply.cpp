// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/VectorStoreFileReply.h"

#include "RestReplyBase_p.h"

namespace QtOpenAi {
namespace Client {

class VectorStoreFileReplyPrivate : public RestReplyBasePrivate
{
public:
    Core::VectorStoreFile file;
};

VectorStoreFileReply::VectorStoreFileReply(std::function<QNetworkReply *()> requestFactory,
                                           RetryPolicy policy, QObject *parent)
    : RestReplyBase(*new VectorStoreFileReplyPrivate, std::move(requestFactory), std::move(policy),
                    parent)
{ }

Core::VectorStoreFile VectorStoreFileReply::file() const
{
    Q_D(const VectorStoreFileReply);
    return d->file;
}

bool VectorStoreFileReply::dispatchSuccess(const QByteArray &body, int httpStatus)
{
    Q_D(VectorStoreFileReply);
    QJsonObject object;
    if (!parseJsonObject(body, httpStatus, object))
        return false;
    d->file = Core::VectorStoreFile::fromJson(object);
    Q_EMIT finished(d->file);
    return true;
}

} // namespace Client
} // namespace QtOpenAi
