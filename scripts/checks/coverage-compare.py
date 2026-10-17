#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# The comparator behind scripts/checks/80-coverage.sh (#173): gcovr's JSON
# and the committed baseline in, a per-module table, a verdict and the
# candidate baseline files out.
#
# Only integer counts are stored. Percentages are derived from them as
# fractions and the thresholds are compared exactly, so a module sitting on
# a boundary gets the same verdict on every run and every interpreter.
#
# Exit status: 0 pass, 1 fail, 77 the toolchain differs from the baseline's.

import argparse
import json
import os
import sys
from fractions import Fraction

FAIL_DROP = Fraction(1, 2)  # pt below the baseline that fails
WARN_RISE = Fraction(1)  # pt above the baseline that asks for an update


def measure(coverage):
    """Per-module [covered, total] lines and branches, the uncalled functions
    and every function, from gcovr's JSON (format 0.6, gcovr 7.0)."""
    modules = {}
    called = {}  # "path\tname" -> whether it ran
    for entry in coverage["files"]:
        path = entry["file"]
        parts = path.split("/")
        if len(parts) < 3 or parts[0] != "src":
            continue
        counts = modules.setdefault(parts[1], {"lines": [0, 0], "branches": [0, 0]})
        for line in entry["lines"]:
            if line.get("gcovr/excluded"):
                continue
            counts["lines"][1] += 1
            if line["count"] > 0:
                counts["lines"][0] += 1
            for branch in line["branches"]:
                counts["branches"][1] += 1
                if branch["count"] > 0:
                    counts["branches"][0] += 1
        for function in entry["functions"]:
            if function.get("gcovr/excluded"):
                continue
            # One entry per line it was seen at: it ran if any of them did.
            key = path + "\t" + function["name"]
            called[key] = called.get(key, False) or function["execution_count"] > 0
    uncalled = set(key for key, ran in called.items() if not ran)
    return modules, uncalled, set(called)


def percent(pair):
    covered, total = pair
    return Fraction(100) if total == 0 else Fraction(100 * covered, total)


