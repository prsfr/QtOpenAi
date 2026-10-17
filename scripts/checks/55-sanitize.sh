#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The whole test suite under AddressSanitizer, LeakSanitizer and
# UndefinedBehaviorSanitizer, in a tree next to the main one,
# "$QTOPENAI_BUILD_DIR-sanitize" (#206). Any report fails the test it came
# from. LSAN_OPTIONS points at scripts/sanitizers/lsan.supp when it exists;
# it is for leaks inside Qt only, one commented entry each.
#
# The tree leaves QTOPENAI_WARNINGS_AS_ERRORS at its default OFF: the
# instrumentation can raise GCC diagnostics (stringop, array-bounds) that the
# warning gate is not about.
#
# Linux only. A full second build: run it by name (scripts/check.sh sanitize).
# CI always does.
# check.sh: explicit-only

set -euo pipefail

[[ $(uname -s) == Linux ]] || {
    echo "not Linux -- skipped"
    exit 77
}
command -v cmake >/dev/null || {
    echo "cmake not found -- skipped"
    exit 77
}

flags='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined'
link_flags='-fsanitize=address,undefined'

main=$QTOPENAI_BUILD_DIR
export QTOPENAI_BUILD_DIR="$main-sanitize"

cache=$QTOPENAI_BUILD_DIR/CMakeCache.txt
if [[ -f $cache ]]; then
    # Reused as configured -- but never an uninstrumented one, or this would
    # pass without testing what it says it tests.
    grep -qxF "CMAKE_CXX_FLAGS:STRING=$flags" "$cache" &&
        grep -qxF "CMAKE_EXE_LINKER_FLAGS:STRING=$link_flags" "$cache" &&
        grep -qxF "CMAKE_SHARED_LINKER_FLAGS:STRING=$link_flags" "$cache" &&
        grep -qx 'QTOPENAI_BUILD_TESTS:BOOL=ON' "$cache" &&
        grep -qx 'CMAKE_BUILD_TYPE:STRING=Debug' "$cache" &&
        ! grep -qxE 'QTOPENAI_BUILD_REALTIME:BOOL=(OFF|0|FALSE|NO)' "$cache" &&
        ! grep -qxE 'QTOPENAI_WARNINGS_AS_ERRORS:BOOL=(ON|1|TRUE|YES)' "$cache" || {
        echo "$QTOPENAI_BUILD_DIR is not this check's sanitizer build (Debug, tests on, Realtime not off, warnings option off); remove it or point --build-dir elsewhere" >&2
        exit 1
    }
else
    generator=(-G "Unix Makefiles")
    command -v ninja >/dev/null && generator=(-G Ninja)
    cmake -S "$QTOPENAI_ROOT" -B "$QTOPENAI_BUILD_DIR" "${generator[@]}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="$flags" \
        -DCMAKE_EXE_LINKER_FLAGS="$link_flags" \
        -DCMAKE_SHARED_LINKER_FLAGS="$link_flags" \
        -DQTOPENAI_BUILD_TESTS=ON \
        -DQTOPENAI_BUILD_EXAMPLES=OFF
fi

bash "$QTOPENAI_ROOT/scripts/checks/20-build.sh"

# Set outright, never inherited: a detect_leaks=0 or exitcode=0 already in the
# environment would let the check pass on the reports it exists to fail on.
export ASAN_OPTIONS=detect_leaks=1
export UBSAN_OPTIONS=print_stacktrace=1
export LSAN_OPTIONS=
supp=$QTOPENAI_ROOT/scripts/sanitizers/lsan.supp
[[ -f $supp ]] && LSAN_OPTIONS="suppressions=$supp"
bash "$QTOPENAI_ROOT/scripts/checks/30-test.sh"
