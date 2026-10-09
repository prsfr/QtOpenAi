#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Configure (once) and build the library, tests and examples into
# $QTOPENAI_BUILD_DIR. An existing tree is reused as it is configured, so
# `--build-dir` can point the checks at a build made some other way.
#
# QTOPENAI_CHECK_BUILD_TYPE picks the configuration of a fresh tree (default
# Debug: the fastest to compile). For an existing single-config tree the
# configuration it was made with wins, so the build and test checks never
# disagree about which binaries they mean.

set -euo pipefail

command -v cmake >/dev/null || {
    echo "cmake not found -- skipped"
    exit 77
}

source "$QTOPENAI_ROOT/scripts/checks/build-type.sh"

if [[ ! -f $QTOPENAI_BUILD_DIR/CMakeCache.txt ]]; then
    generator=()
    command -v ninja >/dev/null && generator=(-G Ninja)
    cmake -S "$QTOPENAI_ROOT" -B "$QTOPENAI_BUILD_DIR" ${generator[@]+"${generator[@]}"} \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DQTOPENAI_BUILD_TESTS=ON \
        -DQTOPENAI_BUILD_EXAMPLES=ON
fi

# Bounded parallelism: an unbounded `make -j` once took a CI runner down with
# it (see the Build step in ci.yml).
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
cmake --build "$QTOPENAI_BUILD_DIR" --config "$build_type" --parallel "$jobs"
