#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The declared Qt minimum is real (#205): it is written down once, in
# QTOPENAI_MIN_QT_VERSION, the installed package asks consumers for it, CI
# has a job that builds against it -- and the tree in $QTOPENAI_BUILD_DIR was
# built against it. A newer Qt in the tree is a skip, not a pass: the check
# says something only where the minimum is what got compiled. The CI job that
# installs the minimum names it, so a skip there fails under --strict.

set -euo pipefail

root=$QTOPENAI_ROOT
fail() {
    echo "$1" >&2
    exit 1
}

# The declaration first, so a broken one fails on any machine, whatever Qt the
# tree has.
sets=$(grep -cE '^set\(QTOPENAI_MIN_QT_VERSION [0-9]+\.[0-9]+\)' "$root/CMakeLists.txt" || true)
[[ $sets -eq 1 ]] || fail "CMakeLists.txt must set QTOPENAI_MIN_QT_VERSION <major>.<minor> exactly once (found $sets)"
min=$(sed -nE 's/^set\(QTOPENAI_MIN_QT_VERSION ([0-9]+\.[0-9]+)\)/\1/p' "$root/CMakeLists.txt")

for file in CMakeLists.txt tests/CMakeLists.txt; do
    if grep -nE '^\s*find_package\(Qt6 [0-9]' "$root/$file"; then
        fail "$file names a Qt version itself; use \${QTOPENAI_MIN_QT_VERSION}"
    fi
done
uses=$(cat "$root/CMakeLists.txt" "$root/tests/CMakeLists.txt" |
    grep -cE '^\s*find_package\(Qt6 \$\{QTOPENAI_MIN_QT_VERSION\} ' || true)
[[ $uses -eq 4 ]] || fail "expected 4 find_package(Qt6 \${QTOPENAI_MIN_QT_VERSION} ...) calls, found $uses"

job="name: linux / Qt $min (distro)"
grep -qF "$job" "$root/.github/workflows/ci.yml" || fail "ci.yml has no job '$job'"

cache=$QTOPENAI_BUILD_DIR/CMakeCache.txt
[[ -f $cache ]] || fail "no configured tree in $QTOPENAI_BUILD_DIR -- run the build check first"

config=$QTOPENAI_BUILD_DIR/QtOpenAiConfig.cmake
[[ -f $config ]] || fail "$config is missing"
if grep -nE '^\s*find_dependency\(Qt6 COMPONENTS' "$config"; then
    fail "the package config asks consumers for Qt6 without a version"
fi

# From the tree, not from qmake6, which may belong to another Qt.
dir=$(sed -n 's/^Qt6Core_DIR:[A-Z]*=//p' "$cache")
version=
for file in "$dir/Qt6CoreConfigVersionImpl.cmake" "$dir/Qt6CoreConfigVersion.cmake"; do
    [[ -f $file ]] || continue
    version=$(sed -nE 's/^\s*set\(PACKAGE_VERSION "([0-9.]+)"\)/\1/p' "$file" | head -n1)
    [[ -n $version ]] && break
done
[[ -n $version ]] || fail "cannot read the Qt version of the tree (Qt6Core_DIR='$dir')"

if [[ ${version%.*} != "$min" ]]; then
    echo "tree is built against Qt $version, not the minimum $min -- skipped"
    exit 77
fi
echo "tree is built against Qt $version, the declared minimum"
