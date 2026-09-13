# Private headers shared by every module: the JSON and multipart conventions of
# the API, and the shape of a REST path. Not installed, not exported -- an
# include directory rather than a target, so nothing about it can leak into the
# package config. See src/common/ for why they are not under one module.
# Every module below is declared with the same shape; see cmake/QtOpenAiModule.cmake.
include("${CMAKE_SOURCE_DIR}/cmake/QtOpenAiModule.cmake")

set(QTOPENAI_PRIVATE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")
set(QTOPENAI_PRIVATE_HEADERS
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/FormFields_p.h"
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/JsonHelpers_p.h"
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/RestPath_p.h"
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/UrlQuery_p.h"
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/WireTable_p.h"
)

# The component names find_package(QtOpenAi COMPONENTS ...) accepts, which is
# by definition the set of modules built below: QtOpenAiConfig.cmake.in turns
# each one into the QtOpenAi_<name>_FOUND flag check_required_components()
# looks for. The two optional modules append themselves where they are added,
# so a component only exists when the module behind it does -- which is what
# lets a consumer say "I need Realtime" and be told at find_package() time.
set(QTOPENAI_COMPONENTS Core Client Chat Tools Storage Admin)

add_subdirectory(core)
add_subdirectory(client)
add_subdirectory(chat)
add_subdirectory(tools)
add_subdirectory(storage)
add_subdirectory(admin)
if(QTOPENAI_BUILD_REALTIME)
    add_subdirectory(realtime)
    list(APPEND QTOPENAI_COMPONENTS Realtime)
endif()
if(QTOPENAI_BUILD_SQL)
    add_subdirectory(sql)
    list(APPEND QTOPENAI_COMPONENTS Sql)
endif()

set(QTOPENAI_COMPONENTS "${QTOPENAI_COMPONENTS}" PARENT_SCOPE)
