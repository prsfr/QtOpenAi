# ---------------------------------------------------------------------------
# QtOpenAi::Core — value types and (de)serialisation for the OpenAI API.
# ---------------------------------------------------------------------------

set(QTOPENAI_CORE_PUBLIC_HEADERS
    include/QtOpenAi/Core/GlobalCore.h
    include/QtOpenAi/Core/Enums.h
    include/QtOpenAi/Core/FunctionCall.h
    include/QtOpenAi/Core/ToolCall.h
    include/QtOpenAi/Core/ContentPart.h
    include/QtOpenAi/Core/Message.h
    include/QtOpenAi/Core/Tool.h
    include/QtOpenAi/Core/Usage.h
    include/QtOpenAi/Core/Choice.h
    include/QtOpenAi/Core/ResponseFormat.h
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
    include/QtOpenAi/Core/EmbeddingRequest.h
    include/QtOpenAi/Core/EmbeddingResponse.h
    include/QtOpenAi/Core/FileObject.h
    include/QtOpenAi/Core/FileUploadRequest.h
    include/QtOpenAi/Core/Model.h
    include/QtOpenAi/Core/TranscriptionRequest.h
    include/QtOpenAi/Core/TranscriptionResponse.h
    include/QtOpenAi/Core/TranslationRequest.h
    include/QtOpenAi/Core/ImageGenerationRequest.h
    include/QtOpenAi/Core/ImageEditRequest.h
    include/QtOpenAi/Core/ImageVariationRequest.h
    include/QtOpenAi/Core/ImageResponse.h
    include/QtOpenAi/Core/SpeechRequest.h
    include/QtOpenAi/Core/VideoJob.h
    include/QtOpenAi/Core/Upload.h
    include/QtOpenAi/Core/CreateUploadRequest.h
    include/QtOpenAi/Core/VectorStore.h
    include/QtOpenAi/Core/VectorStoreFile.h
    include/QtOpenAi/Core/VectorStoreSearch.h
    include/QtOpenAi/Core/CreateVectorStoreRequest.h
    include/QtOpenAi/Core/Container.h
    include/QtOpenAi/Core/CreateContainerRequest.h
    include/QtOpenAi/Core/CreateVideoRequest.h
    include/QtOpenAi/Core/Voice.h
    include/QtOpenAi/Core/CreateVoiceRequest.h
    include/QtOpenAi/Core/Batch.h
    include/QtOpenAi/Core/CreateBatchRequest.h
    include/QtOpenAi/Core/FineTuningJob.h
    include/QtOpenAi/Core/FineTuningCheckpoint.h
    include/QtOpenAi/Core/CreateFineTuningJobRequest.h
    include/QtOpenAi/Core/Eval.h
    include/QtOpenAi/Core/EvalRun.h
    include/QtOpenAi/Core/CreateEvalRequest.h
    include/QtOpenAi/Core/Assistant.h
    include/QtOpenAi/Core/CreateAssistantRequest.h
    include/QtOpenAi/Core/Thread.h
    include/QtOpenAi/Core/ThreadMessage.h
    include/QtOpenAi/Core/CreateThreadRequest.h
    include/QtOpenAi/Core/Run.h
    include/QtOpenAi/Core/RunStep.h
    include/QtOpenAi/Core/CreateRunRequest.h
    include/QtOpenAi/Core/Project.h
    include/QtOpenAi/Core/BucketPage.h
    include/QtOpenAi/Core/OrganizationUsage.h
    include/QtOpenAi/Core/OrganizationCosts.h
    include/QtOpenAi/Core/OrganizationUser.h
    include/QtOpenAi/Core/Invite.h
    include/QtOpenAi/Core/CreateInviteRequest.h
    include/QtOpenAi/Core/ProjectServiceAccount.h
    include/QtOpenAi/Core/AdminApiKey.h
    include/QtOpenAi/Core/ProjectApiKey.h
    include/QtOpenAi/Core/ProjectRateLimit.h
    include/QtOpenAi/Core/CursorPage.h
    include/QtOpenAi/Core/OrganizationRole.h
    include/QtOpenAi/Core/RoleRequest.h
    include/QtOpenAi/Core/RoleAssignment.h
    include/QtOpenAi/Core/Group.h
    include/QtOpenAi/Core/Certificate.h
    include/QtOpenAi/Core/ProjectPermissions.h
    include/QtOpenAi/Core/Skill.h
    include/QtOpenAi/Core/SkillVersion.h
    include/QtOpenAi/Core/CreateSkillRequest.h
    include/QtOpenAi/Core/ChatKitSession.h
    include/QtOpenAi/Core/ChatKitThread.h
    include/QtOpenAi/Core/ChatKitThreadItem.h
    include/QtOpenAi/Core/CreateChatKitSessionRequest.h
    include/QtOpenAi/Core/RealtimeSessionConfig.h
    include/QtOpenAi/Core/RealtimeClientSecret.h
    include/QtOpenAi/Core/RealtimeEvent.h
    include/QtOpenAi/Core/MetaSchema.h
    include/QtOpenAi/Core/MetaJson.h
    include/QtOpenAi/Core/ModelInfo.h
    include/QtOpenAi/Core/ModelCatalog.h
    include/QtOpenAi/Core/TokenCounter.h
    include/QtOpenAi/Core/VectorIndex.h
    include/QtOpenAi/Core/SchemaValidator.h
)

