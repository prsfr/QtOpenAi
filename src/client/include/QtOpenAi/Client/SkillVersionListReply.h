// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/SkillVersion.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /skills/{id}/versions, returning a
// cursor-paginated page of skill versions. See RestReplyBase for the shared
// lifecycle.
class QTOPENAI_CLIENT_EXPORT SkillVersionListReply : public TypedReply<Core::SkillVersionList>
{
    Q_OBJECT
public:
    Core::SkillVersionList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::SkillVersionList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::SkillVersionList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
