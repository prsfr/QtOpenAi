// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/TokenCounter.h"

#include "QtOpenAi/Core/Enums.h"
#include "QtOpenAi/Core/ModelCatalog.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QRegularExpression>
#include <QtCore/QSharedData>

#include <memory>

namespace QtOpenAi {
namespace Core {

namespace {

// --- Framing overhead -------------------------------------------------------
//
// A chat message is not just its text: the model sees each one wrapped in role
// and separator tokens, and the reply is primed with a few more. These are the
// figures OpenAI documents for the chat models.
constexpr int kTokensPerMessage = 3;
constexpr int kTokensPerName = 1;
constexpr int kTokensForReplyPriming = 3;

// The customary estimate when no vocabulary is loaded: English averages about
// four characters to the token.
constexpr int kCharactersPerToken = 4;

// --- Pre-tokenizer patterns -------------------------------------------------
//
// Byte-pair merging never crosses a piece boundary, so the split has to match
// the one the encoding was built with. Each generation of OpenAI encodings uses
// its own pattern.
struct PatternForEncoding
{
    QLatin1String encoding;
    QLatin1String pattern;
};

constexpr QLatin1String kO200kPattern(
        R"([^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]*[\p{Ll}\p{Lm}\p{Lo}\p{M}]+(?i:'s|'t|'re|'ve|'m|'ll|'d)?|)"
        R"([^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]+[\p{Ll}\p{Lm}\p{Lo}\p{M}]*(?i:'s|'t|'re|'ve|'m|'ll|'d)?|)"
        R"(\p{N}{1,3}| ?[^\s\p{L}\p{N}]+[\r\n/]*|\s*[\r\n]|\s+(?!\S)|\s+)");

constexpr QLatin1String
        kCl100kPattern(R"((?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}{1,3}|)"
                       R"( ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+)");

// The GPT-2 pattern, used by the 50k encodings.
constexpr QLatin1String kLegacyPattern(
        R"('s|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+)");

constexpr PatternForEncoding kPatterns[] = {
        {QLatin1String("o200k_base"), kO200kPattern},
        {QLatin1String("cl100k_base"), kCl100kPattern},
        {QLatin1String("p50k_base"), kLegacyPattern},
        {QLatin1String("p50k_edit"), kLegacyPattern},
        {QLatin1String("r50k_base"), kLegacyPattern},
        {QLatin1String("gpt2"), kLegacyPattern},
};

QString patternFor(const QString &encoding)
{
    for (const PatternForEncoding &entry : kPatterns) {
        if (encoding == entry.encoding)
            return QString(entry.pattern);
    }
    // An encoding this library has never heard of is most likely a current one.
    return QString(kO200kPattern);
}

// --- The loaded vocabularies ------------------------------------------------

struct Encoding
{
    QHash<QByteArray, int> ranks;
    QRegularExpression pattern;
};

using EncodingPtr = std::shared_ptr<const Encoding>;

// Counters are copied and shared freely, so the registry has to be safe to read
// from more than one thread.
QMutex &registryMutex()
{
    static QMutex mutex;
    return mutex;
}

QHash<QString, EncodingPtr> &registry()
{
    static QHash<QString, EncodingPtr> encodings;
    return encodings;
}

EncodingPtr lookup(const QString &name)
{
    if (name.isEmpty())
        return {};
    QMutexLocker locker(&registryMutex());
    return registry().value(name);
}

// --- Byte-pair encoding -----------------------------------------------------

// Merge the bytes of one piece into tokens, always taking the lowest-ranked
// adjacent pair first -- the order that reproduces the vocabulary's own
// construction, and so the server's count.
//
// Returns how many tokens the piece cost. `tokens` collects their ranks when a
// caller wants them and may be null: counting is by far the commoner request,
// and it has no use for the ranks themselves.
int bytePairEncode(const QByteArray &piece, const Encoding &encoding, QList<int> *tokens)
{
    if (piece.isEmpty())
        return 0;

    const auto rankOf = [&](int start, int end) {
        return encoding.ranks.value(QByteArray::fromRawData(piece.constData() + start, end - start),
                                    -1);
    };

    // A piece that is a token already needs no merging at all, which is the
    // common case for ordinary words.
    if (const int whole = rankOf(0, piece.size()); whole >= 0) {
        if (tokens)
            tokens->append(whole);
        return 1;
    }

    // Boundaries between the tokens found so far; every byte starts as its own.
    QList<int> boundaries;
    boundaries.reserve(piece.size() + 1);
    for (int i = 0; i <= piece.size(); ++i)
        boundaries.append(i);

    // The rank of merging the token at boundaries[i] with the one after it,
    // kept alongside the boundaries themselves. A merge changes the score of at
    // most two pairs, so scoring every pair again on every pass -- which is
    // what this list replaces -- was asking the vocabulary O(n^2) questions to
    // learn something that changes O(1) times per merge.
    const auto pairRankAt = [&](int i) {
        return i + 2 < boundaries.size() ? rankOf(boundaries.at(i), boundaries.at(i + 2)) : -1;
    };
    QList<int> pairRank(boundaries.size(), -1);
    for (int i = 0; i + 2 < boundaries.size(); ++i)
        pairRank[i] = pairRankAt(i);

    while (boundaries.size() > 2) {
        int bestIndex = -1;
        int bestRank = -1;
        for (int i = 0; i + 2 < boundaries.size(); ++i) {
            const int rank = pairRank.at(i);
            if (rank >= 0 && (bestRank < 0 || rank < bestRank)) {
                bestRank = rank;
                bestIndex = i;
            }
        }
        if (bestIndex < 0)
            break;

        boundaries.removeAt(bestIndex + 1);
        pairRank.removeAt(bestIndex + 1);

        // The merged token has a new right-hand neighbour, and it is itself the
        // new right-hand side of the pair before it. Every other pair spans the
        // same bytes it did and scores the same.
        pairRank[bestIndex] = pairRankAt(bestIndex);
        if (bestIndex > 0)
            pairRank[bestIndex - 1] = pairRankAt(bestIndex - 1);
    }

    if (tokens) {
        for (int i = 0; i + 1 < boundaries.size(); ++i) {
            // A byte outside the vocabulary still costs a token; -1 marks it as
            // one this vocabulary cannot name.
            tokens->append(rankOf(boundaries.at(i), boundaries.at(i + 1)));
        }
    }
    return int(boundaries.size()) - 1;
}

int heuristicCount(const QString &text)
{
    if (text.isEmpty())
        return 0;
    return qMax(1, (int(text.size()) + kCharactersPerToken - 1) / kCharactersPerToken);
}

// Split `text` with the encoding's pre-tokenizer and merge each piece. Returns
// the token count; `tokens` collects the ranks when a caller wants them.
int encodeWith(const Encoding &encoding, const QString &text, QList<int> *tokens)
{
    int total = 0;
    QRegularExpressionMatchIterator pieces = encoding.pattern.globalMatch(text);
    while (pieces.hasNext())
        total += bytePairEncode(pieces.next().captured().toUtf8(), encoding, tokens);
    return total;
}

// What one string costs against a vocabulary already resolved -- exactly when
// there is one, and by the four-characters-to-the-token estimate when there is
// not. Taking the vocabulary as an argument is what lets a caller counting many
// strings look it up once; see TokenCounter::count(const QList<Message> &).
int countWith(const Encoding *encoding, const QString &text)
{
    return encoding ? encodeWith(*encoding, text, nullptr) : heuristicCount(text);
}

// What one message contributes to a request, framing included but the
// once-per-request reply priming excluded -- that belongs to the conversation,
// not to any message in it.
int countMessageWith(const Encoding *encoding, const Message &message)
{
    int total = kTokensPerMessage;
    total += countWith(encoding, roleToString(message.role()));
    total += countWith(encoding, message.content());
    if (!message.name().isEmpty())
        total += kTokensPerName + countWith(encoding, message.name());
    if (!message.refusal().isEmpty())
        total += countWith(encoding, message.refusal());
    // A tool call is billed for the JSON that carries it.
    for (const ToolCall &call : message.toolCalls()) {
        total += countWith(encoding, call.function().name());
        total += countWith(encoding, call.function().arguments());
    }
    return total;
}

} // namespace

class TokenCounterData : public QSharedData
{
public:
    QString encoding;

