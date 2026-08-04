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

bool Store::fail(const QString &message)
{
    d->lastError = message;
    return false;
}

void Store::clearError() { d->lastError.clear(); }

} // namespace Storage
} // namespace QtOpenAi
