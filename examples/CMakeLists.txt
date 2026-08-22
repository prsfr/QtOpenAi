# Example programs demonstrating the QtOpenAi client library.

# Helper: one QtOpenAi::Client-linked example executable per source file.
function(qtopenai_add_example name source)
    qt_add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE QtOpenAi::Client)
endfunction()

# --- Infrastructure ---
qtopenai_add_example(pagination pagination.cpp)
qtopenai_add_example(token_budget token_budget.cpp)
qtopenai_add_example(metrics metrics.cpp)
qtopenai_add_example(interceptors interceptors.cpp)
qtopenai_add_example(rate_limiting rate_limiting.cpp)
qtopenai_add_example(parallel_map parallel_map.cpp)

# --- Chat & core inference ---
qtopenai_add_example(conversation conversation.cpp)
target_link_libraries(conversation PRIVATE QtOpenAi::Chat)
qtopenai_add_example(agent agent.cpp)
target_link_libraries(agent PRIVATE QtOpenAi::Chat)
qtopenai_add_example(chat_tool_loop tool_loop.cpp)
qtopenai_add_example(meta_tools meta_tools.cpp)
qtopenai_add_example(sandboxed_tools sandboxed_tools.cpp)
target_link_libraries(sandboxed_tools PRIVATE QtOpenAi::Chat QtOpenAi::Tools)
qtopenai_add_example(streaming streaming.cpp)
qtopenai_add_example(responses responses.cpp)
qtopenai_add_example(structured_output structured_output.cpp)
qtopenai_add_example(typed_output typed_output.cpp)
qtopenai_add_example(vision vision.cpp)
qtopenai_add_example(embeddings embeddings.cpp)
qtopenai_add_example(models models.cpp)
qtopenai_add_example(stored_completions stored_completions.cpp)
qtopenai_add_example(semantic_search semantic_search.cpp)
qtopenai_add_example(moderations moderations.cpp)
qtopenai_add_example(guardrails guardrails.cpp)

# --- Multimodal generation (v0.5) ---
qtopenai_add_example(tts text_to_speech.cpp)
qtopenai_add_example(voice_cloning voice_cloning.cpp)
qtopenai_add_example(transcribe transcription.cpp)
qtopenai_add_example(image image_generation.cpp)
qtopenai_add_example(video video_generation.cpp)
qtopenai_add_example(video_derivatives video_derivatives.cpp)

# --- Data & storage (v0.6) ---
qtopenai_add_example(files files.cpp)
qtopenai_add_example(chunked_upload chunked_upload.cpp)
qtopenai_add_example(vector_search vector_search.cpp)
qtopenai_add_example(containers containers.cpp)

# --- Batch, tuning & evals (v0.7) ---
qtopenai_add_example(batch batch.cpp)
qtopenai_add_example(fine_tuning fine_tuning.cpp)
qtopenai_add_example(evals evals.cpp)

# --- Assistants beta (v0.8) ---
qtopenai_add_example(assistant assistant.cpp)

# --- Realtime & newer surfaces (v0.9) ---
qtopenai_add_example(skills skills.cpp)
qtopenai_add_example(chatkit chatkit.cpp)
# The SIP call-control endpoints are the REST half of Realtime, so this one
# needs only QtOpenAi::Client -- unlike the WebSocket example below.
qtopenai_add_example(realtime_calls realtime_calls.cpp)

# --- Administration (v1.0) ---
qtopenai_add_example(organization organization.cpp)
target_link_libraries(organization PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_usage organization_usage.cpp)
target_link_libraries(organization_usage PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_members organization_members.cpp)
target_link_libraries(organization_members PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_projects organization_projects.cpp)
target_link_libraries(organization_projects PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_roles organization_roles.cpp)
target_link_libraries(organization_roles PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_certificates organization_certificates.cpp)
target_link_libraries(organization_certificates PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_permissions organization_permissions.cpp)
target_link_libraries(organization_permissions PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_admin_keys organization_admin_keys.cpp)
target_link_libraries(organization_admin_keys PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_audit_logs organization_audit_logs.cpp)
target_link_libraries(organization_audit_logs PRIVATE QtOpenAi::Admin)
qtopenai_add_example(organization_spend_alerts organization_spend_alerts.cpp)
target_link_libraries(organization_spend_alerts PRIVATE QtOpenAi::Admin)

# --- Persistence ---
qtopenai_add_example(persistence persistence.cpp)
target_link_libraries(persistence PRIVATE QtOpenAi::Chat QtOpenAi::Storage)
# The SQLite backend is optional, so the example demonstrates it only when the
# module was built -- and falls back to the JSON-files store when it was not.
if(QTOPENAI_BUILD_SQL)
    target_link_libraries(persistence PRIVATE QtOpenAi::Sql)
    target_compile_definitions(persistence PRIVATE QTOPENAI_EXAMPLES_HAVE_SQL)
endif()

# The Realtime example needs the optional WebSocket module as well as the client.
if(QTOPENAI_BUILD_REALTIME)
    qtopenai_add_example(realtime realtime.cpp)
    target_link_libraries(realtime PRIVATE QtOpenAi::Realtime)
endif()
