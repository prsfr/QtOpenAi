// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/TranslationRequest.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class TranslationRequestData : public QSharedData
{
public:
    QByteArray fileData;
    QString fileName;
    QString model;
    QString prompt;
    QString responseFormat;
    std::optional<double> temperature;
};

TranslationRequest::TranslationRequest()
    : d(new TranslationRequestData)
{ }

TranslationRequest::TranslationRequest(QByteArray fileData, QString fileName, QString model)
    : d(new TranslationRequestData)
{
    d->fileData = std::move(fileData);
    d->fileName = std::move(fileName);
    d->model = std::move(model);
}

TranslationRequest::TranslationRequest(const TranslationRequest &other) = default;
TranslationRequest::TranslationRequest(TranslationRequest &&other) noexcept = default;
TranslationRequest &TranslationRequest::operator=(const TranslationRequest &other) = default;
TranslationRequest &TranslationRequest::operator=(TranslationRequest &&other) noexcept = default;
TranslationRequest::~TranslationRequest() = default;

QByteArray TranslationRequest::fileData() const { return d->fileData; }
void TranslationRequest::setFileData(const QByteArray &fileData) { d->fileData = fileData; }

QString TranslationRequest::fileName() const { return d->fileName; }
void TranslationRequest::setFileName(const QString &fileName) { d->fileName = fileName; }

QString TranslationRequest::model() const { return d->model; }
void TranslationRequest::setModel(const QString &model) { d->model = model; }

QString TranslationRequest::prompt() const { return d->prompt; }
void TranslationRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString TranslationRequest::responseFormat() const { return d->responseFormat; }
void TranslationRequest::setResponseFormat(const QString &format) { d->responseFormat = format; }

std::optional<double> TranslationRequest::temperature() const { return d->temperature; }
void TranslationRequest::setTemperature(double temperature) { d->temperature = temperature; }

QList<TranslationRequest::FormField> TranslationRequest::formFields() const
{
    QList<FormField> fields;
    fields.append({QStringLiteral("model"), d->model});
    if (!d->prompt.isEmpty())
        fields.append({QStringLiteral("prompt"), d->prompt});
    if (!d->responseFormat.isEmpty())
        fields.append({QStringLiteral("response_format"), d->responseFormat});
    if (d->temperature)
        fields.append({QStringLiteral("temperature"), QString::number(*d->temperature)});
    return fields;
}

bool TranslationRequest::operator==(const TranslationRequest &other) const
{
    return d->fileData == other.d->fileData && d->fileName == other.d->fileName
           && d->model == other.d->model && d->prompt == other.d->prompt
           && d->responseFormat == other.d->responseFormat
           && d->temperature == other.d->temperature;
}

} // namespace Core
} // namespace QtOpenAi
