// SPDX-License-Identifier: MIT
#pragma once

// Internal Server-Sent-Events framing helper shared by the streaming replies.
// Accumulates raw bytes and yields each complete event (events are separated by
// a blank line): its `event:` name and its decoded `data:` payload. The
// terminating "[DONE]" sentinel is passed through verbatim for the caller to
// detect. Not installed.
//
// Most OpenAI streams repeat the event type inside the payload and leave the
// name unused; the Assistants run stream is the one that does not name it
// anywhere else, so the name has to survive framing.

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace QtOpenAi {
namespace Client {
namespace detail {

// One framed event: the `event:` field (empty when the stream does not name its
// events) and the concatenation of its `data:` fields.
struct SseEvent
{
    QByteArray name;
    QByteArray data;
};

class SseParser
{
public:
    // Append newly-received bytes and return every event that completed in this
    // feed (in order). Events carrying no `data:` field are skipped; multi-line
    // `data:` fields are concatenated.
    QList<SseEvent> feed(const QByteArray &bytes)
    {
        QList<SseEvent> events;

        // Both of the scans below used to start at 0 on every call, and the
        // buffer only shrinks when an event *completes*. Framing one event of E
        // bytes arriving in k chunks therefore read ~E*k/2 bytes instead of E.
        // Below one transport chunk that is invisible; from ~128 KB it is 7x and
        // at 2 MB it is 63x. It is not hypothetical: every Responses stream ends
        // with one `response.completed` event as large as its whole answer, and
        // this runs on the GUI thread from readyRead.
        //
        // Normalise only what just arrived. A CRLF can straddle a chunk
        // boundary, so start one byte early; the indexOf guard keeps an LF-only
        // stream -- which is what the API sends -- from copying anything at all.
        const qsizetype appendedAt = m_buffer.size();
        m_buffer += bytes;
        const qsizetype normaliseFrom = appendedAt > 0 ? appendedAt - 1 : 0;
        if (m_buffer.indexOf('\r', normaliseFrom) != -1) {
            QByteArray tail = m_buffer.mid(normaliseFrom);
            tail.replace("\r\n", "\n");
            m_buffer.truncate(normaliseFrom);
            m_buffer += tail;
        }

        // SSE events are separated by a blank line.
        while (true) {
            const qsizetype sep = m_buffer.indexOf("\n\n", m_scanned);
            if (sep == -1)
                break;
            const QByteArray block = m_buffer.left(sep);
            m_buffer.remove(0, sep + 2);
            // The buffer shifted to the front, so what has been examined starts
            // over.
            m_scanned = 0;

            SseEvent event;
            const QList<QByteArray> lines = block.split('\n');
            for (const QByteArray &rawLine : lines) {
                if (rawLine.startsWith(':')) // comment / heartbeat
                    continue;
                if (rawLine.startsWith("event:"))
                    event.name = fieldValue(rawLine.mid(6));
                else if (rawLine.startsWith("data:"))
                    event.data += fieldValue(rawLine.mid(5));
            }
            if (!event.data.isEmpty())
                events.append(event);
        }

        // No separator in what is buffered, so the next feed() need not look at
        // it again -- except for the last byte, which a separator split across
        // the boundary would begin with.
        m_scanned = m_buffer.isEmpty() ? 0 : m_buffer.size() - 1;
        return events;
    }

    // Bytes received but not yet forming a complete event. On an error response
    // (delivered as a single JSON body, not SSE) this holds the whole body.
    QByteArray buffered() const { return m_buffer; }

private:
    // A field value is separated from its name by an optional single space.
    static QByteArray fieldValue(QByteArray value)
    {
        if (value.startsWith(' '))
            value = value.mid(1);
        return value;
    }

    QByteArray m_buffer;
    // How far into m_buffer the separator search has already looked.
    qsizetype m_scanned = 0;
};

} // namespace detail
} // namespace Client
} // namespace QtOpenAi
