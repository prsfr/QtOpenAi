// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Core {

class TranscriptionRequestData;

// The body of a POST /audio/transcriptions request. This endpoint expects a
// multipart/form-data upload: the audio bytes plus a set of scalar form fields.
// The file itself is carried as raw bytes (with a filename); everything else is
// exposed through formFields() as ordered name/value pairs the Client turns into
// multipart parts. Optional fields are only emitted when explicitly set.
class QTOPENAI_CORE_EXPORT TranscriptionRequest
{
public:
    using FormField = QPair<QString, QString>;

    TranscriptionRequest();
    TranscriptionRequest(QByteArray fileData, QString fileName, QString model);
    TranscriptionRequest(const TranscriptionRequest &other);
    TranscriptionRequest(TranscriptionRequest &&other) noexcept;
    TranscriptionRequest &operator=(const TranscriptionRequest &other);
    TranscriptionRequest &operator=(TranscriptionRequest &&other) noexcept;
    ~TranscriptionRequest();

    void swap(TranscriptionRequest &other) noexcept { d.swap(other.d); }

    // The audio bytes to upload (the multipart `file` part).
    QByteArray fileData() const;
    void setFileData(const QByteArray &fileData);

    // The upload filename; its extension tells the API the audio format.
    QString fileName() const;
    void setFileName(const QString &fileName);

    QString model() const;
    void setModel(const QString &model);

    // ISO-639-1 source language hint; empty omits it.
    QString language() const;
    void setLanguage(const QString &language);

    // Optional prompt to guide style/spelling; empty omits it.
    QString prompt() const;
    void setPrompt(const QString &prompt);

    // "json" (default), "text", "srt", "verbose_json", or "vtt"; empty omits it.
    QString responseFormat() const;
    void setResponseFormat(const QString &format);

    std::optional<double> temperature() const;
    void setTemperature(double temperature);

    // "word" and/or "segment" (requires verbose_json); empty omits the field.
    QStringList timestampGranularities() const;
    void setTimestampGranularities(const QStringList &granularities);

    // Extra fields to include, e.g. "logprobs"; empty omits the field.
    QStringList include() const;
    void setInclude(const QStringList &include);

    std::optional<bool> stream() const;
    void setStream(bool stream);

    // The non-file form fields, in a stable order, ready for multipart encoding.
    QList<FormField> formFields() const;

    bool operator==(const TranscriptionRequest &other) const;
    bool operator!=(const TranscriptionRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<TranscriptionRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::TranscriptionRequest)
