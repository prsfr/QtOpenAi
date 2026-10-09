#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The one entry point for every check a change has to pass, run the same way
# by a contributor, by the agent pipeline in AGENTS.md and by CI (#171).
#
#   scripts/check.sh                     # every check, in order
#   scripts/check.sh format test         # just these, still in order
#   scripts/check.sh --list              # what exists
#   scripts/check.sh --strict            # a skipped check fails (CI)
#   scripts/check.sh --build-dir build   # reuse an existing build tree
#
# Each check is one file, scripts/checks/NN-<name>.sh. NN fixes the order: a
# check may rely on the ones before it (`test` needs `build`), never on the
# ones after. A check exits 0 to pass, 77 to skip -- a tool it needs is not
# installed, or it does not apply on this platform -- and anything else to
# fail. Skipping is fine on a developer machine missing clang-format; it is not
# fine in CI, where a silently skipped check is a check that never ran, so CI
# passes --strict.
#
# CI calls this script rather than spelling a check out in ci.yml. That is the
# point of having it: the local run and the CI run ask the same question, and
# cannot drift apart the way the formatting sweep once did (#115).
#
# Adding a check: drop scripts/checks/NN-<name>.sh, executable, and make it
# deterministic -- the same tree gives the same verdict on every run and every
# machine. No wall-clock thresholds; compare against a committed baseline when
# the check measures something.

set -euo pipefail

root=$(git rev-parse --show-toplevel)
checks_dir="$root/scripts/checks"

usage() {
    sed -n '4,11p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

bad_usage() {
    echo "$1" >&2
    usage 2 >&2
}

strict=0
list=0
build_dir="$root/build-check"
selected=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --strict) strict=1 ;;
        --list) list=1 ;;
        --build-dir)
            [[ $# -ge 2 ]] || bad_usage "--build-dir needs a directory"
            build_dir=$2
            shift
            ;;
        -h | --help) usage 0 ;;
        -*) bad_usage "unknown option '$1'" ;;
        *) selected+=("$1") ;;
    esac
    shift
done

# Absolute, so a check that changes directory still finds it.
case $build_dir in
    /*) ;;
    *) build_dir="$PWD/$build_dir" ;;
esac

declare -a names files
for file in "$checks_dir"/[0-9][0-9]-*.sh; do
    name=$(basename "$file" .sh)
    names+=("${name#[0-9][0-9]-}")
    files+=("$file")
done

if [[ $list -eq 1 ]]; then
    printf '%s\n' "${names[@]}"
    exit 0
fi

for want in ${selected[@]+"${selected[@]}"}; do
    found=0
    for name in "${names[@]}"; do
        [[ $name == "$want" ]] && found=1
    done
    if [[ $found -eq 0 ]]; then
        echo "unknown check '$want' (scripts/check.sh --list)" >&2
        exit 2
    fi
done

export QTOPENAI_ROOT=$root
export QTOPENAI_BUILD_DIR=$build_dir

summary=()
failed=0
for i in "${!names[@]}"; do
    name=${names[$i]}
    if [[ ${#selected[@]} -gt 0 ]]; then
        wanted=0
        for want in ${selected[@]+"${selected[@]}"}; do
            [[ $want == "$name" ]] && wanted=1
        done
        [[ $wanted -eq 1 ]] || continue
    fi

    echo "==> $name"
    status=0
    bash "${files[$i]}" || status=$?
    case $status in
        0) summary+=("PASS  $name") ;;
        77)
            if [[ $strict -eq 1 ]]; then
                summary+=("FAIL  $name (skipped under --strict)")
                failed=1
            else
                summary+=("SKIP  $name")
            fi
            ;;
        *)
            summary+=("FAIL  $name (exit $status)")
            failed=1
            ;;
    esac
done

echo
printf '%s\n' "${summary[@]}"
exit $failed
