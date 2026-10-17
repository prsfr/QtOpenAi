#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The warning gate is CI's, on pinned compilers (#206). Locally warnings are
# printed and never fail a check, so no local verdict depends on the compiler;
# CI's build-and-test configures with QTOPENAI_WARNINGS_AS_ERRORS=ON on
# runners whose compiler does not move under it. This holds that arrangement
# in place:
#
#   (a) build-and-test's Configure step sets the option ON;
#   (b) no runner label in ci.yml ends in -latest, which would change the
#       compiler under the gate with no change here;
#   (c) the option defaults OFF, so no local build is gated;
#   (d) the macOS job selects its Xcode by path -- Apple Clang moves inside a
#       macOS label.
#
# Greps over committed files only: no build tree, the same answer everywhere.

set -euo pipefail

root=$QTOPENAI_ROOT
ci=$root/.github/workflows/ci.yml
failed=0
fail() {
    echo "$1" >&2
    failed=1
}

# From `  build-and-test:` to the next job.
job=$(awk '/^  build-and-test:$/ {in_job = 1; next} in_job && /^  [A-Za-z0-9_-]+:$/ {exit} in_job' "$ci")
configure=$(awk '/^      - name: Configure$/ {in_step = 1; print; next} in_step && /^      - name:/ {exit} in_step' <<<"$job")
grep -qxF '          -DQTOPENAI_WARNINGS_AS_ERRORS=ON' <<<"$configure" ||
    fail "(a) build-and-test's Configure step must pass -DQTOPENAI_WARNINGS_AS_ERRORS=ON"

latest=$(grep -nE "^[[:space:]]*(-[[:space:]]+)?(runs-on|os):[[:space:]]*[\"']?[A-Za-z0-9._-]*-latest[\"']?[[:space:]]*(#.*)?$" "$ci" || true)
[[ -z $latest ]] || fail "(b) ci.yml runs on a moving label; pin it:
$latest"

option=$(grep -E '^[[:space:]]*option\(QTOPENAI_WARNINGS_AS_ERRORS[[:space:]]' "$root/CMakeLists.txt" || true)
[[ $option =~ OFF\)[[:space:]]*$ ]] ||
    fail "(c) CMakeLists.txt must declare option(QTOPENAI_WARNINGS_AS_ERRORS ... OFF)"

grep -qE '^[[:space:]]+(run: )?sudo xcode-select -s /Applications/Xcode_[0-9.]+\.app$' <<<"$job" ||
    fail "(d) build-and-test must select its Xcode by path (sudo xcode-select -s /Applications/Xcode_<v>.app)"

exit $failed
