// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/Skill.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single skill (POST/GET/DELETE around /skills).
// The deletion acknowledgement decodes into the same type, with its object
// reported as "skill.deleted". See RestReplyBase for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT SkillReply : public TypedReply<Core::Skill>
{
    Q_OBJECT
public:
    Core::Skill skill() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::Skill &skill);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::Skill &skill) override { Q_EMIT finished(skill); }
};

} // namespace Client
} // namespace QtOpenAi
