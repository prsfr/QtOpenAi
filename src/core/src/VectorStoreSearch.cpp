// SPDX-License-Identifier: MIT
#include "QtOpenAi/Core/VectorStoreSearch.h"

#include "JsonHelpers_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QSharedData>

namespace QtOpenAi {
namespace Core {

namespace {

// Read the `content` / `data` array of text chunks shared by both page types.
QList<VectorStoreContent> contentFromArray(const QJsonArray &array)
{
    QList<VectorStoreContent> content;
    for (const QJsonValue &value : array)
        content.append(VectorStoreContent::fromJson(value.toObject()));
    return content;
}

QJsonArray contentToArray(const QList<VectorStoreContent> &content)
{
    QJsonArray array;
    for (const VectorStoreContent &chunk : content)
        array.append(chunk.toJson());
    return array;
}

// Join the chunks' text, the form callers usually want to feed back to a model.
QString joinedText(const QList<VectorStoreContent> &content)
{
    QStringList parts;
    parts.reserve(content.size());
    for (const VectorStoreContent &chunk : content)
        parts.append(chunk.text);
    return parts.join(QLatin1Char('\n'));
}

} // namespace

// --- VectorStoreContent ----------------------------------------------------

QJsonObject VectorStoreContent::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("type"), type);
    detail::insertIfNotEmpty(json, QStringLiteral("text"), text);
    return json;
}

VectorStoreContent VectorStoreContent::fromJson(const QJsonObject &json)
{
    VectorStoreContent content;
    content.type = detail::stringOr(json, QStringLiteral("type"));
    content.text = detail::stringOr(json, QStringLiteral("text"));
    return content;
}

// --- VectorStoreSearchRequest ----------------------------------------------

class VectorStoreSearchRequestData : public QSharedData
{
public:
    QStringList query;
    std::optional<int> maxNumResults;
    std::optional<bool> rewriteQuery;
    QJsonObject filters;
    QJsonObject rankingOptions;
};

VectorStoreSearchRequest::VectorStoreSearchRequest()
    : d(new VectorStoreSearchRequestData)
{ }

VectorStoreSearchRequest::VectorStoreSearchRequest(QString query)
    : d(new VectorStoreSearchRequestData)
{
    d->query = QStringList {std::move(query)};
}

VectorStoreSearchRequest::VectorStoreSearchRequest(const VectorStoreSearchRequest &other) = default;
VectorStoreSearchRequest::VectorStoreSearchRequest(VectorStoreSearchRequest &&other) noexcept
        = default;
VectorStoreSearchRequest &VectorStoreSearchRequest::operator=(const VectorStoreSearchRequest &other)
        = default;
VectorStoreSearchRequest &
VectorStoreSearchRequest::operator=(VectorStoreSearchRequest &&other) noexcept
        = default;
VectorStoreSearchRequest::~VectorStoreSearchRequest() = default;

QStringList VectorStoreSearchRequest::query() const { return d->query; }
void VectorStoreSearchRequest::setQuery(const QStringList &query) { d->query = query; }
void VectorStoreSearchRequest::setQuery(const QString &query) { d->query = QStringList {query}; }

std::optional<int> VectorStoreSearchRequest::maxNumResults() const { return d->maxNumResults; }
void VectorStoreSearchRequest::setMaxNumResults(int maxNumResults)
{
    d->maxNumResults = maxNumResults;
}

std::optional<bool> VectorStoreSearchRequest::rewriteQuery() const { return d->rewriteQuery; }
void VectorStoreSearchRequest::setRewriteQuery(bool rewriteQuery)
{
    d->rewriteQuery = rewriteQuery;
}

QJsonObject VectorStoreSearchRequest::filters() const { return d->filters; }
void VectorStoreSearchRequest::setFilters(const QJsonObject &filters) { d->filters = filters; }

QJsonObject VectorStoreSearchRequest::rankingOptions() const { return d->rankingOptions; }
void VectorStoreSearchRequest::setRankingOptions(const QJsonObject &rankingOptions)
{
    d->rankingOptions = rankingOptions;
}

QJsonObject VectorStoreSearchRequest::toJson() const
{
    QJsonObject json;
    // A single query goes out as a plain string, several as an array.
    if (d->query.size() == 1) {
        json.insert(QStringLiteral("query"), d->query.first());
    } else if (!d->query.isEmpty()) {
        QJsonArray query;
        for (const QString &term : d->query)
            query.append(term);
        json.insert(QStringLiteral("query"), query);
    }
    if (d->maxNumResults)
        json.insert(QStringLiteral("max_num_results"), *d->maxNumResults);
    if (d->rewriteQuery)
        json.insert(QStringLiteral("rewrite_query"), *d->rewriteQuery);
    if (!d->filters.isEmpty())
        json.insert(QStringLiteral("filters"), d->filters);
    if (!d->rankingOptions.isEmpty())
        json.insert(QStringLiteral("ranking_options"), d->rankingOptions);
    return json;
}

bool VectorStoreSearchRequest::operator==(const VectorStoreSearchRequest &other) const
{
    return d->query == other.d->query && d->maxNumResults == other.d->maxNumResults
           && d->rewriteQuery == other.d->rewriteQuery && d->filters == other.d->filters
           && d->rankingOptions == other.d->rankingOptions;
}

// --- VectorStoreSearchResult -----------------------------------------------

