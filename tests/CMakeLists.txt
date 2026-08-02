find_package(Qt6 6.4 REQUIRED COMPONENTS Test)

# Helper to declare a QtTest-based unit test with a consistent naming scheme.
function(qtopenai_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    # Lets tests share helpers via #include "support/....h".
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

# --- Core module tests ---
qtopenai_add_test(tst_core_enums
    SOURCES core/tst_enums.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_roundtrip
    SOURCES core/tst_roundtrip.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_serialization
    SOURCES core/tst_serialization.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_multimodal
    SOURCES core/tst_multimodal.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_valuesemantics
    SOURCES core/tst_valuesemantics.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_chunk
    SOURCES core/tst_chunk.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_responses
    SOURCES core/tst_responses.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_conversations
    SOURCES core/tst_conversations.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_moderations
    SOURCES core/tst_moderations.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_completions
    SOURCES core/tst_completions.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_liststore
    SOURCES core/tst_liststore.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_embeddings
    SOURCES core/tst_embeddings.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_models
    SOURCES core/tst_models.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_chatrequestparams
    SOURCES core/tst_chatrequestparams.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_responseformat
    SOURCES core/tst_responseformat.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_transcriptions
    SOURCES core/tst_transcriptions.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_images
    SOURCES core/tst_images.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_speech
    SOURCES core/tst_speech.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_voices
    SOURCES core/tst_voices.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_video
    SOURCES core/tst_video.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_files
    SOURCES core/tst_files.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_uploads
    SOURCES core/tst_uploads.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_vectorstores
    SOURCES core/tst_vectorstores.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_containers
    SOURCES core/tst_containers.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_batches
    SOURCES core/tst_batches.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_finetuning
    SOURCES core/tst_finetuning.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_evals
    SOURCES core/tst_evals.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_assistants
    SOURCES core/tst_assistants.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_threads
    SOURCES core/tst_threads.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_skills
    SOURCES core/tst_skills.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_chatkit
    SOURCES core/tst_chatkit.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_realtime
    SOURCES core/tst_realtime.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_metaschema
    SOURCES core/tst_metaschema.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_schemavalidator
    SOURCES core/tst_schemavalidator.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_metajson
    SOURCES core/tst_metajson.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_modelcatalog
    SOURCES core/tst_modelcatalog.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_tokencounter
    SOURCES core/tst_tokencounter.cpp
    LIBS QtOpenAi::Core)

# --- Client module tests ---
qtopenai_add_test(tst_client_toolregistry
    SOURCES client/tst_toolregistry.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_metrics
    SOURCES client/tst_metrics.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_interceptor
    SOURCES client/tst_interceptor.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_responsecache
    SOURCES client/tst_responsecache.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_ratelimiter
    SOURCES client/tst_ratelimiter.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_providerprofile
    SOURCES client/tst_providerprofile.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_reply
    SOURCES client/tst_reply.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_requestplumbing
    SOURCES client/tst_requestplumbing.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_streaming
    SOURCES client/tst_streaming.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_resilience
    SOURCES client/tst_resilience.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_responses
    SOURCES client/tst_responses.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_conversations
    SOURCES client/tst_conversations.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_moderations
    SOURCES client/tst_moderations.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_completions
    SOURCES client/tst_completions.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_storedcompletions
    SOURCES client/tst_storedcompletions.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_embeddings
    SOURCES client/tst_embeddings.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_streaming_extra
    SOURCES client/tst_streaming_extra.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_transcriptions
    SOURCES client/tst_transcriptions.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_images
    SOURCES client/tst_images.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_speech
    SOURCES client/tst_speech.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_video
    SOURCES client/tst_video.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_files
    SOURCES client/tst_files.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_uploads
    SOURCES client/tst_uploads.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_vectorstores
    SOURCES client/tst_vectorstores.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_containers
    SOURCES client/tst_containers.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_batches
    SOURCES client/tst_batches.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_finetuning
    SOURCES client/tst_finetuning.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_pagination
    SOURCES client/tst_pagination.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_voices
    SOURCES client/tst_voices.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_evals
    SOURCES client/tst_evals.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_assistants
    SOURCES client/tst_assistants.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_threads
    SOURCES client/tst_threads.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_skills
    SOURCES client/tst_skills.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_chatkit
    SOURCES client/tst_chatkit.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_realtime
    SOURCES client/tst_realtime.cpp
    LIBS QtOpenAi::Client)

# --- Chat module tests ---
qtopenai_add_test(tst_chat_transcript
    SOURCES chat/tst_transcript.cpp
    LIBS QtOpenAi::Chat)

qtopenai_add_test(tst_chat_trimpolicy
    SOURCES chat/tst_trimpolicy.cpp
    LIBS QtOpenAi::Chat)

qtopenai_add_test(tst_chat_conversationmodel
    SOURCES chat/tst_conversationmodel.cpp
    LIBS QtOpenAi::Chat)

qtopenai_add_test(tst_chat_agent
    SOURCES chat/tst_agent.cpp
    LIBS QtOpenAi::Chat)

# --- Realtime module tests (built with the module) ---
if(QTOPENAI_BUILD_REALTIME)
    qtopenai_add_test(tst_realtime_connection
        SOURCES realtime/tst_connection.cpp
        LIBS QtOpenAi::Realtime Qt6::WebSockets)
endif()
