// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>
#include <QtOpenAi/Core/Message.h>

#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace QtOpenAi {
namespace Core {

class TokenCounterData;

// How many tokens a prompt will cost, without asking the server.
//
// Trimming to a context window, estimating a bill and staying under a rate
// limit all need the number *before* the request goes out, which rules out
// reading it off the response.
//
//     TokenCounter counter = TokenCounter::forModel("gpt-4o-mini");
//     counter.count(messages);
//
// **Vocabulary is not bundled.** The OpenAI encodings are several megabytes of
// table each, they belong to a project with its own release cadence, and most
// callers never need an exact count. So this ships the algorithm and takes the
// data: point `loadEncodingFile()` at a `.tiktoken` file -- the format
// tiktoken publishes, one `<base64 token> <rank>` per line -- and counting for
// that encoding becomes exact from then on.
//
// Without it, `count()` still answers, using the customary one-token-per-four-
// characters estimate. `isExact()` says which of the two you are getting, so a
// caller that must not overrun a window can leave itself room, and one that
// only wants a rough figure need not care.
//
// Byte-pair merging follows tiktoken's algorithm on the UTF-8 bytes of each
// piece the encoding's pre-tokenizer regex produces, so a loaded vocabulary
// counts what the server counts. Special tokens (`<|endoftext|>` and friends)
// are not part of a `.tiktoken` file and are not recognised; text is counted as
// text.
class QTOPENAI_CORE_EXPORT TokenCounter
{
public:
    // The heuristic counter -- no encoding, never exact.
    TokenCounter();
    // A counter for a named encoding, e.g. "o200k_base". Exact once that
    // encoding's vocabulary has been loaded, heuristic until then, so the
    // counter can be built before the data is.
    explicit TokenCounter(const QString &encoding);
    TokenCounter(const TokenCounter &other);
    TokenCounter(TokenCounter &&other) noexcept;
    TokenCounter &operator=(const TokenCounter &other);
    TokenCounter &operator=(TokenCounter &&other) noexcept;
    ~TokenCounter();

    void swap(TokenCounter &other) noexcept { d.swap(other.d); }

    // A counter for whatever encoding ModelCatalog names for this model.
    static TokenCounter forModel(const QString &model);

    // Load a `.tiktoken` vocabulary, replacing any already loaded under that
    // name. Returns false and leaves the registry untouched if no line parses.
    // The pre-tokenizer regex is chosen from the encoding name; pass one
    // explicitly for an encoding this library does not know by name.
    static bool loadEncoding(const QString &name, const QByteArray &data,
                             const QString &pattern = QString());
    static bool loadEncodingFile(const QString &name, const QString &path,
                                 const QString &pattern = QString());

    static bool hasEncoding(const QString &name);
    static QStringList loadedEncodings();

    QString encoding() const;

    // Whether counts come from a real vocabulary rather than the estimate.
    bool isExact() const;

    // The token ranks for a string; empty when no vocabulary is loaded, since
    // the heuristic knows a count but not which tokens.
    QList<int> encode(const QString &text) const;

    int count(const QString &text) const;

    // A conversation as the chat models bill it: every message carries a fixed
    // framing overhead around its role and content, and the reply is primed
    // with a few more. Non-text content parts are counted for their text only;
    // an image's cost depends on tiling rules that live server-side.
    int count(const QList<Message> &messages) const;

    // What each message costs on its own, in order: element i is what
    // messages.at(i) contributes to count(messages). Their sum plus
    // requestOverhead() is count(messages) exactly.
    //
    // For weighing a conversation against a budget message by message. Asking
    // count() about one window after another re-counts every message the
    // windows have in common, which is every message but one -- see
    // Chat::TrimPolicy, which does exactly this walk.
    QList<int> countEach(const QList<Message> &messages) const;

    // What a request costs beyond its messages: the tokens that prime the
    // reply, charged once to a conversation that has any messages at all.
    static int requestOverhead();

private:
    QSharedDataPointer<TokenCounterData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::TokenCounter)
