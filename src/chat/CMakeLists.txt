# ---------------------------------------------------------------------------
# QtOpenAi::Chat — client-side conversation management.
#
# Its own module because it is orchestration, not data: Core stays a pure
# value-type and serialisation layer, and this is the part that decides what
# goes into the next request — and, in Agent's case, drives the whole
# chat/tool/chat loop, which is why the module links Client. Transcript and
# TrimPolicy themselves touch no networking: history can be built, trimmed and
# persisted without a Client ever existing.
# ---------------------------------------------------------------------------

set(QTOPENAI_CHAT_PUBLIC_HEADERS
    include/QtOpenAi/Chat/GlobalChat.h
    include/QtOpenAi/Chat/TrimPolicy.h
    include/QtOpenAi/Chat/Transcript.h
    include/QtOpenAi/Chat/ConversationModel.h
    include/QtOpenAi/Chat/Agent.h
)

add_library(QtOpenAiChat
    ${QTOPENAI_CHAT_PUBLIC_HEADERS}
    src/TrimPolicy.cpp
    src/Transcript.cpp
    src/ConversationModel.cpp
    src/Agent.cpp
)

add_library(QtOpenAi::Chat ALIAS QtOpenAiChat)

target_link_libraries(QtOpenAiChat
    PUBLIC
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
)

target_include_directories(QtOpenAiChat
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(QtOpenAiChat PRIVATE QTOPENAI_CHAT_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiChat PUBLIC QTOPENAI_CHAT_STATIC)
endif()

set_target_properties(QtOpenAiChat PROPERTIES
    OUTPUT_NAME QtOpenAiChat
    EXPORT_NAME Chat
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiChat
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Chat/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
