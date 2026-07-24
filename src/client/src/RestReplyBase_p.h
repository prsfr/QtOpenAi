// SPDX-License-Identifier: MIT
#pragma once

// Private data for RestReplyBase and its subclasses. A concrete reply derives
// its own XxxReplyPrivate from this and stores its parsed value there, so the
// value member stays out of the public header (Qt d-pointer convention). The
// base's virtual destructor lets the single QScopedPointer<RestReplyBasePrivate>
// own the derived Private safely. Not installed / not part of the public API.

#include "QtOpenAi/Client/ClientError.h"

namespace QtOpenAi {
namespace Client {

class RestReply;

class RestReplyBasePrivate
{
public:
    virtual ~RestReplyBasePrivate() = default;

    RestReply *engine = nullptr;
    ClientError error;
    bool finished = false;
    bool success = false;
    bool autoDelete = true;
};

} // namespace Client
} // namespace QtOpenAi
