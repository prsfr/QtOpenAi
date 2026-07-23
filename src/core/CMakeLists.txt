# ---------------------------------------------------------------------------
# QtOpenAi::Core — value types and (de)serialisation for the chat API.
# ---------------------------------------------------------------------------

set(QTOPENAI_CORE_PUBLIC_HEADERS
    include/QtOpenAi/Core/GlobalCore.h
    include/QtOpenAi/Core/Enums.h
    include/QtOpenAi/Core/FunctionCall.h
    include/QtOpenAi/Core/ToolCall.h
    include/QtOpenAi/Core/Message.h
    include/QtOpenAi/Core/Tool.h
    include/QtOpenAi/Core/Usage.h
    include/QtOpenAi/Core/Choice.h
    include/QtOpenAi/Core/ChatCompletionRequest.h
    include/QtOpenAi/Core/ChatCompletionResponse.h
    include/QtOpenAi/Core/ChatCompletionChunk.h
    include/QtOpenAi/Core/ResponseUsage.h
    include/QtOpenAi/Core/ResponseOutputItem.h
    include/QtOpenAi/Core/Response.h
    include/QtOpenAi/Core/ResponseRequest.h
    include/QtOpenAi/Core/Conversation.h
    include/QtOpenAi/Core/ConversationItemList.h
    include/QtOpenAi/Core/ModerationRequest.h
    include/QtOpenAi/Core/ModerationResponse.h
    include/QtOpenAi/Core/CompletionChoice.h
    include/QtOpenAi/Core/CompletionResponse.h
    include/QtOpenAi/Core/CompletionRequest.h
    include/QtOpenAi/Core/ListPage.h
    include/QtOpenAi/Core/ChatCompletionList.h
)

add_library(QtOpenAiCore
    ${QTOPENAI_CORE_PUBLIC_HEADERS}
    src/JsonHelpers_p.h
    src/Enums.cpp
    src/FunctionCall.cpp
    src/ToolCall.cpp
    src/Message.cpp
    src/Tool.cpp
    src/Usage.cpp
    src/Choice.cpp
    src/ChatCompletionRequest.cpp
    src/ChatCompletionResponse.cpp
    src/ChatCompletionChunk.cpp
    src/ResponseUsage.cpp
    src/ResponseOutputItem.cpp
    src/Response.cpp
    src/ResponseRequest.cpp
    src/Conversation.cpp
    src/ConversationItemList.cpp
    src/ModerationRequest.cpp
    src/ModerationResponse.cpp
    src/CompletionChoice.cpp
    src/CompletionResponse.cpp
    src/CompletionRequest.cpp
)

# Namespaced alias so consumers can link QtOpenAi::Core.
add_library(QtOpenAi::Core ALIAS QtOpenAiCore)

target_link_libraries(QtOpenAiCore PUBLIC Qt6::Core)

target_include_directories(QtOpenAiCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(QtOpenAiCore PRIVATE QTOPENAI_CORE_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiCore PUBLIC QTOPENAI_CORE_STATIC)
endif()

set_target_properties(QtOpenAiCore PROPERTIES
    OUTPUT_NAME QtOpenAiCore
    EXPORT_NAME Core
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiCore
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Core/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
