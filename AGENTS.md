# AGENTS.md

How a change gets into QtOpenAi. It applies to every contributor — human, a
coding agent, or an agent driving subagents — and it is the same every time,
so that the result does not depend on who made the change or how careful they
were that day.

Two ideas carry it:

- **One pipeline.** Every change goes through the same six stages, in the same
  order, and each stage ends in a written verdict. Nothing is "probably fine".
- **No sprawl.** The library is feature-complete for 1.0. A change makes the
  code base *do* what its issue asks and otherwise leaves it no bigger, no more
  coupled and no less tested than it found it. The checks in
  `scripts/check.sh` enforce what can be measured; the Code Reviewer enforces
  the rest.

## The check script

```sh
scripts/check.sh                  # every default check, in order
scripts/check.sh format test      # a subset
scripts/check.sh --list           # what exists
scripts/check.sh --strict         # a skipped check fails -- what CI runs
```

Each check is one file in `scripts/checks/`, numbered for order. CI calls the
script instead of spelling checks out in `ci.yml`, so a green local run and a
green CI run mean the same thing. Every check is **deterministic**: the same
tree gives the same verdict on every run and every machine. Where a check
measures something (coverage, duplication, instruction counts), it compares
against a baseline committed in the repository (coverage's is in
`tests/coverage/`, duplication's is
`scripts/checks/duplication-baseline.json`), and the baseline only ever moves
in the good direction.

Run it before every push. Run the full set, not the check you think is
relevant.

The warning gate (`-Werror`) runs in CI on pinned runners; `cmake
-DQTOPENAI_WARNINGS_AS_ERRORS=ON <tree>` reproduces it locally. That keeps the
"same verdict on every machine" rule: locally warnings never fail a check, and
the compiler-dependent gate runs only on CI's pinned compilers.

A check too costly for every run (`static`, `sanitize`, `coverage`: each a
second full build) is marked `explicit-only` in its file: it runs only when
named (`scripts/check.sh static`, `scripts/check.sh coverage`), and CI always
names it. A run that names no checks prints it as
`SKIP ... (explicit only; ...)`; that line is not a failure and does not block
a stage, `--strict` or not. Run it by name when a change could behave
differently in a static build, and `coverage` whenever a change adds or
removes code in `src/`.

### Coverage

`scripts/check.sh coverage` builds `<build-dir>-coverage` with
`QTOPENAI_COVERAGE=ON`, runs the tests from zeroed counters and has gcovr
measure `src/`. It prints lines and branches per module (the directory under
`src/`) with the change against the baseline, and fails when:

1. a module's line or branch coverage is more than 0.5 points below its
   baseline;
2. a function in `src/` is never called and not listed in
   `tests/coverage/uncalled-functions.txt`;
3. a listed function is called now, or no longer exists (a stale entry);
4. a module is measured but not in the baseline, or the other way round;
5. a module's measured line or branch total is 0 while its baseline total is
   not.

Rule 2 sees only the functions the compiler emits: an unused inline function
in a header, or a template nothing instantiates, is invisible to it, so the
reviewer checks by eye that such code has a test. The check ignores
`LCOV_EXCL_*`/`GCOVR_EXCL_*` exclusion markers and any `gcovr.cfg`, and runs
the tests without the caller's Qt logging configuration, so none of them can
change the measurement.

A module more than 1.0 point above its baseline passes with a WARNING asking
for the baseline to be raised.

The baseline is two files: `tests/coverage/baseline.json` (covered and total
lines and branches per module, and the toolchain) and
`tests/coverage/uncalled-functions.txt` (file and function name, one per
line). It holds for one compiler, gcovr and Qt -- CI's coverage job, the
ubuntu-24.04 compiler with the distro gcovr and Qt. Another toolchain still
measures and prints the table, then reports a SKIP; under `--strict`, as in
CI, that is a failure. The coverage tree must sit inside the source tree, or
gcov cannot read what moc generated and the check fails. After deleting or
renaming a source file, remove `<build-dir>-coverage`: Ninja leaves the old
objects behind and gcov fails on them.

`QTOPENAI_COVERAGE_UPDATE_BASELINE=1 scripts/check.sh coverage` writes the
measurement into both files and prints whether each module went up, down or
stayed the same. It refuses on a toolchain other than the baseline's.
Baselines only improve (rule 8): a "down" line is not allowed unless the
commit says why.

A change that adds a function adds a test that calls it, or says in the pull
request why the function joins `uncalled-functions.txt`.

### Duplication

