#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The whole test suite against static QtOpenAi libraries, in a tree next to
# the main one, "$QTOPENAI_BUILD_DIR-static" (#210). Static-only mistakes --
# a QTOPENAI_<MODULE>_STATIC define not reaching a target, initialisation
# order -- never show in the shared build.
#
# A full second build: run it by name (scripts/check.sh static). CI always does.
# check.sh: explicit-only

set -euo pipefail

command -v cmake >/dev/null || {
    echo "cmake not found -- skipped"
    exit 77
}

main=$QTOPENAI_BUILD_DIR
export QTOPENAI_BUILD_DIR="$main-static"
source "$QTOPENAI_ROOT/scripts/checks/build-type.sh"

cache=$QTOPENAI_BUILD_DIR/CMakeCache.txt
if [[ -f $cache ]]; then
    # Reused as configured, like the main tree -- but never a shared one, or
    # this would pass without testing what it says it tests.
    grep -qx 'QTOPENAI_BUILD_SHARED:BOOL=OFF' "$cache" && grep -qx 'QTOPENAI_BUILD_TESTS:BOOL=ON' "$cache" || {
        echo "$QTOPENAI_BUILD_DIR is not a static build with tests; remove it or point --build-dir elsewhere" >&2
        exit 1
    }
else
    # Configured here, before the build check sees the directory: on an empty
    # one it would configure a shared tree.
    generator=()
    if command -v ninja >/dev/null; then
        generator=(-G Ninja)
    elif [[ $(uname -s) != MINGW* && $(uname -s) != MSYS* && $(uname -s) != CYGWIN* ]]; then
        generator=(-G "Unix Makefiles")
    fi
    cmake -S "$QTOPENAI_ROOT" -B "$QTOPENAI_BUILD_DIR" ${generator[@]+"${generator[@]}"} \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DQTOPENAI_BUILD_SHARED=OFF \
        -DQTOPENAI_BUILD_TESTS=ON \
        -DQTOPENAI_BUILD_EXAMPLES=OFF
fi

bash "$QTOPENAI_ROOT/scripts/checks/20-build.sh"
bash "$QTOPENAI_ROOT/scripts/checks/30-test.sh"
