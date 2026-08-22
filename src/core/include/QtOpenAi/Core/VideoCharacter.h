// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class VideoCharacterData;

// A reusable cameo built from an uploaded video (POST /videos/characters,
// GET /videos/characters/{id}).
//
// A character is a *likeness*, registered once and then referred to by id in
// later prompts, so the same person appears across renders without re-uploading
// the footage. That makes it the one part of the video surface that outlives a
// single job -- and the one with a consent question attached, since the
// likeness in the uploaded video belongs to somebody.
//
// The record itself is deliberately thin: an id, the display name given at
// creation, and when it was made. The video that produced it is not part of it
// and cannot be read back.
class QTOPENAI_CORE_EXPORT VideoCharacter
{
public:
    VideoCharacter();
    VideoCharacter(const VideoCharacter &other);
    VideoCharacter(VideoCharacter &&other) noexcept;
    VideoCharacter &operator=(const VideoCharacter &other);
    VideoCharacter &operator=(VideoCharacter &&other) noexcept;
    ~VideoCharacter();

    void swap(VideoCharacter &other) noexcept { d.swap(other.d); }

    // The identifier later prompts refer to, e.g. "char_123".
    QString id() const;
    void setId(const QString &id);

    // Display name, 1-80 characters at creation time. The API declares both id
    // and name as nullable, so either can come back empty.
    QString name() const;
    void setName(const QString &name);

    // Unix timestamp (seconds) of creation; 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QJsonObject toJson() const;
    static VideoCharacter fromJson(const QJsonObject &json);

    bool operator==(const VideoCharacter &other) const;
    bool operator!=(const VideoCharacter &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VideoCharacterData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VideoCharacter)
Q_DECLARE_METATYPE(QtOpenAi::Core::VideoCharacter)
