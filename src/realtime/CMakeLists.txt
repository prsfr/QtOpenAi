# ---------------------------------------------------------------------------
# QtOpenAi::Realtime — the Realtime WebSocket channel.
#
# Kept in its own module because it is the only part of the library that needs
# Qt6::WebSockets: every other endpoint is a request/reply over QNetworkAccess-
# Manager. Depends on Core for the event and session value types, and not on
# Client — the REST half of /realtime (minting a client secret, SIP call
# control) lives there and is usable on its own.
# ---------------------------------------------------------------------------

set(QTOPENAI_REALTIME_PUBLIC_HEADERS
    include/QtOpenAi/Realtime/GlobalRealtime.h
    include/QtOpenAi/Realtime/RealtimeConnection.h
)

add_library(QtOpenAiRealtime
    ${QTOPENAI_REALTIME_PUBLIC_HEADERS}
    src/RealtimeConnection.cpp
)

add_library(QtOpenAi::Realtime ALIAS QtOpenAiRealtime)

target_link_libraries(QtOpenAiRealtime
    PUBLIC
        QtOpenAi::Core
        Qt6::Core
        Qt6::WebSockets
)

target_include_directories(QtOpenAiRealtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${QTOPENAI_PRIVATE_INCLUDE_DIR}
)

target_compile_definitions(QtOpenAiRealtime PRIVATE QTOPENAI_REALTIME_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiRealtime PUBLIC QTOPENAI_REALTIME_STATIC)
endif()

set_target_properties(QtOpenAiRealtime PROPERTIES
    OUTPUT_NAME QtOpenAiRealtime
    EXPORT_NAME Realtime
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiRealtime
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Realtime/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
