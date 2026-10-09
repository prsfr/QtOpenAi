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
scripts/check.sh                  # every check, in order
scripts/check.sh format test      # a subset
scripts/check.sh --list           # what exists
scripts/check.sh --strict         # a skipped check fails -- what CI runs
```

Each check is one file in `scripts/checks/`, numbered for order. CI calls the
script instead of spelling checks out in `ci.yml`, so a green local run and a
green CI run mean the same thing. Every check is **deterministic**: the same
tree gives the same verdict on every run and every machine. Where a check
measures something (coverage, duplication, instruction counts), it compares
against a baseline committed next to it, and the baseline only ever moves in
the good direction.

Run it before every push. Run the full set, not the check you think is
relevant.

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

- A FAIL at stage 4, 5 or 6 goes back to stage 3. After the fix, the failing
  stage and **every stage after it** run again; earlier PASS verdicts stand.
- A fix that needs a file outside the footprint goes back to stage 1, and the
  pipeline runs on from there. There is no other way to widen a footprint.
- A finding is closed by a code change, or by a one-line reason it does not
  apply that the stage that raised it accepts. It is never closed by silence.
- Three FAILs in a row at the same stage for the same finding: stop and ask
  the person who owns the issue. The spec is probably wrong.
- The spec may change during the work — by going back to stage 1, never by
  drifting in stage 3.

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
Touches:      modules and their dependency edges; must not add an edge
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
