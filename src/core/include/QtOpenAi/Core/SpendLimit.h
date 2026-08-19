// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class SpendLimitData;

// A hard spend limit (GET/POST/DELETE /organization/spend_limit, and the same
// three under /organization/projects/{project_id}/spend_limit).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// **This is the one that stops requests, not the one that sends email.** Its
// sibling Core::SpendAlert notifies when spending crosses a threshold and
// changes nothing; a limit is enforced, and `enforcementStatus()` reports
// whether it is currently biting. Setting one on the wrong scope, or with the
// factor of a hundred wrong, takes an organization off the air rather than
// filling an inbox — so the two are separate types with separate methods even
// though four of their fields have the same names.
//
// **`thresholdAmount()` is in cents**, as the alert's is, and the library keeps
// the API's unit rather than converting. One difference from the alert is worth
// knowing: the API's minimum here is **1**, not 0. A limit of zero would mean
// "permit nothing", and the API declines to let that be expressed by accident —
// the way to have no limit is to delete it, not to set it to nothing.
//
//     limit.setThresholdAmount(100000);   // $1,000.00
//
// One type serves the organization's limit and a project's; they differ only in
// the value of `object`, as the spend alerts do.
//
// The deletion acknowledgement decodes into this type as well, reporting the
// object as "organization.spend_limit.deleted" (or the project's equivalent)
// and carrying no threshold — deleting the limit is how spending becomes
// unlimited again.
class QTOPENAI_CORE_EXPORT SpendLimit
{
public:
    SpendLimit();
    SpendLimit(const SpendLimit &other);
    SpendLimit(SpendLimit &&other) noexcept;
    SpendLimit &operator=(const SpendLimit &other);
    SpendLimit &operator=(SpendLimit &&other) noexcept;
    ~SpendLimit();

    void swap(SpendLimit &other) noexcept { d.swap(other.d); }

    // "organization.spend_limit" or "project.spend_limit", and the ".deleted"
    // forms of either.
    QString object() const;
    void setObject(const QString &object);

    // **Cents**, and at least 1 on a write. See the class note.
    qint64 thresholdAmount() const;
    void setThresholdAmount(qint64 thresholdAmount);

    // "USD" — the only currency today, and defaulted to it because a write
    // without one is refused. A string rather than an enum, as every vocabulary
    // on this surface is.
    QString currency() const;
    void setCurrency(const QString &currency);

    // "month" — likewise the only interval, and likewise defaulted.
    QString interval() const;
    void setInterval(const QString &interval);

    // "inactive" or "enforcing": whether the limit is currently stopping
    // requests. **Server-owned** — it is reported by a read and never sent by a
    // write, because whether a limit is biting is a fact about spending rather
    // than a setting.
    QString enforcementStatus() const;
    void setEnforcementStatus(const QString &status);

    // Whether requests are being refused right now. Note that this is not the
    // same question as "is a limit configured": a limit exists from the moment
    // it is set, and only starts enforcing once spending reaches it.
    bool isEnforcing() const { return enforcementStatus() == QLatin1String("enforcing"); }

    // True in the answer to DELETE .../spend_limit.
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static SpendLimit fromJson(const QJsonObject &json);

    bool operator==(const SpendLimit &other) const;
    bool operator!=(const SpendLimit &other) const { return !(*this == other); }

private:
    QSharedDataPointer<SpendLimitData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::SpendLimit)
Q_DECLARE_METATYPE(QtOpenAi::Core::SpendLimit)
