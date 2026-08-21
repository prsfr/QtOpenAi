// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/PageWalker.h"

namespace QtOpenAi {
namespace Client {

class PageWalkerBasePrivate
{
public:
    int pageCount = 0;
    bool walking = false;
    bool finished = false;
    bool autoDelete = true;
};

PageWalkerBase::PageWalkerBase(QObject *parent)
    : QObject(parent)
    , d_ptr(new PageWalkerBasePrivate)
{ }

PageWalkerBase::~PageWalkerBase() = default;

int PageWalkerBase::pageCount() const
{
    Q_D(const PageWalkerBase);
    return d->pageCount;
}

bool PageWalkerBase::isWalking() const
{
    Q_D(const PageWalkerBase);
    return d->walking;
}

bool PageWalkerBase::isFinished() const
{
    Q_D(const PageWalkerBase);
    return d->finished;
}

void PageWalkerBase::setAutoDelete(bool enabled)
{
    Q_D(PageWalkerBase);
    d->autoDelete = enabled;
}

bool PageWalkerBase::autoDelete() const
{
    Q_D(const PageWalkerBase);
    return d->autoDelete;
}

void PageWalkerBase::start()
{
    Q_D(PageWalkerBase);
    if (d->finished || d->walking)
        return;
    d->walking = true;
    // An empty cursor is the first page: there is nothing to advance past yet.
    requestPage(QString());
}

void PageWalkerBase::stop()
{
    Q_D(PageWalkerBase);
    d->walking = false;
}

void PageWalkerBase::pageHandled(const QString &nextCursor)
{
    Q_D(PageWalkerBase);
    ++d->pageCount;
    if (nextCursor.isEmpty()) {
        finish();
        Q_EMIT finished();
        return;
    }
    // The cursor goes to the template, which knows which field of which query
    // type it belongs in -- see PageWalker's class note.
    requestPage(nextCursor);
}

void PageWalkerBase::finish()
{
    Q_D(PageWalkerBase);
    d->walking = false;
    d->finished = true;
    if (d->autoDelete)
        deleteLater();
}

void PageWalkerBase::reportFailure(const ClientError &error)
{
    finish();
    Q_EMIT failed(error);
}

} // namespace Client
} // namespace QtOpenAi
