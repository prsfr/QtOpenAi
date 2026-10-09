#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# QtOpenAi is a headless library: no module may depend on a GUI stack. The
# configure step already refuses a GUI module named in any target's link
# libraries; this is the other half -- what the linker actually recorded, so a
# GUI dependency arriving *transitively*, through a dependency of a dependency
# no CMakeLists mentions, is caught too. A headless library that turns out to
# need a display stack at runtime is otherwise discovered on the deployment,
# not in review.
#
# ELF only, so Linux only; skipped elsewhere and for a static build, where
# there is no NEEDED list to read.

set -euo pipefail

[[ $(uname -s) == Linux ]] || {
    echo "not Linux -- skipped"
    exit 77
}
command -v objdump >/dev/null || {
    echo "objdump not found -- skipped"
    exit 77
}

# The real files only: the .so.N symlinks would count every library twice.
libs=()
while IFS= read -r -d '' lib; do
    libs+=("$lib")
done < <(find "$QTOPENAI_BUILD_DIR/lib" -maxdepth 1 -type f -name 'libQtOpenAi*.so*' -print0 2>/dev/null)
if [[ ${#libs[@]} -eq 0 ]]; then
    echo "no shared QtOpenAi libraries in $QTOPENAI_BUILD_DIR/lib -- skipped"
    exit 77
fi

found=0
for lib in "${libs[@]}"; do
    offenders=$(objdump -p "$lib" | awk '/NEEDED/ {print $2}' |
        grep -E 'libQt6(Gui|Widgets|Quick|Qml|OpenGL|Charts|Svg|PrintSupport)' || true)
    if [[ -n $offenders ]]; then
        echo "::error::$(basename "$lib") needs $(echo $offenders | tr '\n' ' ')"
        found=1
    fi
done

if [[ $found -ne 0 ]]; then
    echo "QtOpenAi is a headless library; no module may depend on a GUI stack."
    exit 1
fi
echo "No QtOpenAi module depends on a GUI stack (${#libs[@]} libraries)."
