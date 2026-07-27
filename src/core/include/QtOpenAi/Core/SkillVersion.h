// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class SkillVersionData;

// One immutable version of a skill (POST/GET /skills/{id}/versions,
// GET/DELETE /skills/{id}/versions/{version}). Publishing never overwrites: each
// upload becomes a new version, and the skill's `default_version` pointer decides
// which one is served.
//
// `version` is a string on the wire, not a number — it is the path segment of
// /skills/{id}/versions/{version}, so it is kept exactly as sent rather than
// parsed into an int and re-rendered.
//
// The deletion acknowledgement of DELETE /skills/{id}/versions/{version} also
// decodes into this type, reporting the object as "skill.version.deleted".
class QTOPENAI_CORE_EXPORT SkillVersion
{
public:
    SkillVersion();
    SkillVersion(const SkillVersion &other);
    SkillVersion(SkillVersion &&other) noexcept;
    SkillVersion &operator=(const SkillVersion &other);
    SkillVersion &operator=(SkillVersion &&other) noexcept;
    ~SkillVersion();

    void swap(SkillVersion &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "skill.version" (or "skill.version.deleted").
    QString object() const;
    void setObject(const QString &object);

    // The skill this version belongs to.
    QString skillId() const;
    void setSkillId(const QString &skillId);

    // The version number, as the API spells it.
    QString version() const;
    void setVersion(const QString &version);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QJsonObject toJson() const;
    static SkillVersion fromJson(const QJsonObject &json);

    bool operator==(const SkillVersion &other) const;
    bool operator!=(const SkillVersion &other) const { return !(*this == other); }

private:
    QSharedDataPointer<SkillVersionData> d;
};

// A `list` of skill versions (GET /skills/{id}/versions). Cursor-paginated;
// reuses the shared list-page type.
using SkillVersionList = ListPage<SkillVersion>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::SkillVersion)
Q_DECLARE_METATYPE(QtOpenAi::Core::SkillVersion)
Q_DECLARE_METATYPE(QtOpenAi::Core::SkillVersionList)
