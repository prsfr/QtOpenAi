// SPDX-License-Identifier: MIT
#include "QtOpenAi/Client/PageWalker.h"

namespace QtOpenAi {
namespace Client {

class PageWalkerBasePrivate
{
public:
    ListParams params;
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

ListParams PageWalkerBase::params() const
{
    Q_D(const PageWalkerBase);
    return d->params;
}

void PageWalkerBase::setParams(const ListParams &params)
{
    Q_D(PageWalkerBase);
    d->params = params;
}

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
    requestPage();
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
    d->params.after = nextCursor;
    requestPage();
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
