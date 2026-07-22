// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Core/ChatCompletionChunk.h>
#include <QtOpenAi/Core/ChatCompletionResponse.h>

#include <QtCore/QScopedPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Client {

class ChatCompletionAccumulatorPrivate;

// Reassembles a sequence of streamed ChatCompletionChunk deltas into a complete
// ChatCompletionResponse: content fragments are concatenated and tool-call
// fragments are merged by index into whole ToolCalls.
//
// This is a plain helper (no networking); ChatCompletionStreamReply uses it
// internally, but it can also be driven directly over collected chunks.
class QTOPENAI_CLIENT_EXPORT ChatCompletionAccumulator
{
public:
    ChatCompletionAccumulator();
    ~ChatCompletionAccumulator();

    // Fold one chunk into the accumulated state.
    void add(const Core::ChatCompletionChunk &chunk);

    // The response assembled so far (well-formed at any point, final once the
    // stream has completed).
    Core::ChatCompletionResponse response() const;

    // The accumulated text content of a choice (default: the first choice).
    QString content(int choiceIndex = 0) const;

    void clear();

private:
    Q_DISABLE_COPY(ChatCompletionAccumulator)
    QScopedPointer<ChatCompletionAccumulatorPrivate> d;
};

} // namespace Client
} // namespace QtOpenAi
