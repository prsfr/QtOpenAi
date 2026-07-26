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

    // The query parameters the walk started from. `after` is overwritten per
    // page with the previous page's last id; everything else (limit, order,
    // before) is carried through unchanged.
    ListParams params() const;
    void setParams(const ListParams &params);

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

    // Issue the request for the next page and wire its reply. Implemented by
    // the template.
    virtual void requestPage() = 0;

    // Called by the template once a page has been handled: `nextCursor` is the
    // page's last id when another page follows, empty when the walk is over.
    // An empty cursor always ends the walk — a server that claims has_more but
    // sends no last_id gives nothing to advance on, and looping would spin.
    void pageHandled(const QString &nextCursor);

    // Mark the walk as finished and honour the auto-delete policy.
    void finish();

    // Report a failed page request: ends the walk and emits failed().
    void reportFailure(const ClientError &error);

    QScopedPointer<PageWalkerBasePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(PageWalkerBase)
};

// Iterate a cursor-paginated endpoint page by page.
//
// Every list endpoint answers with a ListPage<T> carrying `has_more` and
// `last_id`, and takes the same ListParams — so one helper can drive them all.
// Construct it with a factory that issues one request for a given ListParams:
//
//     auto *walker = new PageWalker<FileListReply, Core::FileList>(
//             [&client](const ListParams &p) { return client.listFiles(p); });
//     walker->setPageHandler([](const Core::FileList &page) { ... });
//     connect(walker, &PageWalkerBase::finished, ...);
//     walker->start();
//
// The walker feeds the previous page's last id back as the next `after` cursor
// and stops when the server clears `has_more`. It deletes itself once it stops
// unless setAutoDelete(false) is used.
template <typename Reply, typename Page>
class PageWalker : public PageWalkerBase
{
public:
    using Fetch = std::function<Reply *(const ListParams &)>;
    using PageHandler = std::function<void(const Page &)>;

    explicit PageWalker(Fetch fetch, const ListParams &params = {}, QObject *parent = nullptr)
        : PageWalkerBase(parent)
        , m_fetch(std::move(fetch))
    {
        setParams(params);
    }

    // Called once per page, in order, before the next request goes out.
    void setPageHandler(PageHandler handler) { m_pageHandler = std::move(handler); }

protected:
    void requestPage() override
    {
        Reply *reply = m_fetch(params());
        connect(reply, &Reply::finished, this, [this](const Page &page) {
            if (!isWalking())
                return;
            if (m_pageHandler)
                m_pageHandler(page);
            // stop() may have been called from the handler.
            if (!isWalking())
                return;
            pageHandled(page.hasMore ? page.lastId : QString());
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
};

} // namespace Client
} // namespace QtOpenAi
