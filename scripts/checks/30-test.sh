#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The whole ctest suite against $QTOPENAI_BUILD_DIR. Every test runs offline
# against the stub servers in tests/support, so a failure is a failure, never
# the network.
#
# QTOPENAI_CHECK_TEST_REPEAT=N runs each test until it fails, at most N times.
# The QA stage of AGENTS.md uses 3: a test that passes once and fails on a
# rerun is a defect in the test or the code, and the place to find out is
# before the pull request, not in CI.

set -euo pipefail

if [[ ! -f $QTOPENAI_BUILD_DIR/CTestTestfile.cmake ]]; then
    echo "no build in $QTOPENAI_BUILD_DIR -- run the build check first" >&2
    exit 1
fi

source "$QTOPENAI_ROOT/scripts/checks/build-type.sh"
repeat=${QTOPENAI_CHECK_TEST_REPEAT:-1}
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

QT_QPA_PLATFORM=offscreen ctest --test-dir "$QTOPENAI_BUILD_DIR" \
    --build-config "$build_type" \
    --output-on-failure --parallel "$jobs" --repeat "until-fail:$repeat"