`scripts/check.sh duplication` runs jscpd (pinned in
`scripts/checks/package.json`, settings in `.jscpd.json`) over `src/`,
`tests/` and `examples/`, each on its own, and compares each directory's clone
count and duplicated lines with `scripts/checks/duplication-baseline.json`. It
fails when either number exceeds the baseline, and lists that directory's clone
pairs as `file:start-end ≡ file:start-end`. It needs node and npm, and the
registry for the first `npm ci`; without them it skips, a FAIL under
`--strict`.

The files measured are git's: untracked, non-ignored `.cpp`/`.h` files under
the three directories count locally, so a scratch file can fail a local run.
jscpd merges overlapping clones in the order it reads files, so renaming or
adding a file that takes part in a clone can move a directory's totals by a few
lines with no new clone. Such a rise is explained in review, or the clone is
refactored; the baseline is never raised.

When a directory goes below its baseline, the check passes and prints
`QTOPENAI_DUPLICATION_UPDATE_BASELINE=1 scripts/check.sh duplication`, which
writes the measurement into the baseline. It refuses, writing nothing, when any
number went up (rule 8).

A pull request that adds a clone removes it, or explains in review why it is one
of the deliberate cases below. A deliberate clone is listed here, never marked
in the source: a `jscpd:ignore` marker in a measured file fails the check. So
does a listed file of six or more lines that jscpd leaves unmeasured (no code or
comment tokens, e.g. a header of only `#include` lines without the SPDX
comment). jscpd skips preprocessor lines, so a copied multi-line `#define` is
invisible to it and is caught in review only. The cases looked at in #182 and
left as they are:

- The out-of-line special members of the implicitly shared value types: the
  compiler checks every line against its declaration; public-header macros
  were declined in #162.
- The per-field getter/setter/`toJson`/`fromJson`/`operator==` mirroring: a
  generator would make the most-read code the hardest to read, and the
  key-symmetry risk is covered by #180.
- The request/resource field blocks (`Create*Request` vs. resource,
  `ImageEditRequest`/`ImageVariationRequest`, …): flat value types, #142.
- The `TypedReply` subclass residue (`friend class Client;`,
  `using TypedReply::TypedReply;`): #162.
- The encode/decode loops of `ListPage`/`CursorPage`/`BucketPage`: installed
  templates that cannot use the private helpers.
- `SqliteStore.h` ≡ `JsonFileStore.h` and `ResponseCache.h` ≡
  `PersistentResponseCache.h`: each the override list of a shared interface.
- The `Client.cpp` endpoint wrappers over `get`/`post`/`remove`/
  `postMultipart`: #162.
- `LoggingInterceptor.cpp`: the body-excerpt tail of
  `beforeRequest()`/`afterResponse()`; fold it into a private helper when that
  file is next touched.
- README snippets that repeat a header or example: they are meant to read
  stand-alone.
- The internal repeats of `tests/storage/tst_store.cpp` and
  `tests/*/tst_pagination.cpp`: already data-driven; the rest are per-scenario
  arrange blocks.

### Modules

`cmake/module-deps.txt` is the declared module graph: one line per edge,
`<Module> <Dependency> <public|private>`, where a dependency is another module
or a Qt target (`Qt6::Network`). `scripts/check.sh modules` fails when

1. the table is malformed, names an unknown module, leaves a module out, or
   has a cycle;
2. an `#include` in `src/<module>/include/` or `src/<module>/src/` reaches a
   module or Qt module the table does not give that module -- a `public` edge
   for an installed header. A `src/common/` header counts as written in the
   file that includes it. Relative, macro and slash-less Qt includes
   (`<QNetworkReply>`) are refused;
3. an installed header includes a private header, or the install export file
   names a path in the source tree;
4. a target's link lists and the table differ, either way: public is a
   `PUBLIC` link, private a `PRIVATE` one. The lists are read from a
   configure-only tree, `<build-dir>-modules`, rebuilt on every run.

Without cmake, on Windows, or with a Qt that lacks WebSockets or Sql, it runs
the rules it can and skips, a FAIL under `--strict`.

Adding or widening an edge is a deliberate edit of that file, reviewed like an
API change (rule 4): it is in the diff, with its reason. The PR that removes
or narrows an edge a line's comment names (`# to be removed by #<n>`) edits
that line together with the CMake.

## The pipeline

One issue → one `git worktree` → one branch → one pull request.

```sh
git fetch origin main
git worktree add -b issue-<n>-<slug> ../wt/issue-<n> origin/main
```

| # | Stage | Done by | Produces | Gate |
|---|---|---|---|---|
| 1 | Specify | `specifier` subagent | the spec | every acceptance criterion is testable |
| 2 | Analyze | `analyzer` subagent | the impact map | footprint and reuse list confirmed |
| 3 | Implement | the main agent / author | tests first, then code | `scripts/check.sh` green |
| 4 | Review | `code-reviewer` subagent | findings | PASS |
| 5 | QA | `qa` subagent | criterion → test table | PASS |
| 6 | Harden | `hardener` subagent | adversarial findings + tests | PASS |

