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

qtopenai_add_module(Tools
    HEADERS
        ${QTOPENAI_TOOLS_PUBLIC_HEADERS}
    SOURCES
        src/FileSandbox.cpp
        src/FileTools.cpp
        src/HttpTools.cpp
        src/UtilityTools.cpp
        src/DefaultTools.cpp
    LIBS
        QtOpenAi::Client
        QtOpenAi::Core
        Qt6::Core
        Qt6::Network
)
