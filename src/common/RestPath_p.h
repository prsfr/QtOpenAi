// SPDX-License-Identifier: MIT
#pragma once

// Internal (private) helper for composing REST resource paths. Not installed
// and not part of the public API.
//
// Client and Admin both hang endpoint families off a collection, a member of
// it, and sometimes something below that. Both had their own copy of this, and
// Admin's carried a comment saying so.

#include <QtCore/QLatin1String>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Rest {

// A resource path: a collection, optionally one member of it, optionally a
// sub-resource below that -- e.g. ("/organization/projects", "proj_1",
// "/api_keys"). An empty id addresses the collection itself, which is what
// makes the same call serve "list" and "get". Nesting composes: pass the result
// of one call as the suffix of another.
inline QString resourcePath(QLatin1String collection, const QString &id, const QString &suffix = {})
{
    QString path(collection);
    if (!id.isEmpty())
        path += QLatin1Char('/') + id;
    return path + suffix;
}

} // namespace Rest
} // namespace QtOpenAi
