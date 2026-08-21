// SPDX-License-Identifier: MIT
#pragma once

#include <QtOpenAi/Client/ClientError.h>
#include <QtOpenAi/Client/GlobalClient.h>
#include <QtOpenAi/Client/ListParams.h>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <functional>

namespace QtOpenAi {
namespace Client {

class PageWalkerBasePrivate;

// The QObject half of PageWalker: the signals and the walk state.
//
// PageWalker itself is a class template, and templates cannot carry Q_OBJECT, so
// everything that needs moc lives here and the template derives from it. Callers
// connect to these signals; the typed pages arrive through the template's
// page handler.
class QTOPENAI_CLIENT_EXPORT PageWalkerBase : public QObject
{
    Q_OBJECT
public:
    ~PageWalkerBase() override;

    // Pages successfully handed to the page handler so far.
    int pageCount() const;

    bool isWalking() const;
    bool isFinished() const;

    // Delete the walker once it stops (default true).
    void setAutoDelete(bool enabled);
    bool autoDelete() const;

    // Fetch the first page and keep going while the server reports has_more.
    // No-op once the walk has finished.
    void start();
    // Stop without emitting finished(). Safe to call from the page handler when
    // the caller has seen enough.
    void stop();

Q_SIGNALS:
    // Emitted once when the last page (has_more == false) has been handled.
    void finished();
    // Emitted once when a page request fails (network/HTTP/parse). The pages
    // handled before the failure stay delivered.
    void failed(const QtOpenAi::Client::ClientError &error);

protected:
    // The base owns its private data outright: PageWalker adds no state of its
    // own beyond the two std::functions it holds directly, so no subclass
    // Private is needed and PageWalkerBasePrivate stays out of this header.
    explicit PageWalkerBase(QObject *parent = nullptr);

    // Issue the request for one page and wire its reply. Implemented by the
    // template. An empty `cursor` is the first page; otherwise the template
    // puts the cursor into its own query type before fetching.
    //
    // The cursor is passed through rather than stored here because the query
    // type varies: ListParams for most endpoints, but the administration
    // reports carry their own, and one of those advances by `page` rather than
    // `after`.
    virtual void requestPage(const QString &cursor) = 0;

    // Called by the template once a page has been handled: `nextCursor` is
    // whatever Core::nextPageCursor() read off the page, empty when the walk is
    // over. An empty cursor always ends the walk — a server that claims has_more
    // but sends no cursor gives nothing to advance on, and looping would spin.
    void pageHandled(const QString &nextCursor);

    // Mark the walk as finished and honour the auto-delete policy.
    void finish();

    // Report a failed page request: ends the walk and emits failed().
    void reportFailure(const ClientError &error);

    QScopedPointer<PageWalkerBasePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(PageWalkerBase)
};

// Iterate a paginated endpoint page by page.
//
// Construct it with a factory that issues one request for a given query:
//
//     auto *walker = new PageWalker<FileListReply, Core::FileList>(
//             [&client](const ListParams &p) { return client.listFiles(p); });
//     walker->setPageHandler([](const Core::FileList &page) { ... });
//     connect(walker, &PageWalkerBase::finished, ...);
//     walker->start();
//
// The walker feeds each page's cursor back into the query and stops when the
// cursor comes back empty. It deletes itself once it stops unless
// setAutoDelete(false) is used.
//
// **Three page shapes, one walker.** The library paginates in three different
// spellings -- Core::ListPage advances by `last_id`, Core::CursorPage by an
// opaque `next`, Core::BucketPage by `next_page` -- and the endpoints carry
// their cursor back in two different query fields. Rather than three walkers,
// the two varying steps are free functions found by argument-dependent lookup:
//
//   * `Core::nextPageCursor(page)` reads the cursor off whichever page shape
//   * `applyPageCursor(query, cursor)` writes it into whichever query type
//
// So walking the administration reports needs nothing but the third template
// argument naming their query:
//
//     new PageWalker<UsageReply, Core::UsagePage, Admin::UsageQuery>(
//             [&](const Admin::UsageQuery &q) { return org.usage(kind, q); }, query);
//
// A query type defined elsewhere joins in by declaring its own
// `applyPageCursor` overload beside itself; nothing here needs to know of it.
template <typename Reply, typename Page, typename Params = ListParams>
class PageWalker : public PageWalkerBase
{
public:
    using Fetch = std::function<Reply *(const Params &)>;
    using PageHandler = std::function<void(const Page &)>;

    explicit PageWalker(Fetch fetch, const Params &params = {}, QObject *parent = nullptr)
        : PageWalkerBase(parent)
        , m_fetch(std::move(fetch))
        , m_params(params)
    { }

    // The query the walk started from. Its cursor field is overwritten per page;
    // every other filter is carried through unchanged, which is what keeps a
    // walk restricted to what the caller asked for.
    Params params() const { return m_params; }
    void setParams(const Params &params) { m_params = params; }

    // Called once per page, in order, before the next request goes out.
    void setPageHandler(PageHandler handler) { m_pageHandler = std::move(handler); }

protected:
    void requestPage(const QString &cursor) override
    {
        if (!cursor.isEmpty())
            applyPageCursor(m_params, cursor); // ADL; see the class note
        Reply *reply = m_fetch(m_params);
        connect(reply, &Reply::finished, this, [this](const Page &page) {
            if (!isWalking())
                return;
            if (m_pageHandler)
                m_pageHandler(page);
            // stop() may have been called from the handler.
            if (!isWalking())
                return;
            pageHandled(nextPageCursor(page)); // ADL; see the class note
        });
        connect(reply, &Reply::failed, this, [this](const ClientError &error) {
            if (!isWalking())
                return;
            reportFailure(error);
        });
    }

private:
    Fetch m_fetch;
    PageHandler m_pageHandler;
    Params m_params;
};

} // namespace Client
} // namespace QtOpenAi
