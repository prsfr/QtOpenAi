// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>

#include <QtCore/QString>
#include <QtCore/QUrlQuery>

namespace QtOpenAi {
namespace Client {

// Common cursor-pagination query parameters shared by list endpoints. Unset
// fields are omitted from the query.
struct QTOPENAI_CLIENT_EXPORT ListParams
{
    QString after;  // cursor: return items after this id
    QString before; // cursor: return items before this id
    int limit = -1; // page size (-1 leaves the server default)
    QString order;  // "asc" or "desc"

    QUrlQuery toQuery() const
    {
        QUrlQuery query;
        if (!after.isEmpty())
            query.addQueryItem(QStringLiteral("after"), after);
        if (!before.isEmpty())
            query.addQueryItem(QStringLiteral("before"), before);
        if (limit >= 0)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
        if (!order.isEmpty())
            query.addQueryItem(QStringLiteral("order"), order);
        return query;
    }
};

} // namespace Client
} // namespace QtOpenAi