add_library(QtOpenAiCore
    ${QTOPENAI_CORE_PUBLIC_HEADERS}
    src/JsonHelpers_p.h
    src/Enums.cpp
    src/FunctionCall.cpp
    src/ToolCall.cpp
    src/ContentPart.cpp
    src/Message.cpp
    src/Tool.cpp
    src/Usage.cpp
    src/Choice.cpp
    src/ResponseFormat.cpp
    src/ChatCompletionRequest.cpp
    src/ChatCompletionResponse.cpp
    src/ChatCompletionChunk.cpp
    src/ResponseUsage.cpp
    src/ResponseOutputItem.cpp
    src/Response.cpp
    src/ResponseRequest.cpp
    src/Conversation.cpp
    src/ModerationRequest.cpp
    src/ModerationResponse.cpp
    src/CompletionChoice.cpp
    src/CompletionResponse.cpp
    src/CompletionRequest.cpp
    src/EmbeddingRequest.cpp
    src/EmbeddingResponse.cpp
    src/FileObject.cpp
    src/FileUploadRequest.cpp
    src/Model.cpp
    src/TranscriptionRequest.cpp
    src/TranscriptionResponse.cpp
    src/TranslationRequest.cpp
    src/ImageGenerationRequest.cpp
    src/ImageEditRequest.cpp
    src/ImageVariationRequest.cpp
    src/ImageResponse.cpp
    src/SpeechRequest.cpp
    src/VideoJob.cpp
    src/Upload.cpp
    src/CreateUploadRequest.cpp
    src/VectorStore.cpp
    src/VectorStoreFile.cpp
    src/VectorStoreSearch.cpp
    src/CreateVectorStoreRequest.cpp
    src/Container.cpp
    src/CreateContainerRequest.cpp
    src/CreateVideoRequest.cpp
    src/Voice.cpp
    src/CreateVoiceRequest.cpp
    src/Batch.cpp
    src/CreateBatchRequest.cpp
    src/FineTuningJob.cpp
    src/FineTuningCheckpoint.cpp
    src/CreateFineTuningJobRequest.cpp
    src/Eval.cpp
    src/EvalRun.cpp
    src/CreateEvalRequest.cpp
    src/Assistant.cpp
    src/CreateAssistantRequest.cpp
    src/Thread.cpp
    src/ThreadMessage.cpp
    src/CreateThreadRequest.cpp
    src/Run.cpp
    src/RunStep.cpp
    src/CreateRunRequest.cpp
    src/Project.cpp
    src/OrganizationUsage.cpp
    src/OrganizationCosts.cpp
    src/OrganizationUser.cpp
    src/Invite.cpp
    src/CreateInviteRequest.cpp
    src/ProjectServiceAccount.cpp
    src/AdminApiKey.cpp
    src/ProjectApiKey.cpp
    src/ProjectRateLimit.cpp
    src/OrganizationRole.cpp
    src/RoleRequest.cpp
    src/RoleAssignment.cpp
    src/Group.cpp
    src/Certificate.cpp
    src/ProjectPermissions.cpp
    src/Skill.cpp
    src/SkillVersion.cpp
    src/CreateSkillRequest.cpp
    src/ChatKitSession.cpp
    src/ChatKitThread.cpp
    src/ChatKitThreadItem.cpp
    src/CreateChatKitSessionRequest.cpp
    src/RealtimeSessionConfig.cpp
    src/RealtimeClientSecret.cpp
    src/RealtimeEvent.cpp
    src/MetaSchema.cpp
    src/MetaJson.cpp
    src/ModelInfo.cpp
    src/ModelCatalog.cpp
    src/TokenCounter.cpp
    src/VectorIndex.cpp
    src/SchemaValidator.cpp
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
