#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Every installed header compiles on its own, as a consumer includes it, under
# -Wall -Wextra -Wpedantic -Wconversion -Wshadow (#206). $QTOPENAI_BUILD_DIR is
# installed into "$QTOPENAI_BUILD_DIR-headers/prefix" and tests/headers is
# built against it with the tree's compiler and Qt: one TU per header, plus
# tests/headers/instantiate.cpp for the class templates.
#
# Warnings fail the check only when the tree has QTOPENAI_WARNINGS_AS_ERRORS
# ON, which only CI's pinned compilers do; elsewhere they are printed. What
# fails everywhere: a header that does not compile alone, and a class
# template that instantiate.cpp does not instantiate.
#
# The template guard reads `template <...> class|struct [EXPORT] Name {|:`,
# with the head on one line or wrapped over several, and skips `//` comments
# and forward declarations. A `/* */` comment inside the head hides it. It does not see function templates or member
# templates -- an explicit class instantiation does not instantiate those, so
# they are checked only where the library instantiates them, under the
# library's own flags -- and each template is checked for the arguments in
# instantiate.cpp only.
#
# The consumer tree is kept, so a rerun with nothing changed compiles nothing.

set -euo pipefail

command -v cmake >/dev/null || {
    echo "cmake not found -- skipped"
    exit 77
}
# tests/headers needs IMPORTED_NO_SYSTEM (3.23); the library itself builds
# with 3.21, so an older cmake is a skip, not a failure.
cmake_version=$(cmake --version | sed -nE '1s/.* ([0-9]+)\.([0-9]+).*/\1 \2/p')
read -r cmake_major cmake_minor <<<"$cmake_version"
if ((cmake_major < 3 || (cmake_major == 3 && cmake_minor < 23))); then
    echo "cmake $cmake_major.$cmake_minor is older than 3.23 -- skipped"
    exit 77
fi

tree=$QTOPENAI_BUILD_DIR
cache=$tree/CMakeCache.txt
if [[ ! -f $cache ]]; then
    echo "no configured tree in $tree -- run the build check first" >&2
    exit 1
fi
# The flags below are GCC/Clang spelling, and explicitly instantiating a
# dllexported class's base clashes with dllimport.
if grep -qsE 'CMAKE_CXX_COMPILER_(ID|FRONTEND_VARIANT) "MSVC"' "$tree"/CMakeFiles/*/CMakeCXXCompiler.cmake; then
    echo "MSVC tree -- skipped"
    exit 77
fi

source "$QTOPENAI_ROOT/scripts/checks/build-type.sh"
cached() { sed -n "s/^$1:[A-Z]*=//p" "$cache" | tr -d '\r'; }
install=$(cached QTOPENAI_INSTALL)
if grep -q '^QTOPENAI_INSTALL:' "$cache" &&
    [[ ${install^^} =~ ^(OFF|0|FALSE|NO|N|IGNORE)?$|-NOTFOUND$ ]]; then
    echo "$tree is configured with QTOPENAI_INSTALL=OFF; this check compiles the installed headers" >&2
    exit 1
fi

failed=0

templates=()
for header in "$QTOPENAI_ROOT"/src/*/include/QtOpenAi/*/*.h; do
    while IFS= read -r name; do
        [[ -n $name ]] && templates+=("$name")
    done < <(grep -v '^[[:space:]]*//' "$header" | sed -E 's|[[:space:]]//.*$||' | tr '\n' ' ' |
        grep -oE 'template[[:space:]]*<[^;{]*>[[:space:]]*(class|struct)[[:space:]]+(QTOPENAI_[A-Z]+_EXPORT[[:space:]]+)?[A-Za-z_][A-Za-z0-9_]*[[:space:]]*(final[[:space:]]*)?[:{]' |
        sed -E 's/.*(class|struct)[[:space:]]+(QTOPENAI_[A-Z]+_EXPORT[[:space:]]+)?([A-Za-z_][A-Za-z0-9_]*).*/\3/' || true)
done
instantiate=$QTOPENAI_ROOT/tests/headers/instantiate.cpp
for name in ${templates[@]+"${templates[@]}"}; do
    if ! grep -qE "^[[:space:]]*template[[:space:]]+(class|struct)[[:space:]]+([A-Za-z_:]*::)?$name<" "$instantiate"; then
        echo "class template $name is not instantiated in tests/headers/instantiate.cpp" >&2
        failed=1
    fi
done

dir=$tree-headers
includedir=$(cached CMAKE_INSTALL_INCLUDEDIR)
# Wiped first, so a header deleted from the source cannot linger in the prefix.
# The install keeps mtimes, so the TUs of unchanged headers are not rebuilt.
rm -rf "$dir/prefix/${includedir:-include}"
cmake --install "$tree" --prefix "$dir/prefix" --config "$build_type" >/dev/null

unset CMAKE_BUILD_TYPE
# Keep going past the first failing TU, so one run names every site.
generator=(-G "Unix Makefiles")
keep_going=(-- -k)
if command -v ninja >/dev/null; then
    generator=(-G Ninja)
    keep_going=(-- -k 0)
fi
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

# From an empty cache every time, like 60-subproject.sh, so nothing a
# previous run was given -- the option above all -- can outlive a change to
# the main tree. The objects are kept.
rm -f "$dir/build/CMakeCache.txt"
# QtOpenAi_DIR names the prefix just installed, so neither a QtOpenAi_ROOT in
# the environment nor check.sh's QTOPENAI_ROOT (read as <Name>_ROOT, CMP0144)
# can point find_package at another install.
env -u QTOPENAI_ROOT -u QtOpenAi_ROOT cmake -S "$QTOPENAI_ROOT/tests/headers" -B "$dir/build" "${generator[@]}" \
    -DCMAKE_CXX_COMPILER="$(cached CMAKE_CXX_COMPILER)" \
    -DQt6_DIR="$(cached Qt6_DIR)" \
    -DQtOpenAi_DIR="$dir/prefix/$(cached QTOPENAI_INSTALL_CMAKEDIR)" \
    -DQTOPENAI_WARNINGS_AS_ERRORS="$(cached QTOPENAI_WARNINGS_AS_ERRORS)" >/dev/null
cmake --build "$dir/build" --parallel "$jobs" "${keep_going[@]}" || failed=1

exit $failed
