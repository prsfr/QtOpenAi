// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateBatchRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {
// The only completion window the API accepts today.
const QString kDefaultCompletionWindow = QStringLiteral("24h");
} // namespace

class CreateBatchRequestData : public QSharedData
{
public:
    QString inputFileId;
    QString endpoint;
    QString completionWindow = kDefaultCompletionWindow;
    QJsonObject metadata;
    QString outputExpiresAfterAnchor;
    qint64 outputExpiresAfterSeconds = 0;
};

CreateBatchRequest::CreateBatchRequest()
    : d(new CreateBatchRequestData)
{ }

CreateBatchRequest::CreateBatchRequest(QString inputFileId, QString endpoint)
    : d(new CreateBatchRequestData)
{
    d->inputFileId = std::move(inputFileId);
    d->endpoint = std::move(endpoint);
}

CreateBatchRequest::CreateBatchRequest(const CreateBatchRequest &other) = default;
CreateBatchRequest::CreateBatchRequest(CreateBatchRequest &&other) noexcept = default;
CreateBatchRequest &CreateBatchRequest::operator=(const CreateBatchRequest &other) = default;
CreateBatchRequest &CreateBatchRequest::operator=(CreateBatchRequest &&other) noexcept = default;
CreateBatchRequest::~CreateBatchRequest() = default;

QString CreateBatchRequest::inputFileId() const { return d->inputFileId; }
void CreateBatchRequest::setInputFileId(const QString &inputFileId)
{
    d->inputFileId = inputFileId;
}

QString CreateBatchRequest::endpoint() const { return d->endpoint; }
void CreateBatchRequest::setEndpoint(const QString &endpoint) { d->endpoint = endpoint; }

QString CreateBatchRequest::completionWindow() const { return d->completionWindow; }
void CreateBatchRequest::setCompletionWindow(const QString &completionWindow)
{
    d->completionWindow = completionWindow;
}

QJsonObject CreateBatchRequest::metadata() const { return d->metadata; }
void CreateBatchRequest::setMetadata(const QJsonObject &metadata) { d->metadata = metadata; }

QString CreateBatchRequest::outputExpiresAfterAnchor() const { return d->outputExpiresAfterAnchor; }

qint64 CreateBatchRequest::outputExpiresAfterSeconds() const
{
    return d->outputExpiresAfterSeconds;
}

void CreateBatchRequest::setOutputExpiresAfter(const QString &anchor, qint64 seconds)
{
    d->outputExpiresAfterAnchor = anchor;
    d->outputExpiresAfterSeconds = seconds;
}

QJsonObject CreateBatchRequest::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("input_file_id"), d->inputFileId);
    detail::insertIfNotEmpty(json, QStringLiteral("endpoint"), d->endpoint);
    detail::insertIfNotEmpty(json, QStringLiteral("completion_window"), d->completionWindow);
    if (!d->metadata.isEmpty())
        json.insert(QStringLiteral("metadata"), d->metadata);
    if (d->outputExpiresAfterSeconds > 0) {
        QJsonObject expiresAfter;
        detail::insertIfNotEmpty(expiresAfter, QStringLiteral("anchor"),
                                 d->outputExpiresAfterAnchor);
        expiresAfter.insert(QStringLiteral("seconds"), d->outputExpiresAfterSeconds);
        json.insert(QStringLiteral("output_expires_after"), expiresAfter);
    }
    return json;
}

bool CreateBatchRequest::operator==(const CreateBatchRequest &other) const
{
    return d->inputFileId == other.d->inputFileId && d->endpoint == other.d->endpoint
           && d->completionWindow == other.d->completionWindow && d->metadata == other.d->metadata
           && d->outputExpiresAfterAnchor == other.d->outputExpiresAfterAnchor
           && d->outputExpiresAfterSeconds == other.d->outputExpiresAfterSeconds;
}

} // namespace Core
} // namespace QtOpenAi
