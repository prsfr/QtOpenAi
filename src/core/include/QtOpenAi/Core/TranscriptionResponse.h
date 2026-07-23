// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace QtOpenAi {
namespace Core {

class TranscriptionWordData;

// A single time-stamped word (verbose_json with word granularity).
class QTOPENAI_CORE_EXPORT TranscriptionWord
{
public:
    TranscriptionWord();
    TranscriptionWord(const TranscriptionWord &other);
    TranscriptionWord(TranscriptionWord &&other) noexcept;
    TranscriptionWord &operator=(const TranscriptionWord &other);
    TranscriptionWord &operator=(TranscriptionWord &&other) noexcept;
    ~TranscriptionWord();

    void swap(TranscriptionWord &other) noexcept { d.swap(other.d); }

    QString word() const;
    void setWord(const QString &word);

    double start() const;
    void setStart(double start);

    double end() const;
    void setEnd(double end);

    QJsonObject toJson() const;
    static TranscriptionWord fromJson(const QJsonObject &json);

    bool operator==(const TranscriptionWord &other) const;
    bool operator!=(const TranscriptionWord &other) const { return !(*this == other); }

private:
    QSharedDataPointer<TranscriptionWordData> d;
};

class TranscriptionSegmentData;

// A time-stamped segment (verbose_json with segment granularity).
class QTOPENAI_CORE_EXPORT TranscriptionSegment
{
public:
    TranscriptionSegment();
    TranscriptionSegment(const TranscriptionSegment &other);
    TranscriptionSegment(TranscriptionSegment &&other) noexcept;
    TranscriptionSegment &operator=(const TranscriptionSegment &other);
    TranscriptionSegment &operator=(TranscriptionSegment &&other) noexcept;
    ~TranscriptionSegment();

    void swap(TranscriptionSegment &other) noexcept { d.swap(other.d); }

    int id() const;
    void setId(int id);

    double start() const;
    void setStart(double start);

    double end() const;
    void setEnd(double end);

    QString text() const;
    void setText(const QString &text);

    double avgLogprob() const;
    void setAvgLogprob(double avgLogprob);

    QJsonObject toJson() const;
    static TranscriptionSegment fromJson(const QJsonObject &json);

    bool operator==(const TranscriptionSegment &other) const;
    bool operator!=(const TranscriptionSegment &other) const { return !(*this == other); }

private:
    QSharedDataPointer<TranscriptionSegmentData> d;
};

class TranscriptionResponseData;

// A parsed speech-to-text result (POST /audio/transcriptions or
// /audio/translations). For the plain `text`/`srt`/`vtt` response formats only
// text() is populated; `verbose_json` additionally fills language/duration and
// the segments/words lists.
class QTOPENAI_CORE_EXPORT TranscriptionResponse
{
public:
    TranscriptionResponse();
    TranscriptionResponse(const TranscriptionResponse &other);
    TranscriptionResponse(TranscriptionResponse &&other) noexcept;
    TranscriptionResponse &operator=(const TranscriptionResponse &other);
    TranscriptionResponse &operator=(TranscriptionResponse &&other) noexcept;
    ~TranscriptionResponse();

    void swap(TranscriptionResponse &other) noexcept { d.swap(other.d); }

    QString text() const;
    void setText(const QString &text);

    QString language() const;
    void setLanguage(const QString &language);

    // Audio duration in seconds (verbose_json); 0 when absent.
    double duration() const;
    void setDuration(double duration);

    QList<TranscriptionSegment> segments() const;
    void setSegments(const QList<TranscriptionSegment> &segments);

    QList<TranscriptionWord> words() const;
    void setWords(const QList<TranscriptionWord> &words);

    QJsonObject toJson() const;
    static TranscriptionResponse fromJson(const QJsonObject &json);
    // Build a plain-text result (text/srt/vtt response formats).
    static TranscriptionResponse fromText(const QString &text);

    bool operator==(const TranscriptionResponse &other) const;
    bool operator!=(const TranscriptionResponse &other) const { return !(*this == other); }

private:
    QSharedDataPointer<TranscriptionResponseData> d;
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::TranscriptionWord)
Q_DECLARE_SHARED(QtOpenAi::Core::TranscriptionSegment)
Q_DECLARE_SHARED(QtOpenAi::Core::TranscriptionResponse)
