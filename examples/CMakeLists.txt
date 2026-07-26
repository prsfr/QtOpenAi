# Example programs demonstrating the QtOpenAi client library.

# Helper: one QtOpenAi::Client-linked example executable per source file.
function(qtopenai_add_example name source)
    qt_add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE QtOpenAi::Client)
endfunction()

# --- Infrastructure ---
qtopenai_add_example(pagination pagination.cpp)

# --- Chat & core inference ---
qtopenai_add_example(chat_tool_loop tool_loop.cpp)
qtopenai_add_example(streaming streaming.cpp)
qtopenai_add_example(responses responses.cpp)
qtopenai_add_example(structured_output structured_output.cpp)
qtopenai_add_example(vision vision.cpp)
qtopenai_add_example(embeddings embeddings.cpp)
qtopenai_add_example(moderations moderations.cpp)

# --- Multimodal generation (v0.5) ---
qtopenai_add_example(tts text_to_speech.cpp)
qtopenai_add_example(transcribe transcription.cpp)
qtopenai_add_example(image image_generation.cpp)
qtopenai_add_example(video video_generation.cpp)

# --- Data & storage (v0.6) ---
qtopenai_add_example(files files.cpp)
qtopenai_add_example(chunked_upload chunked_upload.cpp)
qtopenai_add_example(vector_search vector_search.cpp)
qtopenai_add_example(containers containers.cpp)

# --- Batch, tuning & evals (v0.7) ---
qtopenai_add_example(batch batch.cpp)
qtopenai_add_example(fine_tuning fine_tuning.cpp)
