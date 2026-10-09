# SPDX-License-Identifier: MIT
#
# Sourced by the build and test checks, not run (no NN- prefix, so check.sh
# does not list it). Sets $build_type: the configuration an existing
# single-config tree was made with, else QTOPENAI_CHECK_BUILD_TYPE, else Debug.

build_type=${QTOPENAI_CHECK_BUILD_TYPE:-Debug}
if [[ -f $QTOPENAI_BUILD_DIR/CMakeCache.txt ]]; then
    cached=$(sed -n 's/^CMAKE_BUILD_TYPE:[A-Z]*=//p' "$QTOPENAI_BUILD_DIR/CMakeCache.txt")
    [[ -n $cached ]] && build_type=$cached
fi
