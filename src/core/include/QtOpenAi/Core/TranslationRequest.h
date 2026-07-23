// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class TranslationRequestData;

// The body of a POST /audio/translations request (translate audio into English).
// Like TranscriptionRequest it is a multipart/form-data upload: the audio bytes
// plus scalar form fields exposed through formFields(). It carries fewer knobs
// than transcription (no language / granularities / include / stream).
class QTOPENAI_CORE_EXPORT TranslationRequest
{
public:
    using FormField = QPair<QString, QString>;

    TranslationRequest();
    TranslationRequest(QByteArray fileData, QString fileName, QString model);
    TranslationRequest(const TranslationRequest &other);
    TranslationRequest(TranslationRequest &&other) noexcept;
    TranslationRequest &operator=(const TranslationRequest &other);
    TranslationRequest &operator=(TranslationRequest &&other) noexcept;
    ~TranslationRequest();

    void swap(TranslationRequest &other) noexcept { d.swap(other.d); }

    QByteArray fileData() const;
    void setFileData(const QByteArray &fileData);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QString model() const;
    void setModel(const QString &model);

    QString prompt() const;
    void setPrompt(const QString &prompt);

    // "json" (default), "text", "srt", "verbose_json", or "vtt"; empty omits it.
    QString responseFormat() const;
    void setResponseFormat(const QString &format);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    QList<FormField> formFields() const;

    bool operator==(const TranslationRequest &other) const;
    bool operator!=(const TranslationRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<TranslationRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::TranslationRequest)
