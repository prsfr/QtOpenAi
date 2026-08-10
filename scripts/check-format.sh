#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The formatting check, in one place, so that a run before pushing and the run
# in CI ask the same question of the same set of files.
#
#   scripts/check-format.sh          # report violations, non-zero on any
#   scripts/check-format.sh --fix    # rewrite the files in place
#
# **Untracked files count, which is the whole reason this is a script.** The
# obvious spelling, `git ls-files '*.cpp' '*.h'`, lists *tracked* files only. So
# a branch that adds sources formats and checks everything except the code it is
# adding, and passes on the strength of files nobody touched. That is not
# hypothetical: it is exactly how #114 went red in CI after a clean local sweep,
# because the new files were still untracked when the sweep ran. Adding
# `--others --exclude-standard` closes it, and .gitignore is what keeps build
# trees and generated moc output from being swept up along with them.
#
# CI checks out a commit, where every file is tracked, so the two enumerations
# agree there -- the point is that they now also agree in a dirty working tree,
# which is the only place the difference could bite.

set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

# Overridable so a machine with several toolchains can name one; CI installs the
# distribution's default and leaves this alone.
clang_format=${CLANG_FORMAT:-clang-format}

mode=check
case ${1-} in
    --fix | -i) mode=fix ;;
    "") ;;
    *)
        echo "usage: $0 [--fix]" >&2
        exit 2
        ;;
esac

# -z throughout, so that a path containing a space stays one path. `--cached`
# can also name a file deleted in the working tree but not yet staged, which
# clang-format would fail to open, so each candidate is checked for existence.
files=()
while IFS= read -r -d '' file; do
    [[ -f $file ]] && files+=("$file")
done < <(git ls-files -z --cached --others --exclude-standard '*.cpp' '*.h')

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No C++ sources found -- nothing to check."
    exit 0
fi

if [[ $mode == fix ]]; then
    "$clang_format" -i "${files[@]}"
    echo "Formatted ${#files[@]} file(s)."
    exit 0
fi

# --Werror is what turns a violation into a non-zero exit. Note that this runs
# clang-format directly rather than through a pipe: a shell pipeline reports the
# *last* command's status, so `clang-format ... | head` exits 0 however badly
# clang-format failed. Reading that as success was the second half of how #114
# slipped past a local check.
"$clang_format" --dry-run --Werror "${files[@]}"
echo "${#files[@]} file(s) correctly formatted."
