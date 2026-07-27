# ---------------------------------------------------------------------------
# QtOpenAi::Client — networking client + QMeta-based tool dispatch.
# ---------------------------------------------------------------------------

set(QTOPENAI_CLIENT_PUBLIC_HEADERS
    include/QtOpenAi/Client/GlobalClient.h
    include/QtOpenAi/Client/ClientError.h
    include/QtOpenAi/Client/ToolRegistry.h
    include/QtOpenAi/Client/RetryPolicy.h
    include/QtOpenAi/Client/RestReplyBase.h
    include/QtOpenAi/Client/TypedReply.h
    include/QtOpenAi/Client/BinaryReply.h
    include/QtOpenAi/Client/ChatCompletionReply.h
    include/QtOpenAi/Client/ChatCompletionAccumulator.h
    include/QtOpenAi/Client/ChatCompletionStreamReply.h
    include/QtOpenAi/Client/ResponseReply.h
    include/QtOpenAi/Client/ConversationReply.h
    include/QtOpenAi/Client/ConversationItemsReply.h
    include/QtOpenAi/Client/ModerationReply.h
    include/QtOpenAi/Client/CompletionReply.h
    include/QtOpenAi/Client/ListParams.h
    include/QtOpenAi/Client/PageWalker.h
    include/QtOpenAi/Client/ChatCompletionListReply.h
    include/QtOpenAi/Client/ChatCompletionMessageListReply.h
    include/QtOpenAi/Client/EmbeddingReply.h
    include/QtOpenAi/Client/FileReply.h
    include/QtOpenAi/Client/FileListReply.h
    include/QtOpenAi/Client/ModelReply.h
    include/QtOpenAi/Client/ModelListReply.h
    include/QtOpenAi/Client/CompletionStreamReply.h
    include/QtOpenAi/Client/ResponseStreamReply.h
    include/QtOpenAi/Client/TranscriptionReply.h
    include/QtOpenAi/Client/ImageReply.h
    include/QtOpenAi/Client/InputTokensReply.h
    include/QtOpenAi/Client/SpeechReply.h
    include/QtOpenAi/Client/VoiceReply.h
    include/QtOpenAi/Client/VoiceConsentReply.h
    include/QtOpenAi/Client/VoiceConsentListReply.h
    include/QtOpenAi/Client/VideoReply.h
    include/QtOpenAi/Client/VideoListReply.h
    include/QtOpenAi/Client/VideoContentReply.h
    include/QtOpenAi/Client/JobPoller.h
    include/QtOpenAi/Client/VideoPoller.h
    include/QtOpenAi/Client/UploadReply.h
    include/QtOpenAi/Client/UploadPartReply.h
    include/QtOpenAi/Client/ChunkedUploader.h
    include/QtOpenAi/Client/VectorStoreReply.h
    include/QtOpenAi/Client/VectorStoreListReply.h
    include/QtOpenAi/Client/VectorStoreFileReply.h
    include/QtOpenAi/Client/VectorStoreFileListReply.h
    include/QtOpenAi/Client/VectorStoreFileBatchReply.h
    include/QtOpenAi/Client/VectorStoreFileContentReply.h
    include/QtOpenAi/Client/VectorStoreSearchReply.h
    include/QtOpenAi/Client/ContainerReply.h
    include/QtOpenAi/Client/ContainerListReply.h
    include/QtOpenAi/Client/ContainerFileReply.h
    include/QtOpenAi/Client/ContainerFileListReply.h
    include/QtOpenAi/Client/BatchReply.h
    include/QtOpenAi/Client/BatchListReply.h
    include/QtOpenAi/Client/BatchPoller.h
    include/QtOpenAi/Client/FineTuningJobReply.h
    include/QtOpenAi/Client/FineTuningJobListReply.h
    include/QtOpenAi/Client/FineTuningEventListReply.h
    include/QtOpenAi/Client/FineTuningCheckpointListReply.h
    include/QtOpenAi/Client/FineTuningPermissionListReply.h
    include/QtOpenAi/Client/FineTuningPermissionReply.h
    include/QtOpenAi/Client/FineTuningJobPoller.h
    include/QtOpenAi/Client/EvalReply.h
    include/QtOpenAi/Client/EvalListReply.h
    include/QtOpenAi/Client/EvalRunReply.h
    include/QtOpenAi/Client/EvalRunListReply.h
    include/QtOpenAi/Client/EvalRunOutputItemReply.h
    include/QtOpenAi/Client/EvalRunOutputItemListReply.h
    include/QtOpenAi/Client/EvalRunPoller.h
    include/QtOpenAi/Client/AssistantReply.h
    include/QtOpenAi/Client/AssistantListReply.h
    include/QtOpenAi/Client/ThreadReply.h
    include/QtOpenAi/Client/ThreadMessageReply.h
    include/QtOpenAi/Client/ThreadMessageListReply.h
    include/QtOpenAi/Client/RunReply.h
    include/QtOpenAi/Client/RunListReply.h
    include/QtOpenAi/Client/RunStepReply.h
    include/QtOpenAi/Client/RunStepListReply.h
    include/QtOpenAi/Client/RunStreamReply.h
    include/QtOpenAi/Client/RunPoller.h
    include/QtOpenAi/Client/Client.h
)

add_library(QtOpenAiClient
    ${QTOPENAI_CLIENT_PUBLIC_HEADERS}
    src/HttpSupport_p.h
    src/RestReply_p.h
    src/RestReplyBase_p.h
    src/SseParser_p.h
    src/Multipart_p.h
    src/JobPoller_p.h
    src/ToolRegistry.cpp
    src/ClientError.cpp
    src/RetryPolicy.cpp
    src/RestReply.cpp
    src/RestReplyBase.cpp
    src/PageWalker.cpp
    src/BinaryReply.cpp
    src/ChatCompletionAccumulator.cpp
    src/ChatCompletionStreamReply.cpp
    src/ConversationItemsReply.cpp
    src/CompletionStreamReply.cpp
    src/ResponseStreamReply.cpp
    src/InputTokensReply.cpp
    src/SpeechReply.cpp
    src/VideoContentReply.cpp
    src/JobPoller.cpp
    src/VideoPoller.cpp
    src/ChunkedUploader.cpp
    src/BatchPoller.cpp
    src/FineTuningJobPoller.cpp
    src/EvalRunPoller.cpp
    src/RunStreamReply.cpp
    src/RunPoller.cpp
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
