find_package(Qt6 6.4 REQUIRED COMPONENTS Test)

# Helper to declare a QtTest-based unit test with a consistent naming scheme.
function(qtopenai_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

# --- Core module tests ---
qtopenai_add_test(tst_core_serialization
    SOURCES core/tst_serialization.cpp
    LIBS QtOpenAi::Core)

qtopenai_add_test(tst_core_valuesemantics
    SOURCES core/tst_valuesemantics.cpp
    LIBS QtOpenAi::Core)

# --- Client module tests ---
qtopenai_add_test(tst_client_toolregistry
    SOURCES client/tst_toolregistry.cpp
    LIBS QtOpenAi::Client)

qtopenai_add_test(tst_client_reply
    SOURCES client/tst_reply.cpp
    LIBS QtOpenAi::Client)
