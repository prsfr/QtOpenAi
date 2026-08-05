// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/OrganizationCosts.h"

#include "JsonHelpers_p.h"

#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

QJsonObject CostAmount::toJson() const
{
    QJsonObject json;
    // Both fields or neither: a bare number with no currency is not an amount,
    // and writing one would let a reader assume the organization's own.
    if (isValid()) {
        json.insert(QStringLiteral("value"), value);
        json.insert(QStringLiteral("currency"), currency);
    }
    return json;
}

CostAmount CostAmount::fromJson(const QJsonObject &json)
{
    CostAmount amount;
    amount.value = json.value(QStringLiteral("value")).toDouble();
    amount.currency = detail::stringOr(json, QStringLiteral("currency"));
    return amount;
}

class CostResultData : public QSharedData
{
public:
    QString object;
    CostAmount amount;
    QString lineItem;
    QString projectId;
};

CostResult::CostResult()
    : d(new CostResultData)
{ }

CostResult::CostResult(const CostResult &other) = default;
CostResult::CostResult(CostResult &&other) noexcept = default;
CostResult &CostResult::operator=(const CostResult &other) = default;
CostResult &CostResult::operator=(CostResult &&other) noexcept = default;
CostResult::~CostResult() = default;

QString CostResult::object() const { return d->object; }
void CostResult::setObject(const QString &object) { d->object = object; }

CostAmount CostResult::amount() const { return d->amount; }
void CostResult::setAmount(const CostAmount &amount) { d->amount = amount; }

QString CostResult::lineItem() const { return d->lineItem; }
void CostResult::setLineItem(const QString &lineItem) { d->lineItem = lineItem; }

QString CostResult::projectId() const { return d->projectId; }
void CostResult::setProjectId(const QString &projectId) { d->projectId = projectId; }

QJsonObject CostResult::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("object"), d->object);
    if (d->amount.isValid())
        json.insert(QStringLiteral("amount"), d->amount.toJson());
    // Absent rather than null, the same as the usage grouping keys.
    detail::insertIfNotEmpty(json, QStringLiteral("line_item"), d->lineItem);
    detail::insertIfNotEmpty(json, QStringLiteral("project_id"), d->projectId);
    return json;
}

CostResult CostResult::fromJson(const QJsonObject &json)
{
    CostResult result;
    result.d->object = detail::stringOr(json, QStringLiteral("object"));
    result.d->amount = CostAmount::fromJson(json.value(QStringLiteral("amount")).toObject());
    result.d->lineItem = detail::stringOr(json, QStringLiteral("line_item"));
    result.d->projectId = detail::stringOr(json, QStringLiteral("project_id"));
    return result;
}

bool CostResult::operator==(const CostResult &other) const
{
    return d->object == other.d->object && d->amount == other.d->amount
           && d->lineItem == other.d->lineItem && d->projectId == other.d->projectId;
}

} // namespace Core
} // namespace QtOpenAi
