// SPDX-License-Identifier: MIT
#pragma once

// Internal Server-Sent-Events framing helper shared by the streaming replies.
// Accumulates raw bytes and yields the decoded `data:` payload of each complete
// event (events are separated by a blank line). The terminating "[DONE]"
// sentinel is passed through verbatim for the caller to detect. Not installed.

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace QtOpenAi {
namespace Client {
namespace detail {

class SseParser
{
public:
    // Append newly-received bytes and return the `data:` payload of every event
    // that completed in this feed (in order). Payloads that carry no `data:`
    // field are skipped; multi-line `data:` fields are concatenated.
    QList<QByteArray> feed(const QByteArray &bytes)
    {
        QList<QByteArray> payloads;
        m_buffer += bytes;
        // SSE events are separated by a blank line. Normalise CRLF first.
        m_buffer.replace("\r\n", "\n");

        int sep;
        while ((sep = m_buffer.indexOf("\n\n")) != -1) {
            const QByteArray event = m_buffer.left(sep);
            m_buffer.remove(0, sep + 2);

            QByteArray data;
            const QList<QByteArray> lines = event.split('\n');
            for (const QByteArray &rawLine : lines) {
                QByteArray line = rawLine;
                if (line.startsWith(':')) // comment / heartbeat
                    continue;
                if (line.startsWith("data:")) {
                    line = line.mid(5);
                    if (line.startsWith(' '))
                        line = line.mid(1);
                    data += line;
                }
            }
            if (!data.isEmpty())
                payloads.append(data);
        }
        return payloads;
    }

    // Bytes received but not yet forming a complete event. On an error response
    // (delivered as a single JSON body, not SSE) this holds the whole body.
    QByteArray buffered() const { return m_buffer; }

private:
    QByteArray m_buffer;
};

} // namespace detail
} // namespace Client
} // namespace QtOpenAi
