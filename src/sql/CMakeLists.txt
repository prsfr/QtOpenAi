# ---------------------------------------------------------------------------
# QtOpenAi::Sql — the SQLite backend for QtOpenAi::Storage.
#
# Kept out of QtOpenAi::Storage for the same reason QtOpenAi::Realtime is kept
# out of QtOpenAi::Client: it is the only part of the library that needs
# Qt6::Sql, and a module boundary is how that dependency stays optional. An
# application that persists to JSON files -- or does not persist at all -- does
# not link a database driver to get there.
#
# Depends on Storage for the Store interface it implements, and on nothing of
# its own beyond Qt6::Sql.
# ---------------------------------------------------------------------------

set(QTOPENAI_SQL_PUBLIC_HEADERS
    include/QtOpenAi/Sql/GlobalSql.h
    include/QtOpenAi/Sql/SqliteStore.h
)

add_library(QtOpenAiSql
    ${QTOPENAI_SQL_PUBLIC_HEADERS}
    src/SqliteStore.cpp
)

add_library(QtOpenAi::Sql ALIAS QtOpenAiSql)

target_link_libraries(QtOpenAiSql
    PUBLIC
        QtOpenAi::Storage
        Qt6::Core
        Qt6::Sql
)

target_include_directories(QtOpenAiSql
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${QTOPENAI_PRIVATE_INCLUDE_DIR}
)

target_compile_definitions(QtOpenAiSql PRIVATE QTOPENAI_SQL_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiSql PUBLIC QTOPENAI_SQL_STATIC)
endif()

set_target_properties(QtOpenAiSql PROPERTIES
    OUTPUT_NAME QtOpenAiSql
    EXPORT_NAME Sql
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiSql
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Sql/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
