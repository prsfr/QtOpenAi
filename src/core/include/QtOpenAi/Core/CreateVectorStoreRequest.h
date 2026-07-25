// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class CreateVectorStoreRequestData;

// The body of a POST /vector_stores request, and of the modify call on
// /vector_stores/{id} — the two take the same fields, and unset ones are simply
// omitted, so an update carries only what it changes.
class QTOPENAI_CORE_EXPORT CreateVectorStoreRequest
{
public:
    CreateVectorStoreRequest();
    explicit CreateVectorStoreRequest(QString name, QStringList fileIds = {});
    CreateVectorStoreRequest(const CreateVectorStoreRequest &other);
    CreateVectorStoreRequest(CreateVectorStoreRequest &&other) noexcept;
    CreateVectorStoreRequest &operator=(const CreateVectorStoreRequest &other);
    CreateVectorStoreRequest &operator=(CreateVectorStoreRequest &&other) noexcept;
    ~CreateVectorStoreRequest();

    void swap(CreateVectorStoreRequest &other) noexcept { d.swap(other.d); }

    QString name() const;
    void setName(const QString &name);

    // Ids of already-uploaded files to ingest right away (Files API ids).
    QStringList fileIds() const;
    void setFileIds(const QStringList &fileIds);

    // Optional expiry policy (`expires_after`): an anchor — currently only
    // "last_active_at" — plus a lifetime in days. Omitted while days is 0.
    QString expiresAfterAnchor() const;
    int expiresAfterDays() const;
    void setExpiresAfter(const QString &anchor, int days);

    // How to split the files, kept as raw JSON so new strategies pass through
    // (e.g. {"type": "static", "static": {"max_chunk_size_tokens": 800, ...}}).
    QJsonObject chunkingStrategy() const;
    void setChunkingStrategy(const QJsonObject &chunkingStrategy);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    QJsonObject toJson() const;

    bool operator==(const CreateVectorStoreRequest &other) const;
    bool operator!=(const CreateVectorStoreRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateVectorStoreRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateVectorStoreRequest)
