// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/TypedReply.h>
#include <QtOpenAi/Core/SkillVersion.h>

namespace QtOpenAi {
namespace Client {

// An asynchronous handle for a single skill version (POST/GET/DELETE around
// /skills/{id}/versions). The deletion acknowledgement decodes into the same
// type, with its object reported as "skill.version.deleted". See RestReplyBase
// for the shared lifecycle.
class QTOPENAI_CLIENT_EXPORT SkillVersionReply : public TypedReply<Core::SkillVersion>
{
    Q_OBJECT
public:
    Core::SkillVersion version() const { return value(); }

Q_SIGNALS:
    void finished(const QtOpenAi::Core::SkillVersion &version);

private:
    friend class Client;
    using TypedReply::TypedReply;

    void emitFinished(const Core::SkillVersion &version) override { Q_EMIT finished(version); }
};

} // namespace Client
} // namespace QtOpenAi
