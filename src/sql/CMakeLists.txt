# ---------------------------------------------------------------------------
# QtOpenAi::Sql — the SQLite backend for QtOpenAi::Storage.
#
# Kept out of QtOpenAi::Storage for the same reason QtOpenAi::Realtime is kept
# out of QtOpenAi::Client: it is the only part of the library that needs
# Qt6::Sql, and a module boundary is how that dependency stays optional. An
# application that persists to JSON files -- or does not persist at all -- does
# not link a database driver to get there.
#
# Depends on Storage for the Store interface it implements, on Chat and Client
# for the Transcript and MetricsSnapshot it stores (SqliteStore.cpp uses both
# directly, so it declares them rather than receiving them through Storage),
# and on Qt6::Sql.
# ---------------------------------------------------------------------------

set(QTOPENAI_SQL_PUBLIC_HEADERS
    include/QtOpenAi/Sql/GlobalSql.h
    include/QtOpenAi/Sql/SqliteStore.h
)

qtopenai_add_module(Sql
    HEADERS
        ${QTOPENAI_SQL_PUBLIC_HEADERS}
    SOURCES
        src/SqliteStore.cpp
    LIBS
        QtOpenAi::Storage
        QtOpenAi::Chat
        QtOpenAi::Client
        Qt6::Core
        Qt6::Sql
)
