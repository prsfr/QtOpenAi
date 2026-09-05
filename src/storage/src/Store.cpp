// SPDX-License-Identifier: MIT
#include "QtOpenAi/Storage/Store.h"

namespace QtOpenAi {
namespace Storage {

class StorePrivate
{
public:
    QString lastError;
};

Store::Store()
    : d(new StorePrivate)
{ }

Store::~Store() = default;

QString Store::lastError() const { return d->lastError; }

bool Store::beginBatch()
{
    // Nothing to group, but a store that is not open cannot even pretend to:
    // reporting success there is the pretending every other call here avoids.
    // lastError() is deliberately left alone -- see the header.
    return isOpen() || fail(QStringLiteral("Store: not open."));
}

bool Store::endBatch(bool commit)
{
    // Whether the batch was to be kept makes no difference to a backend that
    // never grouped anything: there is nothing held back to drop.
    Q_UNUSED(commit);
    return isOpen() || fail(QStringLiteral("Store: not open."));
}

QList<ConversationRecord> Store::conversations(int limit, int offset)
{
    // The bounded listing a backend that cannot bound its own query still
    // owes: ask for all of them and slice. A ceiling of zero reads nothing,
    // which is the one case this can answer as cheaply as any backend.
    if (limit == 0)
        return {};
    return conversations().mid(qMax(0, offset), limit < 0 ? -1 : limit);
}

bool Store::fail(const QString &message)
{
    d->lastError = message;
    return false;
}

void Store::clearError() { d->lastError.clear(); }

} // namespace Storage
} // namespace QtOpenAi
