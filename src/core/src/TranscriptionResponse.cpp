// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/TranscriptionResponse.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

// --- TranscriptionWord -----------------------------------------------------

class TranscriptionWordData : public QSharedData
{
public:
    QString word;
    double start = 0.0;
    double end = 0.0;
};

TranscriptionWord::TranscriptionWord()
    : d(new TranscriptionWordData)
{ }

TranscriptionWord::TranscriptionWord(const TranscriptionWord &other) = default;
TranscriptionWord::TranscriptionWord(TranscriptionWord &&other) noexcept = default;
TranscriptionWord &TranscriptionWord::operator=(const TranscriptionWord &other) = default;
TranscriptionWord &TranscriptionWord::operator=(TranscriptionWord &&other) noexcept = default;
TranscriptionWord::~TranscriptionWord() = default;

QString TranscriptionWord::word() const { return d->word; }
void TranscriptionWord::setWord(const QString &word) { d->word = word; }

double TranscriptionWord::start() const { return d->start; }
void TranscriptionWord::setStart(double start) { d->start = start; }

double TranscriptionWord::end() const { return d->end; }
void TranscriptionWord::setEnd(double end) { d->end = end; }

QJsonObject TranscriptionWord::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("word"), d->word);
    json.insert(QStringLiteral("start"), d->start);
    json.insert(QStringLiteral("end"), d->end);
    return json;
}

TranscriptionWord TranscriptionWord::fromJson(const QJsonObject &json)
{
    TranscriptionWord word;
    word.d->word = detail::stringOr(json, QStringLiteral("word"));
    word.d->start = json.value(QStringLiteral("start")).toDouble();
    word.d->end = json.value(QStringLiteral("end")).toDouble();
    return word;
}

bool TranscriptionWord::operator==(const TranscriptionWord &other) const
{
    return d->word == other.d->word && d->start == other.d->start && d->end == other.d->end;
}

// --- TranscriptionSegment --------------------------------------------------

class TranscriptionSegmentData : public QSharedData
{
public:
    int id = 0;
    double start = 0.0;
    double end = 0.0;
    QString text;
    double avgLogprob = 0.0;
};

TranscriptionSegment::TranscriptionSegment()
    : d(new TranscriptionSegmentData)
{ }

TranscriptionSegment::TranscriptionSegment(const TranscriptionSegment &other) = default;
TranscriptionSegment::TranscriptionSegment(TranscriptionSegment &&other) noexcept = default;
TranscriptionSegment &TranscriptionSegment::operator=(const TranscriptionSegment &other) = default;
TranscriptionSegment &TranscriptionSegment::operator=(TranscriptionSegment &&other) noexcept
        = default;
TranscriptionSegment::~TranscriptionSegment() = default;

int TranscriptionSegment::id() const { return d->id; }
void TranscriptionSegment::setId(int id) { d->id = id; }

double TranscriptionSegment::start() const { return d->start; }
void TranscriptionSegment::setStart(double start) { d->start = start; }

double TranscriptionSegment::end() const { return d->end; }
void TranscriptionSegment::setEnd(double end) { d->end = end; }

QString TranscriptionSegment::text() const { return d->text; }
void TranscriptionSegment::setText(const QString &text) { d->text = text; }

double TranscriptionSegment::avgLogprob() const { return d->avgLogprob; }
void TranscriptionSegment::setAvgLogprob(double avgLogprob) { d->avgLogprob = avgLogprob; }

QJsonObject TranscriptionSegment::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), d->id);
    json.insert(QStringLiteral("start"), d->start);
    json.insert(QStringLiteral("end"), d->end);
    json.insert(QStringLiteral("text"), d->text);
    json.insert(QStringLiteral("avg_logprob"), d->avgLogprob);
    return json;
}

TranscriptionSegment TranscriptionSegment::fromJson(const QJsonObject &json)
{
    TranscriptionSegment segment;
    segment.d->id = json.value(QStringLiteral("id")).toInt();
    segment.d->start = json.value(QStringLiteral("start")).toDouble();
    segment.d->end = json.value(QStringLiteral("end")).toDouble();
    segment.d->text = detail::stringOr(json, QStringLiteral("text"));
    segment.d->avgLogprob = json.value(QStringLiteral("avg_logprob")).toDouble();
    return segment;
}

