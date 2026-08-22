// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VideoCharacter.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class VideoCharacterData : public QSharedData
{
public:
    QString id;
    QString name;
    qint64 createdAt = 0;
};

VideoCharacter::VideoCharacter()
    : d(new VideoCharacterData)
{ }

VideoCharacter::VideoCharacter(const VideoCharacter &other) = default;
VideoCharacter::VideoCharacter(VideoCharacter &&other) noexcept = default;
VideoCharacter &VideoCharacter::operator=(const VideoCharacter &other) = default;
VideoCharacter &VideoCharacter::operator=(VideoCharacter &&other) noexcept = default;
VideoCharacter::~VideoCharacter() = default;

QString VideoCharacter::id() const { return d->id; }
void VideoCharacter::setId(const QString &id) { d->id = id; }

QString VideoCharacter::name() const { return d->name; }
void VideoCharacter::setName(const QString &name) { d->name = name; }

qint64 VideoCharacter::createdAt() const { return d->createdAt; }
void VideoCharacter::setCreatedAt(qint64 createdAt) { d->createdAt = createdAt; }

QJsonObject VideoCharacter::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("name"), d->name);
    detail::insertIfNonZero(json, QStringLiteral("created_at"), d->createdAt);
    return json;
}

VideoCharacter VideoCharacter::fromJson(const QJsonObject &json)
{
    VideoCharacter character;
    // Both strings are declared nullable, so stringOr() -- which answers empty
    // for a null as readily as for a missing key -- is the right reader here.
    character.d->id = detail::stringOr(json, QStringLiteral("id"));
    character.d->name = detail::stringOr(json, QStringLiteral("name"));
    character.d->createdAt = detail::int64Or(json, QStringLiteral("created_at"));
    return character;
}

bool VideoCharacter::operator==(const VideoCharacter &other) const
{
    return d->id == other.d->id && d->name == other.d->name && d->createdAt == other.d->createdAt;
}

} // namespace Core
} // namespace QtOpenAi
