# ---------------------------------------------------------------------------
# QtOpenAi::Storage — persistence for conversations, cached responses and
# metrics.
#
# Its own module because it is the one place that touches all three at once:
# Transcript belongs to Chat, MetricsSnapshot and ResponseCache to Client, and
# putting the store in either of those would have made that module depend on
# the other. It also keeps the dependency honest in the other direction --
# Client and Chat stay usable with no store anywhere in sight, which is what
# the whole library is for.
#
# Qt6::Core only. The SQLite backend needs Qt6::Sql and therefore lives in
# QtOpenAi::Sql, next door.
# ---------------------------------------------------------------------------

set(QTOPENAI_STORAGE_PUBLIC_HEADERS
    include/QtOpenAi/Storage/GlobalStorage.h
    include/QtOpenAi/Storage/Store.h
    include/QtOpenAi/Storage/JsonFileStore.h
    include/QtOpenAi/Storage/PersistentResponseCache.h
    include/QtOpenAi/Storage/Autosave.h
)

qtopenai_add_module(Storage
    HEADERS
        ${QTOPENAI_STORAGE_PUBLIC_HEADERS}
    SOURCES
        src/Store.cpp
        src/JsonFileStore.cpp
        src/PersistentResponseCache.cpp
        src/Autosave.cpp
    LIBS
        QtOpenAi::Chat
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
)