The subagent definitions live in `.claude/agents/`. Their prompts and this
file say the same thing; if they disagree, this file wins and the prompt is a
bug.

### Loop rules

- A FAIL at stage 4, 5 or 6 goes back to stage 3, and so does a fix for a
  minor finding. If the fix changes only test sources and their
  `tests/**/CMakeLists.txt` entries (rule 1), the stage that raised the
  finding and **every stage after it** run again. If it changes any
  other file — library code, CMake, a script, CI, docs, a baseline — stage 4
  and **every stage after it** run again. Earlier PASS verdicts stand: those of
  the stages before the first one that runs again.
- A fix that needs a file outside the footprint goes back to stage 1, and the
  pipeline runs on from there. There is no other way to widen a footprint.
- A finding is closed by a file change, or by a one-line reason it does not
  apply that the stage that raised it accepts. It is never closed by silence.
  A reason changes no file: only the stage that raised the finding runs again,
  to accept it; no other stage does. Any file changed in the same fix still
  reruns stages as the first bullet says.
- Three FAILs in a row at the same stage for the same finding: stop and ask
  the person who owns the issue. The spec is probably wrong.
- The spec may change during the work — by going back to stage 1, never by
  drifting in stage 3. Every return to stage 1, whether or not it widens the
  footprint, means stages 2 to 6 run again.

### Stage outputs

Each stage writes its output into `.work/<issue>/` in the worktree (ignored by
git), so the next stage reads a file instead of a recollection. The spec and
the three verdicts go into the pull request description.

**1 — Spec** (`spec.md`)

```
Issue:        #<n> <title>
Open questions: only if the issue is ambiguous -- the pipeline stops here
Problem:      what is wrong or missing, with file:line
Acceptance:   A1 … An — each one observable by a test or a check
Footprint:    files that may change; new files, if any, each with a reason
API delta:    public symbols added / changed / removed (usually: none)
Non-goals:    what this change deliberately does not do
Test plan:    which test proves which criterion; the failing test comes first
```

**2 — Impact map** (`impact.md`)

```
Reuse:        existing helpers/types to use instead of writing new ones
Touches:      modules and their edges in cmake/module-deps.txt; must not add one
Duplicates:   copies of the logic being changed elsewhere (change all or none)
Baseline:     scripts/check.sh result before the change
Risks:        what could break that the tests would not notice
```

**4, 5, 6 — Verdicts** (`review.md`, `qa.md`, `harden.md`)

```
Verdict:  PASS | FAIL
Findings: F1 [blocking|minor] file:line — what is wrong — what to do
```

A verdict with a blocking finding is FAIL. Minor findings are fixed in the
same change or answered with a reason; they do not block.

## No sprawl — the rules the Code Reviewer enforces

1. **Footprint.** No change outside the files the spec lists. Needing another
   file means going back to stage 1. Tests are the exception: test sources
   and their `tests/**/CMakeLists.txt` entries added by stage 3 or stage 6 to
   prove a criterion or a finding are always inside the footprint.
2. **Public API.** No public symbol is added, changed or removed unless the
   spec's API delta names it. After 1.0 the ABI is frozen; before it, every
   symbol added is one more to freeze.
3. **Reuse first.** Before adding a helper, type or file, use what the impact
   map lists. A new abstraction needs at least two users in the same diff.
4. **No new edges.** No new module, third-party dependency, or dependency
   between modules unless the issue asks for it. The library stays headless.
5. **Tests prove criteria.** Each acceptance criterion has a test that failed
   before the change and passes after it. Tests use the offline stub servers
   in `tests/support`; no network, no sleeps, no wall-clock assertions.
6. **Delete as you go.** Code the change makes dead is removed in the same
   change. Dead code nobody removes is the first form sprawl takes.
7. **Comments explain why.** A comment states a reason the code cannot; it
   does not narrate the diff. Match the density of the surrounding code.
8. **Baselines only improve.** A check's baseline file may move only in the
   good direction, and only in a commit that says why.

## Conventions

- C++17, Qt 6, `QtOpenAi::` namespace, implicitly shared value types with a
  d-pointer, `Q_DECLARE_PRIVATE` for QObjects, no `get` prefix, a per-module
  export macro.
- Formatting is `.clang-format`; `scripts/check-format.sh --fix` applies it.
- Commit messages explain the *why* and name the issue (`(#<n>)`); the pull
  request description carries the spec and the verdicts.
- Decisions on record are not reopened by a change: `Client`/`Organization`
  stay whole (#123); there is no QML module (#46).