    // Resolved per call rather than cached, so a counter built before its
    // vocabulary was loaded starts being exact the moment it is.
    EncodingPtr vocabulary() const { return lookup(encoding); }
};

TokenCounter::TokenCounter()
    : d(new TokenCounterData)
{ }

TokenCounter::TokenCounter(const QString &encoding)
    : d(new TokenCounterData)
{
    d->encoding = encoding;
}

TokenCounter::TokenCounter(const TokenCounter &other) = default;
TokenCounter::TokenCounter(TokenCounter &&other) noexcept = default;
TokenCounter &TokenCounter::operator=(const TokenCounter &other) = default;
TokenCounter &TokenCounter::operator=(TokenCounter &&other) noexcept = default;
TokenCounter::~TokenCounter() = default;

TokenCounter TokenCounter::forModel(const QString &model)
{
    return TokenCounter(ModelCatalog::shared().model(model).encoding());
}

bool TokenCounter::loadEncoding(const QString &name, const QByteArray &data, const QString &pattern)
{
    if (name.isEmpty())
        return false;

    auto encoding = std::make_shared<Encoding>();
    encoding->pattern = QRegularExpression(pattern.isEmpty() ? patternFor(name) : pattern,
                                           QRegularExpression::UseUnicodePropertiesOption);
    if (!encoding->pattern.isValid())
        return false;

    // tiktoken's format: one "<base64 token> <rank>" per line.
    for (const QByteArray &line : data.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        const int space = trimmed.lastIndexOf(' ');
        if (space <= 0)
            continue;
        bool ok = false;
        const int rank = trimmed.mid(space + 1).toInt(&ok);
        if (!ok)
            continue;
        encoding->ranks.insert(QByteArray::fromBase64(trimmed.left(space)), rank);
    }

    if (encoding->ranks.isEmpty())
        return false;

    QMutexLocker locker(&registryMutex());
    registry().insert(name, std::move(encoding));
    return true;
}

bool TokenCounter::loadEncodingFile(const QString &name, const QString &path,
                                    const QString &pattern)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    return loadEncoding(name, file.readAll(), pattern);
}

bool TokenCounter::hasEncoding(const QString &name)
{
    QMutexLocker locker(&registryMutex());
    return registry().contains(name);
}

QStringList TokenCounter::loadedEncodings()
{
    QMutexLocker locker(&registryMutex());
    QStringList names = registry().keys();
    names.sort();
    return names;
}

QString TokenCounter::encoding() const { return d->encoding; }

bool TokenCounter::isExact() const { return d->vocabulary() != nullptr; }

QList<int> TokenCounter::encode(const QString &text) const
{
    const EncodingPtr encoding = d->vocabulary();
    if (!encoding || text.isEmpty())
        return {};

    QList<int> tokens;
    encodeWith(*encoding, text, &tokens);
    return tokens;
}

int TokenCounter::count(const QString &text) const
{
    // One resolution, not two: going through isExact() and then encode() took
    // the registry's mutex twice to answer one question.
    const EncodingPtr encoding = d->vocabulary();
    return countWith(encoding.get(), text);
}

int TokenCounter::count(const QList<Message> &messages) const
{
    if (messages.isEmpty())
        return 0;

    // Resolved once for the whole conversation. The registry is shared between
    // threads and so guarded by a mutex; asking it again for every field of
    // every message made counting a long transcript take that mutex a thousand
    // times to learn the same answer.
    const EncodingPtr vocabulary = d->vocabulary();

    int total = 0;
    for (const Message &message : messages)
        total += countMessageWith(vocabulary.get(), message);
    return total + kTokensForReplyPriming;
}

QList<int> TokenCounter::countEach(const QList<Message> &messages) const
{
    const EncodingPtr vocabulary = d->vocabulary();

    QList<int> costs;
    costs.reserve(messages.size());
    for (const Message &message : messages)
        costs.append(countMessageWith(vocabulary.get(), message));
    return costs;
}

int TokenCounter::requestOverhead() { return kTokensForReplyPriming; }

} // namespace Core
} // namespace QtOpenAi
