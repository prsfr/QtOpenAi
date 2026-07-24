// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class CreateVideoRequestData;

// The body of a POST /videos request (create a Sora video-generation job).
//
// Only `prompt` is required. When an input reference image/video is attached
// the request must be sent as multipart/form-data (exposed via formFields() and
// the inputReference* accessors); otherwise a plain JSON body (toJson()) is
// sufficient. The Client chooses the encoding based on hasInputReference().
class QTOPENAI_CORE_EXPORT CreateVideoRequest
{
public:
    using FormField = QPair<QString, QString>;

    CreateVideoRequest();
    explicit CreateVideoRequest(QString prompt, QString model = {});
    CreateVideoRequest(const CreateVideoRequest &other);
    CreateVideoRequest(CreateVideoRequest &&other) noexcept;
    CreateVideoRequest &operator=(const CreateVideoRequest &other);
    CreateVideoRequest &operator=(CreateVideoRequest &&other) noexcept;
    ~CreateVideoRequest();

    void swap(CreateVideoRequest &other) noexcept { d.swap(other.d); }

    QString prompt() const;
    void setPrompt(const QString &prompt);

    QString model() const;
    void setModel(const QString &model);

    // Clip duration in seconds as a string (e.g. "4", "8", "12"); empty omits it.
    QString seconds() const;
    void setSeconds(const QString &seconds);

    // Frame size, e.g. "720x1280"; empty omits it.
    QString size() const;
    void setSize(const QString &size);

    // Optional reference image/video the render is conditioned on. Setting it
    // switches the request to a multipart/form-data upload.
    QByteArray inputReferenceData() const;
    QString inputReferenceFileName() const;
    void setInputReference(const QString &fileName, const QByteArray &data);
    bool hasInputReference() const;

    // Extra provider-specific fields merged verbatim into the JSON request body.
    QJsonObject extraBody() const;
    void setExtraBody(const QJsonObject &extra);

    // The non-file form fields, in a stable order, for multipart encoding.
    QList<FormField> formFields() const;

    QJsonObject toJson() const;
    static CreateVideoRequest fromJson(const QJsonObject &json);

    bool operator==(const CreateVideoRequest &other) const;
    bool operator!=(const CreateVideoRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CreateVideoRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CreateVideoRequest)
