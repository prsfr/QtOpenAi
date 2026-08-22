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

add_library(QtOpenAiStorage
    ${QTOPENAI_STORAGE_PUBLIC_HEADERS}
    src/Store.cpp
    src/JsonFileStore.cpp
    src/PersistentResponseCache.cpp
    src/Autosave.cpp
)

add_library(QtOpenAi::Storage ALIAS QtOpenAiStorage)

target_link_libraries(QtOpenAiStorage
    PUBLIC
        QtOpenAi::Chat
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
)

target_include_directories(QtOpenAiStorage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${QTOPENAI_PRIVATE_INCLUDE_DIR}
)

target_compile_definitions(QtOpenAiStorage PRIVATE QTOPENAI_STORAGE_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiStorage PUBLIC QTOPENAI_STORAGE_STATIC)
endif()

set_target_properties(QtOpenAiStorage PROPERTIES
    OUTPUT_NAME QtOpenAiStorage
    EXPORT_NAME Storage
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiStorage
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Storage/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
