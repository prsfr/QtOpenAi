// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/Enums.h>
#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <optional>

namespace QtOpenAi {
namespace Core {

class ToolCallChunkData;

// A fragment of a streamed tool call. Fragments sharing the same index are
// concatenated (arguments) to reconstruct a complete ToolCall.
class QTOPENAI_CORE_EXPORT ToolCallChunk
{
public:
    ToolCallChunk();
    ToolCallChunk(const ToolCallChunk &other);
    ToolCallChunk(ToolCallChunk &&other) noexcept;
    ToolCallChunk &operator=(const ToolCallChunk &other);
    ToolCallChunk &operator=(ToolCallChunk &&other) noexcept;
    ~ToolCallChunk();

    void swap(ToolCallChunk &other) noexcept { d.swap(other.d); }

    int index() const;
    void setIndex(int index);

    QString id() const;
    void setId(const QString &id);

    QString type() const;
    void setType(const QString &type);

    QString functionName() const;
    void setFunctionName(const QString &name);

    // Partial, model-generated argument fragment (concatenate across chunks).
    QString argumentsFragment() const;
    void setArgumentsFragment(const QString &fragment);

    QJsonObject toJson() const;
    static ToolCallChunk fromJson(const QJsonObject &json);

    bool operator==(const ToolCallChunk &other) const;
    bool operator!=(const ToolCallChunk &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ToolCallChunkData> d;
};

class ChoiceDeltaData;

// The incremental delta for one choice within a streamed chunk.
class QTOPENAI_CORE_EXPORT ChoiceDelta
{
public:
    ChoiceDelta();
    ChoiceDelta(const ChoiceDelta &other);
    ChoiceDelta(ChoiceDelta &&other) noexcept;
    ChoiceDelta &operator=(const ChoiceDelta &other);
    ChoiceDelta &operator=(ChoiceDelta &&other) noexcept;
    ~ChoiceDelta();

    void swap(ChoiceDelta &other) noexcept { d.swap(other.d); }

    // The role, present only on the first delta of a choice.
    std::optional<Role> role() const;
    void setRole(Role role);

    // The text fragment carried by this delta (may be empty).
    QString content() const;
    void setContent(const QString &content);
    bool hasContent() const;

    QList<ToolCallChunk> toolCalls() const;
    void setToolCalls(const QList<ToolCallChunk> &toolCalls);

    QString refusal() const;
    void setRefusal(const QString &refusal);

    QJsonObject toJson() const;
    static ChoiceDelta fromJson(const QJsonObject &json);

    bool operator==(const ChoiceDelta &other) const;
    bool operator!=(const ChoiceDelta &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChoiceDeltaData> d;
};

class ChunkChoiceData;

// One choice inside a streamed chunk: an index, a delta and a finish reason.
class QTOPENAI_CORE_EXPORT ChunkChoice
{
public:
    ChunkChoice();
    ChunkChoice(const ChunkChoice &other);
    ChunkChoice(ChunkChoice &&other) noexcept;
    ChunkChoice &operator=(const ChunkChoice &other);
    ChunkChoice &operator=(ChunkChoice &&other) noexcept;
    ~ChunkChoice();

    void swap(ChunkChoice &other) noexcept { d.swap(other.d); }

    int index() const;
    void setIndex(int index);

    ChoiceDelta delta() const;
    void setDelta(const ChoiceDelta &delta);

    FinishReason finishReason() const;
    void setFinishReason(FinishReason reason);

    QJsonObject toJson() const;
    static ChunkChoice fromJson(const QJsonObject &json);

    bool operator==(const ChunkChoice &other) const;
    bool operator!=(const ChunkChoice &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChunkChoiceData> d;
};

class ChatCompletionChunkData;

// A single `chat.completion.chunk` object from a streamed response.
class QTOPENAI_CORE_EXPORT ChatCompletionChunk
{
public:
    ChatCompletionChunk();
    ChatCompletionChunk(const ChatCompletionChunk &other);
    ChatCompletionChunk(ChatCompletionChunk &&other) noexcept;
    ChatCompletionChunk &operator=(const ChatCompletionChunk &other);
    ChatCompletionChunk &operator=(ChatCompletionChunk &&other) noexcept;
    ~ChatCompletionChunk();

    void swap(ChatCompletionChunk &other) noexcept { d.swap(other.d); }

    QString id() const;
    void setId(const QString &id);

    QString object() const;
    void setObject(const QString &object);

    qint64 created() const;
    void setCreated(qint64 created);

    QString model() const;
    void setModel(const QString &model);

    QString systemFingerprint() const;
    void setSystemFingerprint(const QString &fingerprint);

    QList<ChunkChoice> choices() const;
    void setChoices(const QList<ChunkChoice> &choices);

    QJsonObject toJson() const;
    static ChatCompletionChunk fromJson(const QJsonObject &json);

    bool operator==(const ChatCompletionChunk &other) const;
    bool operator!=(const ChatCompletionChunk &other) const { return !(*this == other); }

private:
    QSharedDataPointer<ChatCompletionChunkData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::ToolCallChunk)
Q_DECLARE_SHARED(QtOpenAi::Core::ChoiceDelta)
Q_DECLARE_SHARED(QtOpenAi::Core::ChunkChoice)
Q_DECLARE_SHARED(QtOpenAi::Core::ChatCompletionChunk)
Q_DECLARE_METATYPE(QtOpenAi::Core::ChatCompletionChunk)
