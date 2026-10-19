#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Copy-paste in src/, tests/ and examples/, measured by jscpd (pinned in
# scripts/checks/package.json) with the settings in .jscpd.json, each directory
# on its own, against scripts/checks/duplication-baseline.json (#182). A
# directory fails when its clone count or its duplicated lines exceed the
# baseline, and the check then lists every clone pair in it.
#
# jscpd merges overlapping clones in the order it reads the files and does not
# sort what it finds, so the check hands it the list itself: git's files (an
# untracked, non-ignored one counts, as in check-format.sh), in byte order.
#
# QTOPENAI_DUPLICATION_UPDATE_BASELINE=1 writes the measurement into the
# baseline -- only when no number went up (AGENTS.md, rule 8).
#
# Needs node and npm; npm ci installs the pinned jscpd into
# scripts/checks/node_modules on the first run. Without them, or without the
# registry for that first install, the check skips -- a FAIL under --strict.

set -euo pipefail

command -v node >/dev/null || {
    echo "node not found -- skipped"
    exit 77
}
command -v npm >/dev/null || {
    echo "npm not found -- skipped"
    exit 77
}

# From the root: jscpd reads .jscpd.json and reports paths relative to it.
cd "$QTOPENAI_ROOT"
checks=scripts/checks

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# npm's output is shown only when it fails, so a run that installs prints the
# same as one that does not.
state=$(node -e '
    const fs = require("fs");
    const read = (file) => JSON.parse(fs.readFileSync(file, "utf8"));
    const pinned = read(process.argv[1]).dependencies.jscpd;
    let installed = "";
    try { installed = read(process.argv[2]).version; } catch (e) {}
    console.log(installed === pinned ? "installed" : "missing");
' "$checks/package.json" "$checks/node_modules/jscpd/package.json")
if [[ $state != installed ]]; then
    npm ci --prefix "$checks" --no-audit --no-fund >"$tmp/npm.log" 2>&1 || {
        cat "$tmp/npm.log"
        echo "npm ci --prefix $checks failed -- skipped"
        exit 77
    }
fi

for dir in examples src tests; do
    files=()
    while IFS= read -r -d '' file; do
        [[ $file == tests/package/* ]] && continue
        [[ -f $file ]] && files+=("$file")
    done < <(git ls-files -z --cached --others --exclude-standard -- "$dir/*.cpp" "$dir/*.h" |
        LC_ALL=C sort -z)
    # An empty list means git failed (the tree always has sources), and jscpd
    # given no path would scan the working directory instead.
    if [[ ${#files[@]} -eq 0 ]]; then
        echo "no .cpp/.h files found under $dir/ -- git ls-files failed?" >&2
        exit 1
    fi
    status=0
    node "$checks/node_modules/jscpd/bin/jscpd" --config .jscpd.json --output "$tmp/$dir" \
        "${files[@]}" >"$tmp/$dir.log" 2>&1 || status=$?
    if [[ $status -ne 0 || ! -f $tmp/$dir/jscpd-report.json ]]; then
        cat "$tmp/$dir.log" >&2
        echo "jscpd failed on $dir/ (exit $status) or wrote no report" >&2
        exit 1
    fi
done

update=0
[[ ${QTOPENAI_DUPLICATION_UPDATE_BASELINE:-} == 1 ]] && update=1
node - "$checks/duplication-baseline.json" "$tmp" "$update" <<'EOF'
const fs = require("fs");
const path = require("path");
const [baselinePath, tmp, update] = process.argv.slice(2);
const dirs = ["examples", "src", "tests"];
const kinds = [["clones", "clones"], ["duplicatedLines", "duplicated lines"]];
const command = "QTOPENAI_DUPLICATION_UPDATE_BASELINE=1 scripts/check.sh duplication";

function broken(message) {
    console.error(message);
    process.exit(1);
}

function count(value) {
    return Number.isInteger(value) && value >= 0;
}

let baseline;
try {
    baseline = JSON.parse(fs.readFileSync(baselinePath, "utf8"));
} catch (e) {
    broken(`cannot read ${baselinePath}: ${e.message}`);
}
const keys = (o) => (o && typeof o === "object" && !Array.isArray(o) ? Object.keys(o).sort() : null);
if (String(keys(baseline)) !== String(dirs))
    broken(`${baselinePath} must hold exactly ${dirs.join(", ")}`);
for (const dir of dirs) {
    if (String(keys(baseline[dir])) !== "clones,duplicatedLines")
        broken(`${baselinePath}: ${dir} must hold exactly clones and duplicatedLines`);
    for (const [kind] of kinds)
        if (!count(baseline[dir][kind]))
            broken(`${baselinePath}: ${dir}.${kind} is not a non-negative integer`);
}

const measured = {};
const pairs = {};
for (const dir of dirs) {
    let report;
    try {
        report = JSON.parse(fs.readFileSync(path.join(tmp, dir, "jscpd-report.json"), "utf8"));
    } catch (e) {
        broken(`cannot read jscpd's report for ${dir}/: ${e.message}`);
    }
    const total = report.statistics && report.statistics.total;
    if (!total || !count(total.clones) || !count(total.duplicatedLines) || !Array.isArray(report.duplicates))
        broken(`jscpd's report for ${dir}/ has no clone totals`);
    measured[dir] = { clones: total.clones, duplicatedLines: total.duplicatedLines };
    // jscpd names the later occurrence first; the earlier one leads here.
    pairs[dir] = report.duplicates
        .map((d) => [d.firstFile, d.secondFile].map((f) => ({ name: f.name.replace(/\\/g, "/"), start: f.start, end: f.end })))
        .map(([a, b]) => (a.name < b.name || (a.name === b.name && a.start <= b.start) ? [a, b] : [b, a]))
        .map((ends) => ends.map((f) => `${f.name}:${f.start}-${f.end}`).join(" ≡ "))
        .sort();
}

const over = [];
const lower = [];
for (const dir of dirs) {
    const m = measured[dir];
    const b = baseline[dir];
    console.log(`${dir.padEnd(8)}  clones ${m.clones} (baseline ${b.clones}),` +
        ` duplicated lines ${m.duplicatedLines} (baseline ${b.duplicatedLines})`);
    const up = kinds.filter(([k]) => m[k] > b[k]).map(([k, label]) => `${label} ${m[k]} > ${b[k]}`);
    const down = kinds.filter(([k]) => m[k] < b[k]).map(([k, label]) => `${label} ${m[k]} < ${b[k]}`);
    if (up.length) over.push([dir, up]);
    else if (down.length) lower.push([dir, down]);
}

if (over.length) {
    console.log();
    for (const [dir, up] of over)
        console.log(`${dir}: ${up.join(", ")}`);
    for (const [dir] of over) {
        console.log();
        console.log(`clone pairs in ${dir}/ -- remove the new clone, or say in review why it is a` +
            ` deliberate case (AGENTS.md, "Duplication"):`);
        for (const pair of pairs[dir])
            console.log(pair);
    }
    if (update === "1")
        console.log("\nnot updating the baseline: it never goes up (AGENTS.md, rule 8)");
    process.exit(1);
}

if (update === "1") {
    const next = {};
    for (const dir of dirs) next[dir] = measured[dir];
    fs.writeFileSync(baselinePath, JSON.stringify(next, null, 2) + "\n");
    console.log();
    for (const dir of dirs) {
        const b = baseline[dir];
        const m = measured[dir];
        console.log(`${dir}: clones ${b.clones} → ${m.clones},` +
            ` duplicated lines ${b.duplicatedLines} → ${m.duplicatedLines}`);
    }
    console.log(`baseline updated in ${baselinePath}`);
    process.exit(0);
}

for (const [dir, down] of lower)
    console.log(`${dir}: ${down.join(", ")} -- lower the baseline: ${command}`);
EOF
