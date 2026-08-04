# ---------------------------------------------------------------------------
# QtOpenAi::Tools — ready-made tools for the ToolRegistry, sandboxed.
#
# Its own module, and not part of Client, for a reason that is not tidiness:
# linking Client must never be what gives a model access to the filesystem or
# the network. Reaching these tools takes a deliberate line in a CMakeLists,
# then a deliberate policy object, then an explicit install() call -- and a
# reviewer can find every application that took those steps by grepping for
# the module name.
# ---------------------------------------------------------------------------

set(QTOPENAI_TOOLS_PUBLIC_HEADERS
    include/QtOpenAi/Tools/GlobalTools.h
    include/QtOpenAi/Tools/FileSandbox.h
    include/QtOpenAi/Tools/FileTools.h
    include/QtOpenAi/Tools/HttpTools.h
    include/QtOpenAi/Tools/UtilityTools.h
    include/QtOpenAi/Tools/DefaultTools.h
)

add_library(QtOpenAiTools
    ${QTOPENAI_TOOLS_PUBLIC_HEADERS}
    src/FileSandbox.cpp
    src/FileTools.cpp
    src/HttpTools.cpp
    src/UtilityTools.cpp
    src/DefaultTools.cpp
)

add_library(QtOpenAi::Tools ALIAS QtOpenAiTools)

target_link_libraries(QtOpenAiTools
    PUBLIC
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
        Qt6::Network
)

target_include_directories(QtOpenAiTools
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(QtOpenAiTools PRIVATE QTOPENAI_TOOLS_LIBRARY)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(QtOpenAiTools PUBLIC QTOPENAI_TOOLS_STATIC)
endif()

set_target_properties(QtOpenAiTools PROPERTIES
    OUTPUT_NAME QtOpenAiTools
    EXPORT_NAME Tools
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(QTOPENAI_INSTALL)
    install(TARGETS QtOpenAiTools
        EXPORT QtOpenAiTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    # Preserves the QtOpenAi/Tools/... include layout.
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
