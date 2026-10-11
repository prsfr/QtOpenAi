# Consumer that pulls QtOpenAi in from source, the two ways the README offers
# (add_subdirectory and FetchContent), and fails at configure time if that
# changed anything the parent already had (#210). Run by
# scripts/checks/60-subproject.sh, configured with no build type.
cmake_minimum_required(VERSION 3.21)
project(qtopenai_subproject_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QTOPENAI_SUBPROJECT_MODE "add_subdirectory" CACHE STRING
    "How to include QtOpenAi: add_subdirectory or FetchContent")

# Qt first, so the snapshot below isolates what QtOpenAi does from what Qt does.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Network)

get_cmake_property(_before CACHE_VARIABLES)
foreach(_name IN LISTS _before)
    get_property(_type CACHE "${_name}" PROPERTY TYPE)
    set("_before_type_${_name}" "${_type}")
    set("_before_value_${_name}" "$CACHE{${_name}}")
endforeach()
set(_build_type_before "${CMAKE_BUILD_TYPE}")

set(_repo "${CMAKE_CURRENT_SOURCE_DIR}/../..")
if(QTOPENAI_SUBPROJECT_MODE STREQUAL "add_subdirectory")
    add_subdirectory("${_repo}" qtopenai)
elseif(QTOPENAI_SUBPROJECT_MODE STREQUAL "FetchContent")
    include(FetchContent)
    FetchContent_Declare(qtopenai SOURCE_DIR "${_repo}")
    FetchContent_MakeAvailable(qtopenai)
else()
    message(FATAL_ERROR "unknown QTOPENAI_SUBPROJECT_MODE '${QTOPENAI_SUBPROJECT_MODE}'")
endif()

# Every entry, not only the first: a CI log has to be enough to see what moved.
set(_changed "")
foreach(_name IN LISTS _before)
    get_property(_type CACHE "${_name}" PROPERTY TYPE)
    if(NOT _type STREQUAL _before_type_${_name}
       OR NOT "$CACHE{${_name}}" STREQUAL "${_before_value_${_name}}")
        string(APPEND _changed "\n  ${_name}:${_before_type_${_name}}='${_before_value_${_name}}'"
                               " -> ${_type}='$CACHE{${_name}}'")
    endif()
endforeach()
if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "${_build_type_before}")
    string(APPEND _changed "\n  CMAKE_BUILD_TYPE (variable): '${_build_type_before}'"
                           " -> '${CMAKE_BUILD_TYPE}'")
endif()
if(_changed)
    message(FATAL_ERROR "including QtOpenAi (${QTOPENAI_SUBPROJECT_MODE}) changed the parent:${_changed}")
endif()

if(DEFINED BUILD_SHARED_LIBS AND NOT BUILD_SHARED_LIBS)
    set(_expected STATIC_LIBRARY)
else()
    set(_expected SHARED_LIBRARY)
endif()
get_target_property(_type QtOpenAiClient TYPE)
if(NOT _type STREQUAL _expected)
    message(FATAL_ERROR "QtOpenAiClient is ${_type}, expected ${_expected}"
                        " (BUILD_SHARED_LIBS='${BUILD_SHARED_LIBS}')")
endif()

# The installed-package consumer's program, linked the same way.
add_executable(consumer ../package/main.cpp)
target_link_libraries(consumer
    PRIVATE QtOpenAi::Client QtOpenAi::Chat QtOpenAi::Tools QtOpenAi::Storage QtOpenAi::Admin)
if(TARGET QtOpenAi::Realtime)
    target_link_libraries(consumer PRIVATE QtOpenAi::Realtime)
    target_compile_definitions(consumer PRIVATE QTOPENAI_CONSUMER_HAS_REALTIME)
endif()
if(TARGET QtOpenAi::Sql)
    target_link_libraries(consumer PRIVATE QtOpenAi::Sql)
    target_compile_definitions(consumer PRIVATE QTOPENAI_CONSUMER_HAS_SQL)
endif()
