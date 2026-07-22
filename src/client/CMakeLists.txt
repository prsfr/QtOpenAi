# ---------------------------------------------------------------------------
# QtOpenAi::Client — networking client + QMeta-based tool dispatch.
# ---------------------------------------------------------------------------

set(QTOPENAI_CLIENT_PUBLIC_HEADERS
    include/QtOpenAi/Client/GlobalClient.h
    include/QtOpenAi/Client/ClientError.h
    include/QtOpenAi/Client/ToolRegistry.h
    include/QtOpenAi/Client/ChatCompletionReply.h
    include/QtOpenAi/Client/Client.h
)

add_library(QtOpenAiClient
    ${QTOPENAI_CLIENT_PUBLIC_HEADERS}
    src/ToolRegistry.cpp
    src/ChatCompletionReply.cpp
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
