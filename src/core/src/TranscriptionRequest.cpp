// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/TranscriptionRequest.h"

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
    QList<FormField> fields;
    fields.append({QStringLiteral("model"), d->model});
    if (!d->language.isEmpty())
        fields.append({QStringLiteral("language"), d->language});
    if (!d->prompt.isEmpty())
        fields.append({QStringLiteral("prompt"), d->prompt});
    if (!d->responseFormat.isEmpty())
        fields.append({QStringLiteral("response_format"), d->responseFormat});
    if (d->temperature)
        fields.append({QStringLiteral("temperature"), QString::number(*d->temperature)});
    // Array fields are repeated with the `name[]` convention.
    for (const QString &granularity : d->timestampGranularities)
        fields.append({QStringLiteral("timestamp_granularities[]"), granularity});
    for (const QString &item : d->include)
        fields.append({QStringLiteral("include[]"), item});
    if (d->stream)
        fields.append({QStringLiteral("stream"),
                       *d->stream ? QStringLiteral("true") : QStringLiteral("false")});
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
