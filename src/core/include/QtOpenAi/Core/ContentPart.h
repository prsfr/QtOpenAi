// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class ContentPartData;

// One part of a multimodal message content array. A tagged value type covering
// the OpenAI content-part variants:
//   - "text":        text()
//   - "image_url":   imageUrl() (URL or data: URI) + imageDetail()
//   - "input_audio": audioData() (base64) + audioFormat() ("wav"/"mp3")
//   - "file":        fileId() or fileData()/fileName()
class QTOPENAI_CORE_EXPORT ContentPart
{
public:
    ContentPart();
    explicit ContentPart(const QString &type);
    ContentPart(const ContentPart &other);
    ContentPart(ContentPart &&other) noexcept;
    ContentPart &operator=(const ContentPart &other);
    ContentPart &operator=(ContentPart &&other) noexcept;
    ~ContentPart();

    void swap(ContentPart &other) noexcept { d.swap(other.d); }

    QString type() const;
    void setType(const QString &type);

    // --- text --------------------------------------------------------------
    QString text() const;
    void setText(const QString &text);

    // --- image_url ---------------------------------------------------------
    QString imageUrl() const;
    void setImageUrl(const QString &imageUrl);
    // "auto" (default), "low", or "high"; empty omits it.
    QString imageDetail() const;
    void setImageDetail(const QString &detail);

    // --- input_audio -------------------------------------------------------
    QString audioData() const;
    void setAudioData(const QString &audioData);
    QString audioFormat() const;
    void setAudioFormat(const QString &format);

    // --- file --------------------------------------------------------------
    QString fileId() const;
    void setFileId(const QString &fileId);
    QString fileData() const;
    void setFileData(const QString &fileData);
    QString fileName() const;
    void setFileName(const QString &fileName);

    bool isText() const { return type() == QLatin1String("text"); }

    // Convenience factories.
    static ContentPart text(const QString &text);
    static ContentPart imageUrl(const QString &url, const QString &detail = {});
    static ContentPart inputAudio(const QString &base64Data, const QString &format);
    static ContentPart file(const QString &fileId);

    QJsonObject toJson() const;
    static ContentPart fromJson(const QJsonObject &json);

    bool operator==(const ContentPart &other) const;
    bool operator!=(const ContentPart &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ContentPartData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ContentPart)
