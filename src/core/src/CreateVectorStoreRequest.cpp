// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateVectorStoreRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateVectorStoreRequestData : public QSharedData
{
public:
    QString name;
    QStringList fileIds;
    QString expiresAfterAnchor;
    int expiresAfterDays = 0;
    QJsonObject chunkingStrategy;
    QJsonObject metadata;
};

CreateVectorStoreRequest::CreateVectorStoreRequest()
    : d(new CreateVectorStoreRequestData)
{ }

CreateVectorStoreRequest::CreateVectorStoreRequest(QString name, QStringList fileIds)
    : d(new CreateVectorStoreRequestData)
{
    d->name = std::move(name);
    d->fileIds = std::move(fileIds);
}

CreateVectorStoreRequest::CreateVectorStoreRequest(const CreateVectorStoreRequest &other) = default;
CreateVectorStoreRequest::CreateVectorStoreRequest(CreateVectorStoreRequest &&other) noexcept
        = default;
CreateVectorStoreRequest &CreateVectorStoreRequest::operator=(const CreateVectorStoreRequest &other)
        = default;
CreateVectorStoreRequest &
CreateVectorStoreRequest::operator=(CreateVectorStoreRequest &&other) noexcept
        = default;
CreateVectorStoreRequest::~CreateVectorStoreRequest() = default;

QString CreateVectorStoreRequest::name() const { return d->name; }
void CreateVectorStoreRequest::setName(const QString &name) { d->name = name; }

QStringList CreateVectorStoreRequest::fileIds() const { return d->fileIds; }
void CreateVectorStoreRequest::setFileIds(const QStringList &fileIds) { d->fileIds = fileIds; }

QString CreateVectorStoreRequest::expiresAfterAnchor() const { return d->expiresAfterAnchor; }
int CreateVectorStoreRequest::expiresAfterDays() const { return d->expiresAfterDays; }

void CreateVectorStoreRequest::setExpiresAfter(const QString &anchor, int days)
{
    d->expiresAfterAnchor = anchor;
    d->expiresAfterDays = days;
}

QJsonObject CreateVectorStoreRequest::chunkingStrategy() const { return d->chunkingStrategy; }
void CreateVectorStoreRequest::setChunkingStrategy(const QJsonObject &chunkingStrategy)
{
    d->chunkingStrategy = chunkingStrategy;
}

QJsonObject CreateVectorStoreRequest::metadata() const { return d->metadata; }
void CreateVectorStoreRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QJsonObject CreateVectorStoreRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    if (!d->fileIds.isEmpty()) {
        QJsonArray fileIds;
        for (const QString &fileId : d->fileIds)
            fileIds.append(fileId);
        json.insert(QStringLiteral("file_ids"), fileIds);
    }
    if (d->expiresAfterDays > 0) {
        QJsonObject expiresAfter;
        detail::insertIfNotEmpty(expiresAfter, QStringLiteral("anchor"), d->expiresAfterAnchor);
        expiresAfter.insert(QStringLiteral("days"), d->expiresAfterDays);
        json.insert(QStringLiteral("expires_after"), expiresAfter);
    }
    if (!d->chunkingStrategy.isEmpty())
        json.insert(QStringLiteral("chunking_strategy"), d->chunkingStrategy);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    return json;
}

bool CreateVectorStoreRequest::operator==(const CreateVectorStoreRequest &other) const
{
    return d->name == other.d->name && d->fileIds == other.d->fileIds
           && d->expiresAfterAnchor == other.d->expiresAfterAnchor
           && d->expiresAfterDays == other.d->expiresAfterDays
           && d->chunkingStrategy == other.d->chunkingStrategy && d->metadata == other.d->metadata;
}

} // namespace Core
} // namespace QtOpenAi
