# Example programs demonstrating the QtOpenAi client library.

qt_add_executable(chat_tool_loop tool_loop.cpp)
target_link_libraries(chat_tool_loop PRIVATE QtOpenAi::Client)

# --- Multimodal generation (v0.5) ---
qt_add_executable(tts text_to_speech.cpp)
target_link_libraries(tts PRIVATE QtOpenAi::Client)

qt_add_executable(transcribe transcription.cpp)
target_link_libraries(transcribe PRIVATE QtOpenAi::Client)

qt_add_executable(image image_generation.cpp)
target_link_libraries(image PRIVATE QtOpenAi::Client)

qt_add_executable(video video_generation.cpp)
target_link_libraries(video PRIVATE QtOpenAi::Client)
