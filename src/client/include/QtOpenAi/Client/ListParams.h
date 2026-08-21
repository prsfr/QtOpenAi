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

// Put the next page's cursor where this query carries it.
//
// The other half of the pair that lets Client::PageWalker drive an endpoint it
// was not written against; the reading half is Core::nextPageCursor(). Both are
// found by argument-dependent lookup, so a query type in another module -- the
// administration ones, say -- supplies its own overload beside itself rather
// than this header having to know about it.
inline void applyPageCursor(ListParams &params, const QString &cursor) { params.after = cursor; }

} // namespace Client
} // namespace QtOpenAi