bool TranscriptionSegment::operator==(const TranscriptionSegment &other) const
{
    return d->id == other.d->id && d->start == other.d->start && d->end == other.d->end
           && d->text == other.d->text && d->avgLogprob == other.d->avgLogprob;
}

// --- TranscriptionResponse -------------------------------------------------

class TranscriptionResponseData : public QSharedData
{
public:
    QString text;
    QString language;
    double duration = 0.0;
    QList<TranscriptionSegment> segments;
    QList<TranscriptionWord> words;
};

TranscriptionResponse::TranscriptionResponse()
    : d(new TranscriptionResponseData)
{ }

TranscriptionResponse::TranscriptionResponse(const TranscriptionResponse &other) = default;
TranscriptionResponse::TranscriptionResponse(TranscriptionResponse &&other) noexcept = default;
TranscriptionResponse &TranscriptionResponse::operator=(const TranscriptionResponse &other)
        = default;
TranscriptionResponse &TranscriptionResponse::operator=(TranscriptionResponse &&other) noexcept
        = default;
TranscriptionResponse::~TranscriptionResponse() = default;

QString TranscriptionResponse::text() const { return d->text; }
void TranscriptionResponse::setText(const QString &text) { d->text = text; }

QString TranscriptionResponse::language() const { return d->language; }
void TranscriptionResponse::setLanguage(const QString &language) { d->language = language; }

double TranscriptionResponse::duration() const { return d->duration; }
void TranscriptionResponse::setDuration(double duration) { d->duration = duration; }

QList<TranscriptionSegment> TranscriptionResponse::segments() const { return d->segments; }
void TranscriptionResponse::setSegments(const QList<TranscriptionSegment> &segments)
{
    d->segments = segments;
}

QList<TranscriptionWord> TranscriptionResponse::words() const { return d->words; }
void TranscriptionResponse::setWords(const QList<TranscriptionWord> &words) { d->words = words; }

QJsonObject TranscriptionResponse::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("text"), d->text);
    detail::insertIfNotEmpty(json, QStringLiteral("language"), d->language);
    if (d->duration != 0.0)
        json.insert(QStringLiteral("duration"), d->duration);
    if (!d->segments.isEmpty()) {
        QJsonArray segments;
        for (const TranscriptionSegment &segment : d->segments)
            segments.append(segment.toJson());
        json.insert(QStringLiteral("segments"), segments);
    }
    if (!d->words.isEmpty()) {
        QJsonArray words;
        for (const TranscriptionWord &word : d->words)
            words.append(word.toJson());
        json.insert(QStringLiteral("words"), words);
    }
    return json;
}

TranscriptionResponse TranscriptionResponse::fromJson(const QJsonObject &json)
{
    TranscriptionResponse response;
    response.d->text = detail::stringOr(json, QStringLiteral("text"));
    response.d->language = detail::stringOr(json, QStringLiteral("language"));
    response.d->duration = json.value(QStringLiteral("duration")).toDouble();
    const QJsonArray segments = json.value(QStringLiteral("segments")).toArray();
    for (const QJsonValue &value : segments)
        response.d->segments.append(TranscriptionSegment::fromJson(value.toObject()));
    const QJsonArray words = json.value(QStringLiteral("words")).toArray();
    for (const QJsonValue &value : words)
        response.d->words.append(TranscriptionWord::fromJson(value.toObject()));
    return response;
}

TranscriptionResponse TranscriptionResponse::fromText(const QString &text)
{
    TranscriptionResponse response;
    response.d->text = text;
    return response;
}

bool TranscriptionResponse::operator==(const TranscriptionResponse &other) const
{
    return d->text == other.d->text && d->language == other.d->language
           && d->duration == other.d->duration && d->segments == other.d->segments
           && d->words == other.d->words;
}

} // namespace Core
} // namespace QtOpenAi
