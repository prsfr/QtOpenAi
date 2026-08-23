// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/TranscriptionRequest.h"

#include "FormFields_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class TranscriptionRequestData : public QSharedData
{
public:
    QByteArray fileData;
    QString fileName;
    QString model;
    QString language;
    QString prompt;
    QString responseFormat;
    std::optional<double> temperature;
    QStringList timestampGranularities;
    QStringList include;
    std::optional<bool> stream;
};

TranscriptionRequest::TranscriptionRequest()
    : d(new TranscriptionRequestData)
{ }

TranscriptionRequest::TranscriptionRequest(QByteArray fileData, QString fileName, QString model)
    : d(new TranscriptionRequestData)
{
    d->fileData = std::move(fileData);
    d->fileName = std::move(fileName);
    d->model = std::move(model);
}

TranscriptionRequest::TranscriptionRequest(const TranscriptionRequest &other) = default;
TranscriptionRequest::TranscriptionRequest(TranscriptionRequest &&other) noexcept = default;
TranscriptionRequest &TranscriptionRequest::operator=(const TranscriptionRequest &other) = default;
TranscriptionRequest &TranscriptionRequest::operator=(TranscriptionRequest &&other) noexcept
        = default;
TranscriptionRequest::~TranscriptionRequest() = default;

QByteArray TranscriptionRequest::fileData() const { return d->fileData; }
void TranscriptionRequest::setFileData(const QByteArray &fileData) { d->fileData = fileData; }

QString TranscriptionRequest::fileName() const { return d->fileName; }
void TranscriptionRequest::setFileName(const QString &fileName) { d->fileName = fileName; }

QString TranscriptionRequest::model() const { return d->model; }
void TranscriptionRequest::setModel(const QString &model) { d->model = model; }

QString TranscriptionRequest::language() const { return d->language; }
void TranscriptionRequest::setLanguage(const QString &language) { d->language = language; }

QString TranscriptionRequest::prompt() const { return d->prompt; }
void TranscriptionRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString TranscriptionRequest::responseFormat() const { return d->responseFormat; }
void TranscriptionRequest::setResponseFormat(const QString &format) { d->responseFormat = format; }

std::optional<double> TranscriptionRequest::temperature() const { return d->temperature; }
void TranscriptionRequest::setTemperature(double temperature) { d->temperature = temperature; }

QStringList TranscriptionRequest::timestampGranularities() const
{
    return d->timestampGranularities;
}
void TranscriptionRequest::setTimestampGranularities(const QStringList &granularities)
{
    d->timestampGranularities = granularities;
}

QStringList TranscriptionRequest::include() const { return d->include; }
void TranscriptionRequest::setInclude(const QStringList &include) { d->include = include; }

std::optional<bool> TranscriptionRequest::stream() const { return d->stream; }
void TranscriptionRequest::setStream(bool stream) { d->stream = stream; }

QList<TranscriptionRequest::FormField> TranscriptionRequest::formFields() const
{
    detail::FormFields fields;
    fields.append({QStringLiteral("model"), d->model});
    detail::appendIfNotEmpty(fields, QStringLiteral("language"), d->language);
    detail::appendIfNotEmpty(fields, QStringLiteral("prompt"), d->prompt);
    detail::appendIfNotEmpty(fields, QStringLiteral("response_format"), d->responseFormat);
    detail::appendIfSet(fields, QStringLiteral("temperature"), d->temperature);
    detail::appendEach(fields, QStringLiteral("timestamp_granularities[]"),
                       d->timestampGranularities);
    detail::appendEach(fields, QStringLiteral("include[]"), d->include);
    detail::appendIfSet(fields, QStringLiteral("stream"), d->stream);
    return fields;
}

bool TranscriptionRequest::operator==(const TranscriptionRequest &other) const
{
    return d->fileData == other.d->fileData && d->fileName == other.d->fileName
           && d->model == other.d->model && d->language == other.d->language
           && d->prompt == other.d->prompt && d->responseFormat == other.d->responseFormat
           && d->temperature == other.d->temperature
           && d->timestampGranularities == other.d->timestampGranularities
           && d->include == other.d->include && d->stream == other.d->stream;
}

} // namespace Core
} // namespace QtOpenAi
