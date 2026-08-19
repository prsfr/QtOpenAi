// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/SpendLimit.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

class SpendLimitData : public QSharedData
{
public:
    QString object;
    // Cents. See the header: the unit is the API's, kept rather than converted.
    qint64 thresholdAmount = 0;
    QString currency = QStringLiteral("USD");
    QString interval = QStringLiteral("month");
    QString enforcementStatus;
    bool deleted = false;
};

SpendLimit::SpendLimit()
    : d(new SpendLimitData)
{ }

SpendLimit::SpendLimit(const SpendLimit &other) = default;
SpendLimit::SpendLimit(SpendLimit &&other) noexcept = default;
SpendLimit &SpendLimit::operator=(const SpendLimit &other) = default;
SpendLimit &SpendLimit::operator=(SpendLimit &&other) noexcept = default;
SpendLimit::~SpendLimit() = default;

QString SpendLimit::object() const { return d->object; }
void SpendLimit::setObject(const QString &object) { d->object = object; }

qint64 SpendLimit::thresholdAmount() const { return d->thresholdAmount; }
void SpendLimit::setThresholdAmount(qint64 thresholdAmount)
{
    d->thresholdAmount = thresholdAmount;
}

QString SpendLimit::currency() const { return d->currency; }
void SpendLimit::setCurrency(const QString &currency) { d->currency = currency; }

QString SpendLimit::interval() const { return d->interval; }
void SpendLimit::setInterval(const QString &interval) { d->interval = interval; }

QString SpendLimit::enforcementStatus() const { return d->enforcementStatus; }
void SpendLimit::setEnforcementStatus(const QString &status) { d->enforcementStatus = status; }

bool SpendLimit::isDeleted() const { return d->deleted; }
void SpendLimit::setDeleted(bool deleted) { d->deleted = deleted; }

QJsonObject SpendLimit::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    // Left out at zero, unlike Core::SpendAlert's threshold. There the zero was
    // a legal alert; here the API's minimum is 1, so a zero is an unset limit
    // rather than a limit of nothing -- and writing it would ask the server to
    // permit no spending at all.
    detail::insertIfNonZero(json, QStringLiteral("threshold_amount"), d->thresholdAmount);
    detail::insertIfNotEmpty(json, QStringLiteral("currency"), d->currency);
    detail::insertIfNotEmpty(json, QStringLiteral("interval"), d->interval);
    // Nested on the wire, and reported rather than set -- see the header.
    if (!d->enforcementStatus.isEmpty()) {
        QJsonObject enforcement;
        enforcement.insert(QStringLiteral("status"), d->enforcementStatus);
        json.insert(QStringLiteral("enforcement"), enforcement);
    }
    detail::insertIfTrue(json, QStringLiteral("deleted"), d->deleted);
    return json;
}

SpendLimit SpendLimit::fromJson(const QJsonObject &json)
{
    SpendLimit limit;
    limit.d->object = detail::stringOr(json, QStringLiteral("object"));
    limit.d->thresholdAmount = detail::int64Or(json, QStringLiteral("threshold_amount"));
    limit.d->currency = detail::stringOr(json, QStringLiteral("currency"));
    limit.d->interval = detail::stringOr(json, QStringLiteral("interval"));
    limit.d->enforcementStatus = detail::stringOr(
            json.value(QStringLiteral("enforcement")).toObject(), QStringLiteral("status"));
    limit.d->deleted = json.value(QStringLiteral("deleted")).toBool();
    return limit;
}

bool SpendLimit::operator==(const SpendLimit &other) const
{
    return d->object == other.d->object && d->thresholdAmount == other.d->thresholdAmount
           && d->currency == other.d->currency && d->interval == other.d->interval
           && d->enforcementStatus == other.d->enforcementStatus && d->deleted == other.d->deleted;
}

} // namespace Core
} // namespace QtOpenAi
