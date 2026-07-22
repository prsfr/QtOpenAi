cmake_minimum_required(VERSION 3.21)

project(QtOpenAi
    VERSION 0.2.0
    DESCRIPTION "Qt 6 client library for OpenAI-compatible chat completion APIs with tool calling"
    HOMEPAGE_URL "https://github.com/prsfr/qtopenai"
    LANGUAGES CXX)

# --------------------------------------------------------------------------
# Standard / policy setup
# --------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Treat this as the top-level project only when configured directly.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(QTOPENAI_IS_TOPLEVEL ON)
else()
    set(QTOPENAI_IS_TOPLEVEL OFF)
endif()

option(QTOPENAI_BUILD_TESTS    "Build QtOpenAi unit tests"   ${QTOPENAI_IS_TOPLEVEL})
option(QTOPENAI_BUILD_EXAMPLES "Build QtOpenAi examples"     ${QTOPENAI_IS_TOPLEVEL})
option(QTOPENAI_BUILD_SHARED   "Build QtOpenAi as shared libraries" ON)
option(QTOPENAI_INSTALL        "Generate QtOpenAi install/export rules" ${QTOPENAI_IS_TOPLEVEL})

include(GNUInstallDirs)
set(QTOPENAI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/QtOpenAi"
    CACHE STRING "Install location for QtOpenAi CMake package files")

if(QTOPENAI_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Put all runtime artefacts in one place so tests find the shared libs.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

# --------------------------------------------------------------------------
# Subdirectories
# --------------------------------------------------------------------------
add_subdirectory(src)

if(QTOPENAI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(QTOPENAI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# --------------------------------------------------------------------------
# Install / export: make the project consumable via find_package(QtOpenAi).
# The per-target install(TARGETS ... EXPORT QtOpenAiTargets) rules live in the
# module CMakeLists; here we emit the export set and the package config files.
# --------------------------------------------------------------------------
if(QTOPENAI_INSTALL)
    include(CMakePackageConfigHelpers)

    install(EXPORT QtOpenAiTargets
        FILE QtOpenAiTargets.cmake
        NAMESPACE QtOpenAi::
        DESTINATION "${QTOPENAI_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtOpenAiConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/QtOpenAiConfig.cmake"
        INSTALL_DESTINATION "${QTOPENAI_INSTALL_CMAKEDIR}")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/QtOpenAiConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtOpenAiConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/QtOpenAiConfigVersion.cmake"
        DESTINATION "${QTOPENAI_INSTALL_CMAKEDIR}")

    # Also make the build tree consumable without installing.
    export(EXPORT QtOpenAiTargets
        NAMESPACE QtOpenAi::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/QtOpenAiTargets.cmake")
endif()
