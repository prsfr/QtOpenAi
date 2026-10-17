# Every installed QtOpenAi header, compiled on its own the way a consumer
# compiles it (#206). scripts/checks/45-headers.sh installs the tree and
# configures this project against the prefix; it is not part of the main build.
#
# A find_package consumer gets imported include directories as -isystem and so
# sees no warning from them; an add_subdirectory, FetchContent or non-CMake
# consumer sees every one. IMPORTED_NO_SYSTEM gives QtOpenAi's headers that
# second, plain -I treatment while Qt stays -isystem, as it does for anyone.
cmake_minimum_required(VERSION 3.23)
project(qtopenai_headers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The same modules as tests/package; Realtime and Sql only when installed.
find_package(QtOpenAi REQUIRED COMPONENTS Core Client Chat Tools Storage Admin)
set(modules Core Client Chat Tools Storage Admin)
if(TARGET QtOpenAi::Realtime)
    list(APPEND modules Realtime)
endif()
if(TARGET QtOpenAi::Sql)
    list(APPEND modules Sql)
endif()

set(targets)
foreach(module IN LISTS modules)
    set_target_properties(QtOpenAi::${module} PROPERTIES IMPORTED_NO_SYSTEM ON)
    list(APPEND targets QtOpenAi::${module})
endforeach()

# One TU per installed header, holding nothing but its #include. Written only
# when the content changes, so a rerun with the same headers compiles nothing.
get_target_property(include_dirs QtOpenAi::Core INTERFACE_INCLUDE_DIRECTORIES)
list(GET include_dirs 0 include_dir)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS RELATIVE "${include_dir}" "${include_dir}/QtOpenAi/*.h")
list(SORT headers)
set(sources "${CMAKE_CURRENT_SOURCE_DIR}/instantiate.cpp")
foreach(header IN LISTS headers)
    string(REGEX REPLACE "[/.]" "_" stem "${header}")
    set(tu "${CMAKE_CURRENT_BINARY_DIR}/tu/${stem}.cpp")
    file(CONFIGURE OUTPUT "${tu}" CONTENT "#include <${header}>\n" @ONLY)
    list(APPEND sources "${tu}")
endforeach()
list(LENGTH headers count)
message(STATUS "QtOpenAi headers: ${count} under ${include_dir}")

add_library(headers OBJECT ${sources})
target_link_libraries(headers PRIVATE ${targets})
target_compile_options(headers PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
# The main tree's cache value, passed through unchanged, and read with the
# same if() rule the library's gate uses.
if(QTOPENAI_WARNINGS_AS_ERRORS)
    target_compile_options(headers PRIVATE -Werror)
endif()