def fixed(value, sign=False):
    """A Fraction with two decimals, rounded exactly (half to even)."""
    hundredths = round(value * 100)
    text = "%s%d.%02d" % ("-" if hundredths < 0 else "", abs(hundredths) // 100, abs(hundredths) % 100)
    if sign and hundredths >= 0:
        text = "+" + text
    return text


def direction(new, old):
    a, b = percent(new), percent(old)
    return "up" if a > b else "down" if a < b else "same"


def table(modules, base):
    header = ["module", "lines", "%", "delta", "branches", "%", "delta"]
    rows = []
    total = {"lines": [0, 0], "branches": [0, 0]}
    base_total = {"lines": [0, 0], "branches": [0, 0]}
    for name in sorted(modules):
        row = [name]
        for kind in ("lines", "branches"):
            pair = modules[name][kind]
            total[kind] = [total[kind][0] + pair[0], total[kind][1] + pair[1]]
            row += ["%d/%d" % tuple(pair), fixed(percent(pair))]
            if name in base:
                old = base[name][kind]
                base_total[kind] = [base_total[kind][0] + old[0], base_total[kind][1] + old[1]]
                row.append(fixed(percent(pair) - percent(old), sign=True))
            else:
                row.append("new")
        rows.append(row)
    row = ["total"]
    for kind in ("lines", "branches"):
        row += ["%d/%d" % tuple(total[kind]), fixed(percent(total[kind]))]
        row.append(fixed(percent(total[kind]) - percent(base_total[kind]), sign=True) if base else "n/a")
    rows.append(row)
    return header, rows


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write(text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("coverage_json")
    parser.add_argument("--baseline-dir", required=True)
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--gcovr", required=True)
    parser.add_argument("--qt", required=True)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()

    with open(args.coverage_json, encoding="utf-8") as file:
        modules, uncalled, functions = measure(json.load(file))
    toolchain = {"compiler": args.compiler, "gcovr": args.gcovr, "qt": args.qt}

    baseline_json = os.path.join(args.baseline_dir, "baseline.json")
    uncalled_txt = os.path.join(args.baseline_dir, "uncalled-functions.txt")
    baseline = None
    listed = set()
    if os.path.exists(baseline_json):
        with open(baseline_json, encoding="utf-8") as file:
            baseline = json.load(file)
        if os.path.exists(uncalled_txt):
            with open(uncalled_txt, encoding="utf-8") as file:
                listed = set(line for line in file.read().split("\n") if line)
    base = baseline["modules"] if baseline else {}

    # The candidates: what the baseline files would be after this run.
    candidate_json = json.dumps({"toolchain": toolchain, "modules": modules},
                                sort_keys=True, indent=2) + "\n"
    candidate_txt = "".join(line + "\n" for line in sorted(uncalled))
    write(os.path.join(args.out_dir, "baseline.json"), candidate_json)
    write(os.path.join(args.out_dir, "uncalled-functions.txt"), candidate_txt)

    header, rows = table(modules, base)
    widths = [max(len(r[i]) for r in [header] + rows) for i in range(len(header))]
    for row in [header] + rows:
        print("  ".join(c.ljust(w) if i == 0 else c.rjust(w) for i, (c, w) in enumerate(zip(row, widths))))
    print("uncalled functions: %d of %d" % (len(uncalled), len(functions)))

    failures, warnings, notes = [], [], []
    drift = []
    if baseline:
        for key in sorted(toolchain):
            old = baseline.get("toolchain", {}).get(key)
            if toolchain[key] != old:
                drift.append("toolchain %s %s differs from the baseline's %s -- skipped"
                             % (key, toolchain[key], old))

    if args.update:
        if drift:
            failures = [d.replace(" -- skipped", " -- not updating the baseline") for d in drift]
            status = 1
        else:
            for name in sorted(set(modules) | set(base)):
                if name not in base:
                    notes.append("%s: new module" % name)
                elif name not in modules:
                    notes.append("%s: removed" % name)
                else:
                    notes.append("%s: lines %s, branches %s" % (
                        name, direction(modules[name]["lines"], base[name]["lines"]),
                        direction(modules[name]["branches"], base[name]["branches"])))
            write(baseline_json, candidate_json)
            write(uncalled_txt, candidate_txt)
            notes.append("baseline updated in %s; a 'down' needs its reason in the commit "
                         "(AGENTS.md, rule 8)" % args.baseline_dir)
            status = 0
    elif drift:
        notes = drift
        status = 77
    elif baseline is None:
        failures.append("no baseline at %s (QTOPENAI_COVERAGE_UPDATE_BASELINE=1 writes one)"
                        % baseline_json)
        status = 1
    else:
        for name in sorted(set(modules) - set(base)):
            failures.append("module %s is measured but not in the baseline" % name)
        for name in sorted(set(base) - set(modules)):
            failures.append("module %s is in the baseline but was not measured" % name)
        for name in sorted(set(modules) & set(base)):
            for kind in ("lines", "branches"):
                delta = percent(modules[name][kind]) - percent(base[name][kind])
                if delta < -FAIL_DROP:
                    failures.append("%s %s coverage dropped %s pt below the baseline (more than 0.5)"
                                    % (name, kind, fixed(-delta)))
                elif delta > WARN_RISE:
                    warnings.append("%s %s coverage is %s pt above the baseline; raise it with "
                                    "QTOPENAI_COVERAGE_UPDATE_BASELINE=1" % (name, kind, fixed(delta)))
        for key in sorted(uncalled - listed):
            failures.append("uncalled function not in uncalled-functions.txt: %s" % key)
        for key in sorted(listed - uncalled):
            reason = "now called" if key in functions else "no longer exists"
            failures.append("stale entry in uncalled-functions.txt (%s): %s" % (reason, key))
        status = 1 if failures else 0

    lines = ["FAIL: " + f for f in failures] + ["WARNING: " + w for w in warnings] + notes
    for line in lines:
        print(line)

    md = ["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
    md += ["| " + " | ".join(row) + " |" for row in rows]
    md.append("")
    md.append("Uncalled functions: %d of %d" % (len(uncalled), len(functions)))
    if lines:
        md += [""] + ["- " + line for line in lines]
    write(os.path.join(args.out_dir, "coverage.md"), "## Coverage\n\n" + "\n".join(md) + "\n")
    return status


if __name__ == "__main__":
    sys.exit(main())
