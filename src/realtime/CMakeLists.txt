# ---------------------------------------------------------------------------
# QtOpenAi::Realtime — the Realtime WebSocket channel.
#
# Kept in its own module because it is the only part of the library that needs
# Qt6::WebSockets: every other endpoint is a request/reply over QNetworkAccess-
# Manager. Depends on Core for the event and session value types, and not on
# Client — the REST half of /realtime (minting a client secret, SIP call
# control) lives there and is usable on its own.
# ---------------------------------------------------------------------------

set(QTOPENAI_REALTIME_PUBLIC_HEADERS
    include/QtOpenAi/Realtime/GlobalRealtime.h
    include/QtOpenAi/Realtime/RealtimeConnection.h
)

qtopenai_add_module(Realtime
    HEADERS
        ${QTOPENAI_REALTIME_PUBLIC_HEADERS}
    SOURCES
        src/RealtimeConnection.cpp
    LIBS
        QtOpenAi::Core
        Qt6::Core
        Qt6::WebSockets
)
