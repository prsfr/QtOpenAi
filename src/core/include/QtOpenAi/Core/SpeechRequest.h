// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class SpeechRequestData;

// The body of a POST /audio/speech request (text-to-speech). The endpoint
// returns a binary audio blob rather than JSON, so only the request is modelled
// here; the bytes are surfaced by Client::SpeechReply. Optional parameters are
// serialised only when explicitly set, matching the OpenAI schema semantics.
class QTOPENAI_CORE_EXPORT SpeechRequest
{
public:
    SpeechRequest();
    SpeechRequest(QString model, QString input, QString voice);
    SpeechRequest(const SpeechRequest &other);
    SpeechRequest(SpeechRequest &&other) noexcept;
    SpeechRequest &operator=(const SpeechRequest &other);
    SpeechRequest &operator=(SpeechRequest &&other) noexcept;
    ~SpeechRequest();

    void swap(SpeechRequest &other) noexcept { d.swap(other.d); }

    QString model() const;
    void setModel(const QString &model);

    // The text to synthesise.
    QString input() const;
    void setInput(const QString &input);

    // The voice, e.g. "alloy", "verse", "coral".
    QString voice() const;
    void setVoice(const QString &voice);

    // Audio container: "mp3" (default), "opus", "aac", "flac", "wav", "pcm".
    // Empty omits the field (server default).
    QString responseFormat() const;
    void setResponseFormat(const QString &format);

    // Playback speed multiplier (0.25–4.0); unset omits the field.
    std::optional<double> speed() const;
    void setSpeed(double speed);

    // Voice/tone control for supported models; empty omits the field.
    QString instructions() const;
    void setInstructions(const QString &instructions);

    // Streamed transport shape: "audio" or "sse"; empty omits the field.
    QString streamFormat() const;
    void setStreamFormat(const QString &streamFormat);

    QJsonObject toJson() const;
    static SpeechRequest fromJson(const QJsonObject &json);

    bool operator==(const SpeechRequest &other) const;
    bool operator!=(const SpeechRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<SpeechRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::SpeechRequest)
