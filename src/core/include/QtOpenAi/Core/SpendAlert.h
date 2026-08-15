// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/ListPage.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class SpendAlertNotificationChannelData;

// Where a spend alert is sent when it fires. Email today, and a `type` that says
// so — the API documents no other channel yet, and the field is kept rather than
// assumed so that a channel added later arrives readable.
class QTOPENAI_CORE_EXPORT SpendAlertNotificationChannel
{
public:
    SpendAlertNotificationChannel();
    SpendAlertNotificationChannel(const SpendAlertNotificationChannel &other);
    SpendAlertNotificationChannel(SpendAlertNotificationChannel &&other) noexcept;
    SpendAlertNotificationChannel &operator=(const SpendAlertNotificationChannel &other);
    SpendAlertNotificationChannel &operator=(SpendAlertNotificationChannel &&other) noexcept;
    ~SpendAlertNotificationChannel();

    void swap(SpendAlertNotificationChannel &other) noexcept { d.swap(other.d); }

    // "email". Defaults to that on a channel built here, since it is the only
    // value the API accepts and a create would be rejected without it.
    QString type() const;
    void setType(const QString &type);

    // The addresses the alert goes to. An alert with none is one nobody will
    // ever hear.
    QStringList recipients() const;
    void setRecipients(const QStringList &recipients);

    // Optional prefix on the alert email's subject line.
    QString subjectPrefix() const;
    void setSubjectPrefix(const QString &subjectPrefix);

    QJsonObject toJson() const;
    static SpendAlertNotificationChannel fromJson(const QJsonObject &json);

    bool operator==(const SpendAlertNotificationChannel &other) const;
    bool operator!=(const SpendAlertNotificationChannel &other) const { return !(*this == other); }

private:
    QSharedDataPointer<SpendAlertNotificationChannelData> d;
};

class SpendAlertData;

// A spend alert: an email when spending crosses a threshold within an interval
// (GET/POST /organization/spend_alerts, GET/POST/DELETE .../{alert_id}, and the
// same five under /organization/projects/{project_id}/spend_alerts).
//
// Part of the administration surface, which uses an admin API key rather than a
// standard one — see QtOpenAi::Admin::Organization.
//
// **`thresholdAmount()` is in cents, not dollars.** The API says so and this
// type keeps the API's unit rather than converting, because a converting
// accessor is a place for the factor to be applied twice or not at all. Getting
// it wrong is not a rounding error: at a hundredfold, an alert meant for
// $1,000.00 either fires on the first dollar of the month or never fires at all,
// and nobody notices the second kind until the invoice arrives.
//
//     alert.setThresholdAmount(100000);   // $1,000.00
//
// One type serves the organization's alerts and a project's. The API describes
// them as two schemas, but they differ in nothing except the value of `object` —
// the same call Core::Certificate makes for its three, and Core::OrganizationUser
// for organization and project members.
//
// The deletion acknowledgement decodes into this type as well, reporting the
// object as "organization.spend_alert.deleted" (or the project's equivalent).
class QTOPENAI_CORE_EXPORT SpendAlert
{
public:
    SpendAlert();
    SpendAlert(const SpendAlert &other);
    SpendAlert(SpendAlert &&other) noexcept;
    SpendAlert &operator=(const SpendAlert &other);
    SpendAlert &operator=(SpendAlert &&other) noexcept;
    ~SpendAlert();

    void swap(SpendAlert &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    // "organization.spend_alert" or "project.spend_alert", and the ".deleted"
    // forms of either.
    QString object() const;
    void setObject(const QString &object);

    // **Cents.** See the class note; this is the field to be careful with.
    qint64 thresholdAmount() const;
    void setThresholdAmount(qint64 thresholdAmount);

    // "USD" — the only currency the API takes today. A string rather than an
    // enum, as every vocabulary on this surface is: a currency added later has
    // to survive a round trip rather than decay to the first enumerator.
    // Defaults to "USD", since a create without one is refused.
    QString currency() const;
    void setCurrency(const QString &currency);

    // "month" — the window the threshold is measured over, and likewise the
    // only value today. Defaults to "month" for the same reason.
    QString interval() const;
    void setInterval(const QString &interval);

    SpendAlertNotificationChannel notificationChannel() const;
    void setNotificationChannel(const SpendAlertNotificationChannel &channel);

    // True in the answer to DELETE .../spend_alerts/{id}.
    bool isDeleted() const;
    void setDeleted(bool deleted);

    QJsonObject toJson() const;
    static SpendAlert fromJson(const QJsonObject &json);

    bool operator==(const SpendAlert &other) const;
    bool operator!=(const SpendAlert &other) const { return !(*this == other); }

private:
    QSharedDataPointer<SpendAlertData> d;
};

using SpendAlertList = ListPage<SpendAlert>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::SpendAlertNotificationChannel)
Q_DECLARE_SHARED(QtOpenAi::Core::SpendAlert)
Q_DECLARE_METATYPE(QtOpenAi::Core::SpendAlertNotificationChannel)
Q_DECLARE_METATYPE(QtOpenAi::Core::SpendAlert)
Q_DECLARE_METATYPE(QtOpenAi::Core::SpendAlertList)
