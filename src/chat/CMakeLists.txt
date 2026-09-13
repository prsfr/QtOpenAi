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
    include/QtOpenAi/Chat/Agent.h
)

qtopenai_add_module(Chat
    HEADERS
        ${QTOPENAI_CHAT_PUBLIC_HEADERS}
    SOURCES
        src/TrimPolicy.cpp
        src/Transcript.cpp
        src/Agent.cpp
    LIBS
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
)