class VectorStoreSearchResultData : public QSharedData
{
public:
    QString fileId;
    QString filename;
    double score = 0.0;
    QJsonObject attributes;
    QList<VectorStoreContent> content;
};

VectorStoreSearchResult::VectorStoreSearchResult()
    : d(new VectorStoreSearchResultData)
{ }

VectorStoreSearchResult::VectorStoreSearchResult(const VectorStoreSearchResult &other) = default;
VectorStoreSearchResult::VectorStoreSearchResult(VectorStoreSearchResult &&other) noexcept
        = default;
VectorStoreSearchResult &VectorStoreSearchResult::operator=(const VectorStoreSearchResult &other)
        = default;
VectorStoreSearchResult &
VectorStoreSearchResult::operator=(VectorStoreSearchResult &&other) noexcept
        = default;
VectorStoreSearchResult::~VectorStoreSearchResult() = default;

QString VectorStoreSearchResult::fileId() const { return d->fileId; }
void VectorStoreSearchResult::setFileId(const QString &fileId) { d->fileId = fileId; }

QString VectorStoreSearchResult::filename() const { return d->filename; }
void VectorStoreSearchResult::setFilename(const QString &filename) { d->filename = filename; }

double VectorStoreSearchResult::score() const { return d->score; }
void VectorStoreSearchResult::setScore(double score) { d->score = score; }

QJsonObject VectorStoreSearchResult::attributes() const { return d->attributes; }
void VectorStoreSearchResult::setAttributes(const QJsonObject &attributes)
{
    d->attributes = attributes;
}

QList<VectorStoreContent> VectorStoreSearchResult::content() const { return d->content; }
void VectorStoreSearchResult::setContent(const QList<VectorStoreContent> &content)
{
    d->content = content;
}

QString VectorStoreSearchResult::text() const { return joinedText(d->content); }

QJsonObject VectorStoreSearchResult::toJson() const
{
    QJsonObject json;
    detail::insertIfNotEmpty(json, QStringLiteral("file_id"), d->fileId);
    detail::insertIfNotEmpty(json, QStringLiteral("filename"), d->filename);
    json.insert(QStringLiteral("score"), d->score);
    if (!d->attributes.isEmpty())
        json.insert(QStringLiteral("attributes"), d->attributes);
    json.insert(QStringLiteral("content"), contentToArray(d->content));
    return json;
}

VectorStoreSearchResult VectorStoreSearchResult::fromJson(const QJsonObject &json)
{
    VectorStoreSearchResult result;
    result.d->fileId = detail::stringOr(json, QStringLiteral("file_id"));
    result.d->filename = detail::stringOr(json, QStringLiteral("filename"));
    result.d->score = json.value(QStringLiteral("score")).toDouble();
    result.d->attributes = json.value(QStringLiteral("attributes")).toObject();
    result.d->content = contentFromArray(json.value(QStringLiteral("content")).toArray());
    return result;
}

bool VectorStoreSearchResult::operator==(const VectorStoreSearchResult &other) const
{
    return d->fileId == other.d->fileId && d->filename == other.d->filename
           && qFuzzyCompare(d->score, other.d->score) && d->attributes == other.d->attributes
           && d->content == other.d->content;
}

// --- VectorStoreSearchPage -------------------------------------------------

QJsonObject VectorStoreSearchPage::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("object"), QStringLiteral("vector_store.search_results.page"));
    QJsonArray query;
    for (const QString &term : searchQuery)
        query.append(term);
    json.insert(QStringLiteral("search_query"), query);
    QJsonArray results;
    for (const VectorStoreSearchResult &result : data)
        results.append(result.toJson());
    json.insert(QStringLiteral("data"), results);
    json.insert(QStringLiteral("has_more"), hasMore);
    detail::insertIfNotEmpty(json, QStringLiteral("next_page"), nextPage);
    return json;
}

VectorStoreSearchPage VectorStoreSearchPage::fromJson(const QJsonObject &json)
{
    VectorStoreSearchPage page;
    const QJsonArray query = json.value(QStringLiteral("search_query")).toArray();
    for (const QJsonValue &value : query)
        page.searchQuery.append(value.toString());
    const QJsonArray results = json.value(QStringLiteral("data")).toArray();
    for (const QJsonValue &value : results)
        page.data.append(VectorStoreSearchResult::fromJson(value.toObject()));
    page.hasMore = json.value(QStringLiteral("has_more")).toBool();
    page.nextPage = detail::stringOr(json, QStringLiteral("next_page"));
    return page;
}

// --- VectorStoreFileContentPage --------------------------------------------

QString VectorStoreFileContentPage::text() const { return joinedText(data); }

QJsonObject VectorStoreFileContentPage::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("object"), QStringLiteral("vector_store.file_content.page"));
    json.insert(QStringLiteral("data"), contentToArray(data));
    json.insert(QStringLiteral("has_more"), hasMore);
    detail::insertIfNotEmpty(json, QStringLiteral("next_page"), nextPage);
    return json;
}

VectorStoreFileContentPage VectorStoreFileContentPage::fromJson(const QJsonObject &json)
{
    VectorStoreFileContentPage page;
    page.data = contentFromArray(json.value(QStringLiteral("data")).toArray());
    page.hasMore = json.value(QStringLiteral("has_more")).toBool();
    page.nextPage = detail::stringOr(json, QStringLiteral("next_page"));
    return page;
}

} // namespace Core
} // namespace QtOpenAi
