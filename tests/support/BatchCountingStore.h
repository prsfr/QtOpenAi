// SPDX-License-Identifier: MIT
#pragma once

// A JsonFileStore that counts how it was batched.
//
// "These writes are one batch" is not visible in what a store ends up
// holding: the conversation and the metrics snapshot of one autosave interval
// are on disk either way, and so are a cached response and the prune that
// followed it. What the batching is *for* -- one commit instead of two or
// three -- is only observable at the interface, which is why the callers that
// group their writes are tested by counting the calls rather than by timing
// the result.
//
//     BatchCountingStore store(root.path());
//     store.open();
//     ...
//     QCOMPARE(store.batchesBegun, 1);
//
// Over a real JsonFileStore rather than a stub, so the writes inside the batch
// still land and the test can assert on both.
//
// It deliberately has no Q_OBJECT macro: a Store is not a QObject, so this
// needs no moc even though it lives in a header shared by several test
// translation units.

#include <QtOpenAi/Storage/JsonFileStore.h>

#include <QtCore/QString>

class BatchCountingStore : public QtOpenAi::Storage::JsonFileStore
{
public:
    using JsonFileStore::JsonFileStore;

    bool beginBatch() override
    {
        ++batchesBegun;
        return JsonFileStore::beginBatch();
    }

    bool endBatch(bool commit = true) override
    {
        ++batchesEnded;
        if (!commit)
            ++batchesDropped;
        return JsonFileStore::endBatch(commit);
    }

    int batchesBegun = 0;
    int batchesEnded = 0;
    int batchesDropped = 0;
};
