// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class DataRetentionData;

// How long the API keeps the data sent to it (GET/POST
// /organization/data_retention, and the same pair under
// /organization/projects/{project_id}/data_retention).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// One type for both scopes: the API describes two schemas that differ only in
// the value of `object` and in which retention types they accept. The project
// scope takes everything the organization does, plus two of its own:
//
//   organization and project   zero_data_retention
//                              modified_abuse_monitoring
//                              enhanced_zero_data_retention
//                              enhanced_modified_abuse_monitoring
//   project only               organization_default   (inherit)
//                              none
//
// **The type is a string, and on this field that matters more than most.**
// Retention is a compliance setting: a value this build has never heard of must
// arrive as itself rather than decay to whichever enumerator happened to be
// first, because "the strictest policy" and "the default" are one enum position
// apart and an audit would not forgive the difference.
//
// **Reading and writing use different field names.** The resource reports
// `type`; the update body takes `retention_type`. That is the API's asymmetry,
// not this library's, and it is handled the same way Core::RoleRequest handles
// the same trick — see Admin::Organization::setDataRetention(), which takes the
// value directly, so the mismatch lives at one place instead of in every caller.
class QTOPENAI_CORE_EXPORT DataRetention
{
public:
    DataRetention();
    DataRetention(const DataRetention &other);
    DataRetention(DataRetention &&other) noexcept;
    DataRetention &operator=(const DataRetention &other);
    DataRetention &operator=(DataRetention &&other) noexcept;
    ~DataRetention();

    void swap(DataRetention &other) noexcept { d.swap(other.d); }

    // "organization.data_retention" or "project.data_retention".
    QString object() const;
    void setObject(const QString &object);

    // The configured retention policy; see the class note for the vocabulary.
    QString type() const;
    void setType(const QString &type);

    // True when a project defers to the organization's setting rather than
    // holding one of its own. Never true at the organization scope, which has
    // nothing to defer to.
    bool isOrganizationDefault() const { return type() == QLatin1String("organization_default"); }

    QJsonObject toJson() const;
    static DataRetention fromJson(const QJsonObject &json);

    bool operator==(const DataRetention &other) const;
    bool operator!=(const DataRetention &other) const { return !(*this == other); }

private:
    QSharedDataPointer<DataRetentionData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::DataRetention)
Q_DECLARE_METATYPE(QtOpenAi::Core::DataRetention)
