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
        m_buffer += bytes;
        // SSE events are separated by a blank line. Normalise CRLF first.
        m_buffer.replace("\r\n", "\n");

        int sep;
        while ((sep = m_buffer.indexOf("\n\n")) != -1) {
            const QByteArray block = m_buffer.left(sep);
            m_buffer.remove(0, sep + 2);

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
};

} // namespace detail
} // namespace Client
} // namespace QtOpenAi
