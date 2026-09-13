# ---------------------------------------------------------------------------
# One QtOpenAi module, declared once.
#
# Every module's CMakeLists.txt ended in the same thirty lines -- the alias, the
# include directories, the export/static defines, the version properties and the
# install rules -- differing in nothing but the module's name. Eight copies is
# eight places for a packaging fix to be applied seven times, which is what
# happened: the exported include directory was hardcoded to <prefix>/include in
# all eight, and correcting it meant eight edits.
#
# What genuinely differs per module is its name, its headers, its sources and
# what it links. Those are the arguments; everything else is here.
#
#     qtopenai_add_module(Chat
#         HEADERS ${QTOPENAI_CHAT_PUBLIC_HEADERS}
#         SOURCES src/Agent.cpp
#         LIBS QtOpenAi::Client QtOpenAi::Core Qt6::Core)
#
# Called from the module's own directory, so the relative paths and
# CMAKE_CURRENT_SOURCE_DIR below are the module's -- a function does not open a
# new directory scope.
# ---------------------------------------------------------------------------

function(qtopenai_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "HEADERS;SOURCES;LIBS")

    set(target "QtOpenAi${name}")
    string(TOUPPER "${name}" upper)

    # Headers are listed as sources so they appear in IDE project trees and so
    # AUTOMOC sees the ones that need it.
    add_library(${target} ${ARG_HEADERS} ${ARG_SOURCES})
    add_library(QtOpenAi::${name} ALIAS ${target})

    target_link_libraries(${target} PUBLIC ${ARG_LIBS})

    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            # Whatever GNUInstallDirs was told, since that is where the headers
            # below are installed. Hardcoding "include" here broke every
            # consumer that set a versioned include directory.
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${QTOPENAI_PRIVATE_INCLUDE_DIR}
    )

    target_compile_definitions(${target} PRIVATE QTOPENAI_${upper}_LIBRARY)
    if(NOT BUILD_SHARED_LIBS)
        target_compile_definitions(${target} PUBLIC QTOPENAI_${upper}_STATIC)
    endif()

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${target}
        EXPORT_NAME ${name}
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    if(QTOPENAI_INSTALL)
        install(TARGETS ${target}
            EXPORT QtOpenAiTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        # The directory rather than the file list, which preserves the
        # QtOpenAi/<Module>/... layout the public headers are included by.
        install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    endif()
endfunction()
