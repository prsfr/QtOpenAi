#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Line, branch and function coverage of src/ by the whole test suite, against
# the baseline in tests/coverage/ (#173). The build is a tree next to the main
# one, "$QTOPENAI_BUILD_DIR-coverage", configured with QTOPENAI_COVERAGE=ON;
# scripts/checks/coverage-compare.py prints the per-module table and gives the
# verdict:
#   - a module's line or branch % more than 0.5 pt below its baseline fails;
#   - an uncalled function missing from uncalled-functions.txt fails;
#   - a listed function that is called now, or gone, fails (a stale entry);
#   - a module measured but not in the baseline, or the other way round, fails;
#   - a module more than 1.0 pt above its baseline warns, asking for an update.
#
# The tree must sit inside the source tree: moc includes headers by relative
# path, and from outside gcov cannot open them -- which gcovr reports, and
# which fails the check rather than quietly losing those functions.
#
# The baseline holds for one compiler, gcovr and Qt (Qt's macros expand into
# src/ lines). Any other toolchain still measures and prints the table, then
# skips -- a FAIL under --strict, as in CI.
#
# QTOPENAI_COVERAGE_UPDATE_BASELINE=1 writes the measurement into
# tests/coverage/ instead, printing which way each module moved.
#
# A full second build: run it by name (scripts/check.sh coverage). CI always
# does.
# check.sh: explicit-only

set -euo pipefail

command -v cmake >/dev/null || {
    echo "cmake not found -- skipped"
    exit 77
}
command -v gcovr >/dev/null || {
    echo "gcovr not found -- skipped"
    exit 77
}
command -v python3 >/dev/null || {
    echo "python3 not found -- skipped"
    exit 77
}

main=$QTOPENAI_BUILD_DIR
export QTOPENAI_BUILD_DIR="$main-coverage"
tree=$QTOPENAI_BUILD_DIR

cache=$tree/CMakeCache.txt
if [[ -f $cache ]]; then
    # Reused as configured -- but never an uninstrumented one, or one built
    # differently from the tree the baseline came from.
    grep -qx 'CMAKE_BUILD_TYPE:STRING=Debug' "$cache" &&
        grep -qx 'QTOPENAI_COVERAGE:BOOL=ON' "$cache" &&
        grep -qx 'QTOPENAI_BUILD_TESTS:BOOL=ON' "$cache" &&
        grep -qx 'QTOPENAI_BUILD_EXAMPLES:BOOL=OFF' "$cache" &&
        grep -qx 'QTOPENAI_WARNINGS_AS_ERRORS:BOOL=OFF' "$cache" || {
        echo "$tree is not this check's coverage build (Debug, coverage on, tests on, examples off, warnings option off); remove it or point --build-dir elsewhere" >&2
        exit 1
    }
else
    generator=(-G "Unix Makefiles")
    command -v ninja >/dev/null && generator=(-G Ninja)
    cmake -S "$QTOPENAI_ROOT" -B "$tree" "${generator[@]}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DQTOPENAI_COVERAGE=ON \
        -DQTOPENAI_BUILD_TESTS=ON \
        -DQTOPENAI_BUILD_EXAMPLES=OFF
fi

# Counters from an earlier run would add to this one.
find "$tree" -name '*.gcda' -delete

bash "$QTOPENAI_ROOT/scripts/checks/20-build.sh"
export QTOPENAI_CHECK_TEST_REPEAT=1
bash "$QTOPENAI_ROOT/scripts/checks/30-test.sh"

# The toolchain the baseline is pinned to. Every CMake version that ever
# configured the tree leaves its own CMakeFiles/<version>/, so they must agree.
compiler_value() {
    sed -nE "s/^set\\(CMAKE_CXX_COMPILER_$1 \"([^\"]*)\"\\)/\\1/p" "$tree"/CMakeFiles/*/CMakeCXXCompiler.cmake | sort -u
}
compiler_id=$(compiler_value ID)
compiler_version=$(compiler_value VERSION)
if [[ -z $compiler_id || -z $compiler_version || $compiler_id == *$'\n'* || $compiler_version == *$'\n'* ]]; then
    echo "cannot read one compiler id and version from $tree/CMakeFiles/*/CMakeCXXCompiler.cmake" >&2
    exit 1
fi
gcovr_version=$(gcovr --version | sed -n '1s/^gcovr //p')
# From the tree, not from qmake6, which may belong to another Qt (as in
# 70-qt-min.sh).
dir=$(sed -n 's/^Qt6Core_DIR:[A-Z]*=//p' "$cache" | tr -d '\r')
qt_version=
for file in "$dir/Qt6CoreConfigVersionImpl.cmake" "$dir/Qt6CoreConfigVersion.cmake"; do
    [[ -f $file ]] || continue
    qt_version=$(sed -nE 's/^\s*set\(PACKAGE_VERSION "([0-9.]+)"\)/\1/p' "$file" | head -n1)
    [[ -n $qt_version ]] && break
done
if [[ -z $qt_version ]]; then
    echo "cannot read the Qt version of the tree (Qt6Core_DIR='$dir')" >&2
    exit 1
fi

out=$tree/coverage
rm -rf "$out"
mkdir -p "$out"
# From the root: the relative --filter is matched against paths relative to
# the working directory. The trailing tree keeps gcovr to this tree's
# counters, not those of any other coverage tree in the checkout.
cd "$QTOPENAI_ROOT"
status=0
gcovr --root "$QTOPENAI_ROOT" --object-directory "$tree" \
    --filter src/ --exclude '.*_autogen.*' \
    --exclude-unreachable-branches --exclude-throw-branches \
    --json "$out/coverage.json" \
    --json-summary "$out/coverage-summary.json" \
    --txt "$out/coverage.txt" \
    --html-details "$out/coverage.html" \
    "$tree" || status=$?
if [[ $status -ne 0 ]]; then
    echo "gcovr failed (exit $status). Likely causes:" >&2
    echo "  - the coverage tree is outside the source tree, so gcov cannot open the headers moc includes ($tree)" >&2
    echo "  - stale objects from a removed or renamed source: remove $tree and rerun" >&2
    echo "  - gcov does not match the compiler (e.g. a Clang build read by GCC's gcov)" >&2
    exit 1
fi

update=()
[[ ${QTOPENAI_COVERAGE_UPDATE_BASELINE:-} == 1 ]] && update=(--update)
python3 "$QTOPENAI_ROOT/scripts/checks/coverage-compare.py" "$out/coverage.json" \
    --baseline-dir "$QTOPENAI_ROOT/tests/coverage" --out-dir "$out" \
    --compiler "$compiler_id $compiler_version" --gcovr "$gcovr_version" --qt "$qt_version" \
    ${update[@]+"${update[@]}"}
