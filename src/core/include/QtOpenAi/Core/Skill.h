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

class SkillData;

// A skill (POST/GET /skills, GET/POST/DELETE /skills/{id}) — a named, reusable
// bundle of files a model can be pointed at. The bundle itself is never inline:
// it is uploaded as multipart form data (see CreateSkillRequest) and downloaded
// as a zip (see Client::downloadSkillContent).
//
// A skill is a pointer to versions rather than content of its own, which is why
// it carries two of them: `latest_version` is whatever was published last, while
// `default_version` is the one served to callers who do not ask for a specific
// version. Publishing does not promote — moving the default is its own call
// (Client::setDefaultSkillVersion).
//
// The deletion acknowledgement of DELETE /skills/{id} also decodes into this
// type; it keeps the id in `id` and reports the object as "skill.deleted".
class QTOPENAI_CORE_EXPORT Skill
{
public:
    Skill();
    Skill(const Skill &other);
    Skill(Skill &&other) noexcept;
    Skill &operator=(const Skill &other);
    Skill &operator=(Skill &&other) noexcept;
    ~Skill();

    void swap(Skill &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // The object type, normally "skill" (or "skill.deleted").
    QString object() const;
    void setObject(const QString &object);

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    // Unix creation timestamp (`created_at`); 0 when absent.
    qint64 createdAt() const;
    void setCreatedAt(qint64 createdAt);

    // The version served when a caller names no version.
    QString defaultVersion() const;
    void setDefaultVersion(const QString &defaultVersion);

    // The most recently published version.
    QString latestVersion() const;
    void setLatestVersion(const QString &latestVersion);

    QJsonObject toJson() const;
    static Skill fromJson(const QJsonObject &json);

    bool operator==(const Skill &other) const;
    bool operator!=(const Skill &other) const { return !(*this == other); }

private:
    QSharedDataPointer<SkillData> d;
};

// A `list` of skills (GET /skills). Cursor-paginated; reuses the shared
// list-page type.
using SkillList = ListPage<Skill>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::Skill)
Q_DECLARE_METATYPE(QtOpenAi::Core::Skill)
Q_DECLARE_METATYPE(QtOpenAi::Core::SkillList)
