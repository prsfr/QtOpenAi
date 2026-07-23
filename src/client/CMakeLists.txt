# ---------------------------------------------------------------------------
# QtOpenAi::Client — networking client + QMeta-based tool dispatch.
# ---------------------------------------------------------------------------

set(QTOPENAI_CLIENT_PUBLIC_HEADERS
    include/QtOpenAi/Client/GlobalClient.h
    include/QtOpenAi/Client/ClientError.h
    include/QtOpenAi/Client/ToolRegistry.h
    include/QtOpenAi/Client/RetryPolicy.h
    include/QtOpenAi/Client/ChatCompletionReply.h
    include/QtOpenAi/Client/ChatCompletionAccumulator.h
    include/QtOpenAi/Client/ChatCompletionStreamReply.h
    include/QtOpenAi/Client/ResponseReply.h
    include/QtOpenAi/Client/ConversationReply.h
    include/QtOpenAi/Client/ConversationItemsReply.h
    include/QtOpenAi/Client/ModerationReply.h
    include/QtOpenAi/Client/CompletionReply.h
    include/QtOpenAi/Client/ListParams.h
    include/QtOpenAi/Client/ChatCompletionListReply.h
    include/QtOpenAi/Client/ChatCompletionMessageListReply.h
    include/QtOpenAi/Client/Client.h
)

add_library(QtOpenAiClient
    ${QTOPENAI_CLIENT_PUBLIC_HEADERS}
    src/HttpSupport_p.h
    src/RestReply_p.h
    src/ToolRegistry.cpp
    src/RetryPolicy.cpp
    src/RestReply.cpp
    src/ChatCompletionReply.cpp
    src/ChatCompletionAccumulator.cpp
    src/ChatCompletionStreamReply.cpp
    src/ResponseReply.cpp
    src/ConversationReply.cpp
    src/ConversationItemsReply.cpp
    src/ModerationReply.cpp
    src/CompletionReply.cpp
    src/ChatCompletionListReply.cpp
    src/ChatCompletionMessageListReply.cpp
    src/Client.cpp
)

add_library(QtOpenAi::Client ALIAS QtOpenAiClient)

target_link_libraries(QtOpenAiClient
    PUBLIC
        QtOpenAi::Core
        Qt6::Core
        Qt6::Network
)

target_include_directories(QtOpenAiClient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(QtOpenAiClient PRIVATE QTOPENAI_CLIENT_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiClient PUBLIC QTOPENAI_CLIENT_STATIC)
endif()

set_target_properties(QtOpenAiClient PROPERTIES
    OUTPUT_NAME QtOpenAiClient
    EXPORT_NAME Client
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiClient
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Client/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
