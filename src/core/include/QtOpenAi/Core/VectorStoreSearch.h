// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Core/GlobalCore.h>

#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>

namespace QtOpenAi {
namespace Core {

// One chunk of retrieved text. The same shape carries both a search hit's
// excerpts and the raw contents of a file in a store, so both endpoints share it.
struct QTOPENAI_CORE_EXPORT VectorStoreContent
{
    QString type; // currently always "text"
    QString text;

    QJsonObject toJson() const;
    static VectorStoreContent fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreContent &other) const
    {
        return type == other.type && text == other.text;
    }
    bool operator!=(const VectorStoreContent &other) const { return !(*this == other); }
};

class VectorStoreSearchRequestData;

// The body of a POST /vector_stores/{id}/search request.
//
// The API accepts either a single query string or an array of them; this type
// always carries a list and serialises a one-element list as a plain string, the
// form the API documents for the common case.
class QTOPENAI_CORE_EXPORT VectorStoreSearchRequest
{
public:
    VectorStoreSearchRequest();
    explicit VectorStoreSearchRequest(QString query);
    VectorStoreSearchRequest(const VectorStoreSearchRequest &other);
    VectorStoreSearchRequest(VectorStoreSearchRequest &&other) noexcept;
    VectorStoreSearchRequest &operator=(const VectorStoreSearchRequest &other);
    VectorStoreSearchRequest &operator=(VectorStoreSearchRequest &&other) noexcept;
    ~VectorStoreSearchRequest();

    void swap(VectorStoreSearchRequest &other) noexcept { d.swap(other.d); }

    QStringList query() const;
    void setQuery(const QStringList &query);
    void setQuery(const QString &query);

    // Upper bound on returned chunks (1-50); unset leaves the server default.
    std::optional<int> maxNumResults() const;
    void setMaxNumResults(int maxNumResults);

    // Whether the server may rewrite the query for better vector search.
    std::optional<bool> rewriteQuery() const;
    void setRewriteQuery(bool rewriteQuery);

    // An attribute filter, kept as raw JSON because the comparison/compound
    // filter grammar keeps growing, e.g.
    // {"type": "eq", "key": "region", "value": "eu"}.
    QJsonObject filters() const;
    void setFilters(const QJsonObject &filters);

    // Ranking options, raw JSON for the same reason, e.g.
    // {"ranker": "auto", "score_threshold": 0.5}.
    QJsonObject rankingOptions() const;
    void setRankingOptions(const QJsonObject &rankingOptions);

    QJsonObject toJson() const;

    bool operator==(const VectorStoreSearchRequest &other) const;
    bool operator!=(const VectorStoreSearchRequest &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VectorStoreSearchRequestData> d;
};

class VectorStoreSearchResultData;

// One ranked hit: the file it came from, its relevance score, the caller's
// attributes for that file, and the matching text chunks.
class QTOPENAI_CORE_EXPORT VectorStoreSearchResult
{
public:
    VectorStoreSearchResult();
    VectorStoreSearchResult(const VectorStoreSearchResult &other);
    VectorStoreSearchResult(VectorStoreSearchResult &&other) noexcept;
    VectorStoreSearchResult &operator=(const VectorStoreSearchResult &other);
    VectorStoreSearchResult &operator=(VectorStoreSearchResult &&other) noexcept;
    ~VectorStoreSearchResult();

    void swap(VectorStoreSearchResult &other) noexcept { d.swap(other.d); }

    QString fileId() const;
    void setFileId(const QString &fileId);

    QString filename() const;
    void setFilename(const QString &filename);

    // Relevance in [0, 1]; higher is a better match.
    double score() const;
    void setScore(double score);

    QJsonObject attributes() const;
    void setAttributes(const QJsonObject &attributes);

    QList<VectorStoreContent> content() const;
    void setContent(const QList<VectorStoreContent> &content);

    // Convenience: every chunk's text joined with newlines.
    QString text() const;

    QJsonObject toJson() const;
    static VectorStoreSearchResult fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreSearchResult &other) const;
    bool operator!=(const VectorStoreSearchResult &other) const { return !(*this == other); }

private:
    QSharedDataPointer<VectorStoreSearchResultData> d;
};

// A page of search hits. Unlike the cursor-paginated `list` objects this one
// pages with an opaque `next_page` token and echoes the effective query, so it
// cannot reuse ListPage.
struct QTOPENAI_CORE_EXPORT VectorStoreSearchPage
{
    // The query actually run, after any server-side rewrite.
    QStringList searchQuery;
    QList<VectorStoreSearchResult> data;
    bool hasMore = false;
    QString nextPage;

    bool isEmpty() const { return data.isEmpty(); }
    int size() const { return data.size(); }

    QJsonObject toJson() const;
    static VectorStoreSearchPage fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreSearchPage &other) const
    {
        return searchQuery == other.searchQuery && data == other.data && hasMore == other.hasMore
               && nextPage == other.nextPage;
    }
    bool operator!=(const VectorStoreSearchPage &other) const { return !(*this == other); }
};

// A page of a file's parsed contents
// (GET /vector_stores/{id}/files/{file_id}/content). Paged like the search
// results, with the same `next_page` token.
struct QTOPENAI_CORE_EXPORT VectorStoreFileContentPage
{
    QList<VectorStoreContent> data;
    bool hasMore = false;
    QString nextPage;

    bool isEmpty() const { return data.isEmpty(); }
    int size() const { return data.size(); }

    // Convenience: every chunk's text joined with newlines.
    QString text() const;

    QJsonObject toJson() const;
    static VectorStoreFileContentPage fromJson(const QJsonObject &json);

    bool operator==(const VectorStoreFileContentPage &other) const
    {
        return data == other.data && hasMore == other.hasMore && nextPage == other.nextPage;
    }
    bool operator!=(const VectorStoreFileContentPage &other) const { return !(*this == other); }
};

} // namespace Core
} // namespace QtOpenAi

Q_DECLARE_SHARED(QtOpenAi::Core::VectorStoreSearchRequest)
Q_DECLARE_SHARED(QtOpenAi::Core::VectorStoreSearchResult)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreSearchResult)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreSearchPage)
Q_DECLARE_METATYPE(QtOpenAi::Core::VectorStoreFileContentPage)
