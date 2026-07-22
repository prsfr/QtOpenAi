# Standalone consumer project: verifies QtOpenAi is usable via
# find_package(QtOpenAi) with the installed (or exported) package.
# Configured separately from the main build with CMAKE_PREFIX_PATH pointing at
# the QtOpenAi install prefix.
cmake_minimum_required(VERSION 3.21)
project(qtopenai_package_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QtOpenAi REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE QtOpenAi::Client)
