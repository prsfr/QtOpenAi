// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/BucketPage.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

// A monetary amount as the costs endpoint reports it: a number and the currency
// it is in, never one without the other.
//
// A small aggregate rather than an implicitly shared class, like RateLimit: two
// members, no allocation to save, and it is always read as part of a CostResult.
//
// The value is a `double` because that is what the API sends. It is a reported
// figure to display and to sum for a chart, not an accounting balance — code
// settling invoices should be reading its ledger, not this.
struct QTOPENAI_CORE_EXPORT CostAmount
{
    double value = 0.0;
    QString currency; // ISO 4217, lower-case as the API sends it ("usd")

    // A cost with no currency is one the server did not report, not a free one.
    bool isValid() const { return !currency.isEmpty(); }

    QJsonObject toJson() const;
    static CostAmount fromJson(const QJsonObject &json);

    bool operator==(const CostAmount &other) const
    {
        return qFuzzyCompare(value, other.value) && currency == other.currency;
    }
    bool operator!=(const CostAmount &other) const { return !(*this == other); }
};

class CostResultData;

// One row of an organization cost report (GET /organization/costs), the OpenAPI
// `CostsResult`.
//
// Its own type rather than another UsageResult: a cost row carries money, not a
// counter, and the two would not survive the same map. What it does share is the
// bucket around it — see Bucket in BucketPage.h.
class QTOPENAI_CORE_EXPORT CostResult
{
public:
    CostResult();
    CostResult(const CostResult &other);
    CostResult(CostResult &&other) noexcept;
    CostResult &operator=(const CostResult &other);
    CostResult &operator=(CostResult &&other) noexcept;
    ~CostResult();

    void swap(CostResult &other) noexcept { d.swap(other.d); }

    // The object type, normally "organization.costs.result".
    QString object() const;
    void setObject(const QString &object);

    CostAmount amount() const;
    void setAmount(const CostAmount &amount);

    // What was billed, e.g. "gpt-4o-2024-08-06, input" — set only when the query
    // grouped by line item.
    QString lineItem() const;
    void setLineItem(const QString &lineItem);

    // Set only when the query grouped by project.
    QString projectId() const;
    void setProjectId(const QString &projectId);

    QJsonObject toJson() const;
    static CostResult fromJson(const QJsonObject &json);

    bool operator==(const CostResult &other) const;
    bool operator!=(const CostResult &other) const { return !(*this == other); }

private:
    QSharedDataPointer<CostResultData> d;
};

// One time bucket of a cost report, and the page of them /organization/costs
// returns.
using CostBucket = Bucket<CostResult>;
using CostPage = BucketPage<CostResult>;

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::CostResult)
Q_DECLARE_METATYPE(QtOpenAi::Core::CostAmount)
Q_DECLARE_METATYPE(QtOpenAi::Core::CostResult)
Q_DECLARE_METATYPE(QtOpenAi::Core::CostBucket)
Q_DECLARE_METATYPE(QtOpenAi::Core::CostPage)
