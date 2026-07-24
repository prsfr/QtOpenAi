// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/CreateVideoRequest.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class CreateVideoRequestData : public QSharedData
{
public:
    QString prompt;
    QString model;
    QString seconds;
    QString size;
    QByteArray inputReferenceData;
    QString inputReferenceFileName;
    QJsonObject extraBody;
};

CreateVideoRequest::CreateVideoRequest()
    : d(new CreateVideoRequestData)
{ }

CreateVideoRequest::CreateVideoRequest(QString prompt, QString model)
    : d(new CreateVideoRequestData)
{
    d->prompt = std::move(prompt);
    d->model = std::move(model);
}

CreateVideoRequest::CreateVideoRequest(const CreateVideoRequest &other) = default;
CreateVideoRequest::CreateVideoRequest(CreateVideoRequest &&other) noexcept = default;
CreateVideoRequest &CreateVideoRequest::operator=(const CreateVideoRequest &other) = default;
CreateVideoRequest &CreateVideoRequest::operator=(CreateVideoRequest &&other) noexcept = default;
CreateVideoRequest::~CreateVideoRequest() = default;

QString CreateVideoRequest::prompt() const { return d->prompt; }
void CreateVideoRequest::setPrompt(const QString &prompt) { d->prompt = prompt; }

QString CreateVideoRequest::model() const { return d->model; }
void CreateVideoRequest::setModel(const QString &model) { d->model = model; }

QString CreateVideoRequest::seconds() const { return d->seconds; }
void CreateVideoRequest::setSeconds(const QString &seconds) { d->seconds = seconds; }

QString CreateVideoRequest::size() const { return d->size; }
void CreateVideoRequest::setSize(const QString &size) { d->size = size; }

QByteArray CreateVideoRequest::inputReferenceData() const { return d->inputReferenceData; }
QString CreateVideoRequest::inputReferenceFileName() const { return d->inputReferenceFileName; }

void CreateVideoRequest::setInputReference(const QString &fileName, const QByteArray &data)
{
    d->inputReferenceFileName = fileName;
    d->inputReferenceData = data;
}

bool CreateVideoRequest::hasInputReference() const { return !d->inputReferenceData.isEmpty(); }

QJsonObject CreateVideoRequest::extraBody() const { return d->extraBody; }
void CreateVideoRequest::setExtraBody(const QJsonObject &extra) { d->extraBody = extra; }

QList<CreateVideoRequest::FormField> CreateVideoRequest::formFields() const
{
    QList<FormField> fields;
    fields.append({QStringLiteral("prompt"), d->prompt});
    if (!d->model.isEmpty())
        fields.append({QStringLiteral("model"), d->model});
    if (!d->seconds.isEmpty())
        fields.append({QStringLiteral("seconds"), d->seconds});
    if (!d->size.isEmpty())
        fields.append({QStringLiteral("size"), d->size});
    return fields;
}

QJsonObject CreateVideoRequest::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("prompt"), d->prompt);
    detail::insertIfNotEmpty(json, QStringLiteral("model"), d->model);
    detail::insertIfNotEmpty(json, QStringLiteral("seconds"), d->seconds);
    detail::insertIfNotEmpty(json, QStringLiteral("size"), d->size);

    for (auto it = d->extraBody.constBegin(); it != d->extraBody.constEnd(); ++it) {
        if (!json.contains(it.key()))
            json.insert(it.key(), it.value());
    }
    return json;
}

CreateVideoRequest CreateVideoRequest::fromJson(const QJsonObject &json)
{
    CreateVideoRequest request;
    request.d->prompt = detail::stringOr(json, QStringLiteral("prompt"));
    request.d->model = detail::stringOr(json, QStringLiteral("model"));
    request.d->seconds = detail::stringOr(json, QStringLiteral("seconds"));
    request.d->size = detail::stringOr(json, QStringLiteral("size"));
    return request;
}

bool CreateVideoRequest::operator==(const CreateVideoRequest &other) const
{
    return d->prompt == other.d->prompt && d->model == other.d->model
           && d->seconds == other.d->seconds && d->size == other.d->size
           && d->inputReferenceData == other.d->inputReferenceData
           && d->inputReferenceFileName == other.d->inputReferenceFileName
           && d->extraBody == other.d->extraBody;
}

} // namespace Core
} // namespace QtOpenAi
