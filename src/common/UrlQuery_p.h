// SPDX-License-Identifier: MIT
#pragma once

// The query-string half of the conventions in this directory: what
// FormFields_p.h does for a multipart body, for the endpoints whose parameters
// go in the URL.
//
// Not installed and not part of the public API.

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrlQuery>

namespace QtOpenAi {
namespace Core {
namespace detail {

// An array parameter, which the API expresses by repeating the key once per
// value rather than by joining them with commas -- a project id or an email
// address is free to contain one.
//
// The multipart side of this is FormFields_p.h's appendEach(), and the name is
// deliberately the same: it is one convention with two encodings. It was written
// out twice byte-identically in the Admin module and open-coded a third time,
// with the reasoning restated in three different comments.
inline void appendEach(QUrlQuery &query, const QString &key, const QStringList &values)
{
    for (const QString &value : values)
        query.addQueryItem(key, value);
}

} // namespace detail
} // namespace Core
} // namespace QtOpenAi
