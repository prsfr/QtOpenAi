// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/SpendAlert.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class SpendAlertNotificationChannelData : public QSharedData
{
public:
    // Defaulted rather than left empty: "email" is the only value the API takes,
    // and a create that omitted it would be refused for a field the caller never
    // had a choice about.
    QString type = QStringLiteral("email");
    QStringList recipients;
    QString subjectPrefix;
};

SpendAlertNotificationChannel::SpendAlertNotificationChannel()
    : d(new SpendAlertNotificationChannelData)
{ }

SpendAlertNotificationChannel::SpendAlertNotificationChannel(
        const SpendAlertNotificationChannel &other)
        = default;
SpendAlertNotificationChannel::SpendAlertNotificationChannel(
        SpendAlertNotificationChannel &&other) noexcept
        = default;
SpendAlertNotificationChannel &
SpendAlertNotificationChannel::operator=(const SpendAlertNotificationChannel &other)
        = default;
SpendAlertNotificationChannel &
SpendAlertNotificationChannel::operator=(SpendAlertNotificationChannel &&other) noexcept
        = default;
SpendAlertNotificationChannel::~SpendAlertNotificationChannel() = default;

QString SpendAlertNotificationChannel::type() const { return d->type; }
void SpendAlertNotificationChannel::setType(const QString &type) { d->type = type; }

QStringList SpendAlertNotificationChannel::recipients() const { return d->recipients; }
void SpendAlertNotificationChannel::setRecipients(const QStringList &recipients)
{
    d->recipients = recipients;
}

QString SpendAlertNotificationChannel::subjectPrefix() const { return d->subjectPrefix; }
void SpendAlertNotificationChannel::setSubjectPrefix(const QString &subjectPrefix)
{
    d->subjectPrefix = subjectPrefix;
}

QJsonObject SpendAlertNotificationChannel::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("type"), d->type);
    detail::insertIfNotEmpty(json, QStringLiteral("recipients"), d->recipients);
    detail::insertIfNotEmpty(json, QStringLiteral("subject_prefix"), d->subjectPrefix);
    return json;
}

SpendAlertNotificationChannel SpendAlertNotificationChannel::fromJson(const QJsonObject &json)
{
    SpendAlertNotificationChannel channel;
    channel.d->type = detail::stringOr(json, QStringLiteral("type"));
    channel.d->recipients = detail::stringListOr(json, QStringLiteral("recipients"));
    channel.d->subjectPrefix = detail::stringOr(json, QStringLiteral("subject_prefix"));
    return channel;
}

bool SpendAlertNotificationChannel::operator==(const SpendAlertNotificationChannel &other) const
{
    return d->type == other.d->type && d->recipients == other.d->recipients
           && d->subjectPrefix == other.d->subjectPrefix;
}

class SpendAlertData : public QSharedData
{
public:
    QString id;
    QString object;
    // Cents. See the header: the unit is the API's, kept rather than converted.
    qint64 thresholdAmount = 0;
    QString currency = QStringLiteral("USD");
    QString interval = QStringLiteral("month");
    SpendAlertNotificationChannel notificationChannel;
    bool deleted = false;
};

SpendAlert::SpendAlert()
    : d(new SpendAlertData)
{ }

SpendAlert::SpendAlert(const SpendAlert &other) = default;
SpendAlert::SpendAlert(SpendAlert &&other) noexcept = default;
SpendAlert &SpendAlert::operator=(const SpendAlert &other) = default;
SpendAlert &SpendAlert::operator=(SpendAlert &&other) noexcept = default;
SpendAlert::~SpendAlert() = default;

QString SpendAlert::id() const { return d->id; }
void SpendAlert::setId(const QString &id) { d->id = id; }

QString SpendAlert::object() const { return d->object; }
void SpendAlert::setObject(const QString &object) { d->object = object; }

qint64 SpendAlert::thresholdAmount() const { return d->thresholdAmount; }
void SpendAlert::setThresholdAmount(qint64 thresholdAmount)
{
    d->thresholdAmount = thresholdAmount;
}

QString SpendAlert::currency() const { return d->currency; }
void SpendAlert::setCurrency(const QString &currency) { d->currency = currency; }

QString SpendAlert::interval() const { return d->interval; }
void SpendAlert::setInterval(const QString &interval) { d->interval = interval; }

SpendAlertNotificationChannel SpendAlert::notificationChannel() const
{
    return d->notificationChannel;
}
void SpendAlert::setNotificationChannel(const SpendAlertNotificationChannel &channel)
{
    d->notificationChannel = channel;
}

bool SpendAlert::isDeleted() const { return d->deleted; }
void SpendAlert::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject SpendAlert::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("id"), d->id);
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    // Written even at zero, unlike most fields here: a threshold of 0 is a
    // legal alert -- one that fires on the first cent of the month -- and
    // leaving it out would turn it into a create the server refuses for a
    // missing required field rather than the alert that was asked for.
    json.insert(QStringLiteral("threshold_amount"), d->thresholdAmount);
    detail::insertIfNotEmpty(json, QStringLiteral("currency"), d->currency);
    detail::insertIfNotEmpty(json, QStringLiteral("interval"), d->interval);
    const QJsonObject channel = d->notificationChannel.toJson();
    if (!channel.isEmpty())
        json.insert(QStringLiteral("notification_channel"), channel);
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

SpendAlert SpendAlert::fromJson(const QJsonObject &json)
{
    SpendAlert alert;
    alert.d->id = detail::stringOr(json, QStringLiteral("id"));
    alert.d->object = detail::stringOr(json, QStringLiteral("object"));
    alert.d->thresholdAmount = detail::int64Or(json, QStringLiteral("threshold_amount"));
    alert.d->currency = detail::stringOr(json, QStringLiteral("currency"));
    alert.d->interval = detail::stringOr(json, QStringLiteral("interval"));
    alert.d->notificationChannel = SpendAlertNotificationChannel::fromJson(
            json.value(QStringLiteral("notification_channel")).toObject());
    alert.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return alert;
}

bool SpendAlert::operator==(const SpendAlert &other) const
{
    return d->id == other.d->id && d->object == other.d->object
           && d->thresholdAmount == other.d->thresholdAmount && d->currency == other.d->currency
           && d->interval == other.d->interval
           && d->notificationChannel == other.d->notificationChannel
           && d->deleted == other.d->deleted;
}

} // namespace Core
} // namespace QtOpenAi
