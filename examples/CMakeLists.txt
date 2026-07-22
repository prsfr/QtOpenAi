# Example programs demonstrating the QtOpenAi client library.

qt_add_executable(chat_tool_loop tool_loop.cpp)
target_link_libraries(chat_tool_loop PRIVATE QtOpenAi::Client)
