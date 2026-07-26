// SPDX-License-Identifier: MIT
#pragma once

// Own a reply for the length of a test and block until it settles.
//
// Issuing a request from a test means remembering three separate things: turn
// auto-delete off so the reply survives the assertions that follow, spin the
// event loop until it finishes, and delete it at the end. Forgetting the first
// leaves the assertions reading freed memory; forgetting the last leaks; and
// the middle one is where the timeout lives, so it was written out ~120 times.
//
//     const auto reply = awaited(client.getBatch(id));
//     QVERIFY(reply);                        // settled within the timeout
//     QCOMPARE(reply->batch().id(), id);
//
// Conversion to bool reports whether the reply settled in time, so a hung
// request still fails at the call site, with the test's own line number. The
// reply is deleted when the AwaitedReply goes out of scope.
//
// Tests that need the reply live while they connect to its signals -- the
// streaming ones -- keep managing it themselves; this helper is for the
// issue-and-assert shape.

#include <QtTest/QtTest>

#include <memory>
#include <utility>

template <typename Reply>
class AwaitedReply
{
public:
    AwaitedReply(Reply *reply, int timeoutMs)
        : m_reply(reply)
    {
        reply->setAutoDelete(false);
        m_settled = QTest::qWaitFor([reply] { return reply->isFinished(); }, timeoutMs);
    }

    // True when the reply finished before the timeout expired.
    explicit operator bool() const { return m_settled; }

    Reply *operator->() const { return m_reply.get(); }
    Reply &operator*() const { return *m_reply; }
    // For the few places that need the raw pointer, e.g. QSignalSpy.
    Reply *get() const { return m_reply.get(); }

private:
    std::unique_ptr<Reply> m_reply;
    bool m_settled = false;
};

// Deduces the reply type, so call sites never name it twice.
template <typename Reply>
AwaitedReply<Reply> awaited(Reply *reply, int timeoutMs = 5000)
{
    return AwaitedReply<Reply>(reply, timeoutMs);
}
