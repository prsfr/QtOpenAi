// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ImageGenerationRequestData;

// The body of a POST /images/generations request. Only `prompt` is required;
// every other parameter is serialised only when explicitly set, matching the
// OpenAI schema semantics.
class QTOPENAI_CORE_EXPORT ImageGenerationRequest
{
public:
    ImageGenerationRequest();
    explicit ImageGenerationRequest(QString prompt, QString model = {});
    ImageGenerationRequest(const ImageGenerationRequest &other);
    ImageGenerationRequest(ImageGenerationRequest &&other) noexcept;
    ImageGenerationRequest &operator=(const ImageGenerationRequest &other);
    ImageGenerationRequest &operator=(ImageGenerationRequest &&other) noexcept;
    ~ImageGenerationRequest();

    void swap(ImageGenerationRequest &other) noexcept { d.swap(other.d); }

    QString prompt() const;
    void setPrompt(const QString &prompt);

    QString model() const;
    void setModel(const QString &model);

    std::optional<int> n() const;
    void setN(int n);

    // e.g. "1024x1024", "1792x1024", "auto"; empty omits it.
    QString size() const;
    void setSize(const QString &size);

    // e.g. "standard"/"hd" (dall-e-3) or "low"/"medium"/"high"/"auto"
    // (gpt-image-1); empty omits it.
    QString quality() const;
    void setQuality(const QString &quality);

    // "url" or "b64_json" (dall-e); empty omits it.
    QString responseFormat() const;
    void setResponseFormat(const QString &format);

    // "vivid" or "natural" (dall-e-3); empty omits it.
    QString style() const;
    void setStyle(const QString &style);

    // "transparent"/"opaque"/"auto" (gpt-image-1); empty omits it.
    QString background() const;
    void setBackground(const QString &background);

    // "png"/"jpeg"/"webp" (gpt-image-1); empty omits it.
    QString outputFormat() const;
    void setOutputFormat(const QString &outputFormat);

    // "low"/"auto" (gpt-image-1 moderation strength); empty omits it.
    QString moderation() const;
    void setModeration(const QString &moderation);

    QString user() const;
    void setUser(const QString &user);

    // Extra provider-specific fields merged verbatim into the request body.
    QJsonObject extraBody() const;
    void setExtraBody(const QJsonObject &extra);

    QJsonObject toJson() const;
    static ImageGenerationRequest fromJson(const QJsonObject &json);

    bool operator==(const ImageGenerationRequest &other) const;
    bool operator!=(const ImageGenerationRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ImageGenerationRequestData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ImageGenerationRequest)
