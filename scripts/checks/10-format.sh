#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# clang-format over every tracked and untracked C++ source. The work is in
# check-format.sh, which predates this runner and is still the thing to call
# with --fix.

set -euo pipefail

command -v "${CLANG_FORMAT:-clang-format}" >/dev/null || {
    echo "clang-format not found -- skipped"
    exit 77
}

exec "$QTOPENAI_ROOT/scripts/check-format.sh"
