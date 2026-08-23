# Private headers shared by every module: the JSON and multipart conventions of
# the API, and the shape of a REST path. Not installed, not exported -- an
# include directory rather than a target, so nothing about it can leak into the
# package config. See src/common/ for why they are not under one module.
set(QTOPENAI_PRIVATE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")
set(QTOPENAI_PRIVATE_HEADERS
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/FormFields_p.h"
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/JsonHelpers_p.h"
    "${QTOPENAI_PRIVATE_INCLUDE_DIR}/RestPath_p.h"
)

add_subdirectory(core)
add_subdirectory(client)
add_subdirectory(chat)
add_subdirectory(tools)
add_subdirectory(storage)
add_subdirectory(admin)
if(QTOPENAI_BUILD_REALTIME)
    add_subdirectory(realtime)
endif()
if(QTOPENAI_BUILD_SQL)
    add_subdirectory(sql)
endif()
