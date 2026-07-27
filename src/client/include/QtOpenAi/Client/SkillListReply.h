// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Skill.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for GET /skills, returning a cursor-paginated page of
// skills. See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT SkillListReply : public TypedReply<Core::SkillList>
{
    Q_OBJECT
public:
    Core::SkillList list() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::SkillList &list);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::SkillList &list) override { Q_EMIT finished(list); }
};

} // namespace Client
} // namespace QtOpenAi
