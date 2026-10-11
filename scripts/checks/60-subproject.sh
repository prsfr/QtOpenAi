#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# QtOpenAi included from source, by add_subdirectory and by FetchContent, into
# tests/subproject, which fails its configure if the inclusion changed any of
# the parent's cache (#210). Each configure starts from an empty cache -- the
# point is what a first configure does -- but keeps the object files, so a
# rerun is incremental. Trees live next to the main one, in
# "$QTOPENAI_BUILD_DIR-subproject".

set -euo pipefail

command -v cmake >/dev/null || {
    echo "cmake not found -- skipped"
    exit 77
}

case $(uname -s) in
    MINGW* | MSYS* | CYGWIN*)
        # Running the consumer needs Qt's DLLs on PATH. No CI job runs this
        # check on Windows; a full-set --strict run there would turn this skip
        # into a FAIL.
        echo "not run on Windows -- skipped"
        exit 77
        ;;
esac

# No build type, from anywhere: CMake >= 3.22 reads it from the environment.
unset CMAKE_BUILD_TYPE
# Single-config, whatever CMAKE_GENERATOR says: on a multi-config generator
# there is no build type to protect and the assertion would prove nothing.
generator=(-G "Unix Makefiles")
command -v ninja >/dev/null && generator=(-G Ninja)
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

consume() {
    local dir="$QTOPENAI_BUILD_DIR-subproject/$1"
    shift
    mkdir -p "$dir"
    rm -f "$dir/CMakeCache.txt"
    cmake -S "$QTOPENAI_ROOT/tests/subproject" -B "$dir" "${generator[@]}" "$@"
    cmake --build "$dir" --parallel "$jobs"
    QT_QPA_PLATFORM=offscreen "$dir/consumer"
}

consume add_subdirectory -DQTOPENAI_SUBPROJECT_MODE=add_subdirectory
consume fetchcontent -DQTOPENAI_SUBPROJECT_MODE=FetchContent -DBUILD_SHARED_LIBS=OFF
