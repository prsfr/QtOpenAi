// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateBatchRequestData;

// The body of a POST /batches request.
//
// The input file is a JSONL file uploaded through the Files API with purpose
// "batch", one request per line, all targeting the same `endpoint`.
class QTOPENAI_CORE_EXPORT CreateBatchRequest
{
public:
    CreateBatchRequest();
    CreateBatchRequest(QString inputFileId, QString endpoint);
    CreateBatchRequest(const CreateBatchRequest &other);
    CreateBatchRequest(CreateBatchRequest &&other) noexcept;
    CreateBatchRequest &operator=(const CreateBatchRequest &other);
    CreateBatchRequest &operator=(CreateBatchRequest &&other) noexcept;
    ~CreateBatchRequest();

    void swap(CreateBatchRequest &other) noexcept { d.swap(other.d); }

    QString inputFileId() const;
    void setInputFileId(const QString &inputFileId);

    // The API route every line of the input file targets, e.g.
    // "/v1/chat/completions" or "/v1/embeddings".
    QString endpoint() const;
    void setEndpoint(const QString &endpoint);

    // How long the batch may take; "24h" is the only value the API accepts
    // today and therefore the default.
    QString completionWindow() const;
    void setCompletionWindow(const QString &completionWindow);

    QJsonObject metadata() const;
    void setMetadata(const QJsonObject &metadata);

    // Optional expiry for the generated output/error files
    // (`output_expires_after`): an anchor — currently only "created_at" — plus a
    // lifetime in seconds. Omitted while seconds is 0.
    QString outputExpiresAfterAnchor() const;
    qint64 outputExpiresAfterSeconds() const;
    void setOutputExpiresAfter(const QString &anchor, qint64 seconds);

    QJsonObject toJson() const;

    bool operator==(const CreateBatchRequest &other) const;
    bool operator!=(const CreateBatchRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateBatchRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateBatchRequest)
