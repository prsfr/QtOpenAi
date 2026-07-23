// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Usage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ImageData;

// A single generated/edited image. Depending on the request's response_format
// the payload is either a hosted `url` or inline base64 (`b64_json`); some
// models also return a `revised_prompt`.
class QTOPENAI_CORE_EXPORT Image
{
public:
    Image();
    Image(const Image &other);
    Image(Image &&other) noexcept;
    Image &operator=(const Image &other);
    Image &operator=(Image &&other) noexcept;
    ~Image();

    void swap(Image &other) noexcept { d.swap(other.d); }

    QString url() const;
    void setUrl(const QString &url);

    // Base64-encoded image bytes (response_format "b64_json").
    QString b64Json() const;
    void setB64Json(const QString &b64Json);

    QString revisedPrompt() const;
    void setRevisedPrompt(const QString &revisedPrompt);

    QJsonObject toJson() const;
    static Image fromJson(const QJsonObject &json);

    bool operator==(const Image &other) const;
    bool operator!=(const Image &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ImageData> d;
};

class ImageResponseData;

// A parsed images response (POST /images/generations, /edits, /variations).
class QTOPENAI_CORE_EXPORT ImageResponse
{
public:
    ImageResponse();
    ImageResponse(const ImageResponse &other);
    ImageResponse(ImageResponse &&other) noexcept;
    ImageResponse &operator=(const ImageResponse &other);
    ImageResponse &operator=(ImageResponse &&other) noexcept;
    ~ImageResponse();

    void swap(ImageResponse &other) noexcept { d.swap(other.d); }

    // Unix creation timestamp; 0 when absent.
    qint64 created() const;
    void setCreated(qint64 created);

    QList<Image> data() const;
    void setData(const QList<Image> &data);

    // Token usage (gpt-image-1); nullopt when the model omits it.
    std::optional<Usage> usage() const;
    void setUsage(const Usage &usage);

    // Convenience: the first image, or a default-constructed one.
    Image firstImage() const;

    QJsonObject toJson() const;
    static ImageResponse fromJson(const QJsonObject &json);

    bool operator==(const ImageResponse &other) const;
    bool operator!=(const ImageResponse &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ImageResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Image)
Q_DECLARE_SHARED(QtOpenAi::Core::ImageResponse)
