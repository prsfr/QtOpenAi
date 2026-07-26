// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/RestReplyBase.h>

#include <QtCore/QJsonObject>

namespace QtOpenAi {
namespace Client {

// A reply whose 2xx body decodes into exactly one Core value type.
//
// That describes most of this library's endpoints, and before this template
// each of them repeated the same four things: a Private holding the parsed
// value, a constructor, a getter reading it back, and a dispatchSuccess() that
// parsed the object, assigned it and emitted. Only the type and the getter's
// name ever differed.
//
// TypedReply does all of it. A concrete reply is left with exactly what is
// genuinely its own — a typed finished(...) signal, a getter named the way that
// endpoint names its payload, and the one-line emitFinished() that fires the
// signal:
//
//     class QTOPENAI_CLIENT_EXPORT ModelReply : public TypedReply<Core::Model>
//     {
//         Q_OBJECT
//     public:
//         Core::Model model() const { return value(); }
//     Q_SIGNALS:
//         void finished(const QtOpenAi::Core::Model &model);
//     private:
//         friend class Client;
//         using TypedReply::TypedReply;
//         void emitFinished(const Core::Model &model) override { Q_EMIT finished(model); }
//     };
//
// The parsed value lives here rather than behind a d-pointer, which is safe
// precisely for these types: every Core value type is a single
// QSharedDataPointer and every ListPage a fixed set of Qt containers, so the
// member is pointer-sized and its layout cannot drift. Replies that carry
// genuine state of their own — the streaming ones — still derive from
// RestReplyBase directly with their own Private.
template <typename T>
class TypedReply : public RestReplyBase
{
public:
    // The decoded payload. Default-constructed until the request succeeds.
    T value() const { return m_value; }

protected:
    using RestReplyBase::RestReplyBase;

    // Emit the subclass's typed finished(...) signal. The one line a concrete
    // reply has to write, because a class template cannot declare signals.
    virtual void emitFinished(const T &value) = 0;

private:
    bool dispatchSuccess(const QByteArray &body, int httpStatus) final
    {
        QJsonObject object;
        if (!parseJsonObject(body, httpStatus, object))
            return false;
        m_value = T::fromJson(object);
        emitFinished(m_value);
        return true;
    }

    T m_value;
};

} // namespace Client
} // namespace QtOpenAi
