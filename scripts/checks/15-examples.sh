#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The examples share their plumbing through examples/common/ExampleSupport.h
# (#175). Two ways it gets copied back in, both of which happened before:
#
#   - an example reads OPENAI_API_KEY / OPENAI_ADMIN_KEY / OPENAI_BASE_URL /
#     OPENAI_MODEL itself instead of through the header;
#   - an error handler ends with quit(), which exits 0 and tells a script that
#     a failed request succeeded. Seventeen did, most cloned from one another.
#
# Plain grep over the sources: no build, the same answer on every machine.

set -euo pipefail

cd "$QTOPENAI_ROOT/examples"
failed=0

# grep exits 1 for "no match"; anything above that (an unreadable file) must
# fail the check rather than pass it.
literals=$(grep -rnE '"OPENAI_(API_KEY|ADMIN_KEY|BASE_URL|MODEL)"' --include='*.cpp' \
    --include='*.h' --exclude=ExampleSupport.h .) || [[ $? -eq 1 ]] || exit 2
if [[ -n $literals ]]; then
    echo "examples read the environment themselves; use exampleEnv() from common/ExampleSupport.h:"
    LC_ALL=C sort <<<"$literals"
    failed=1
fi

# The window is lexical: a success handler that quits within four lines of an
# error handler is flagged too. Move it, rather than widening this.
context=$(grep -rn -A4 'error\.message()' --include='*.cpp' --include='*.h' .) ||
    [[ $? -eq 1 ]] || exit 2
quits=$(grep -E 'app\.quit\(\)|QCoreApplication::quit\(\)|qApp->quit\(\)' <<<"$context" || true)
if [[ -n $quits ]]; then
    echo "an error handler quits with exit status 0; use failAndExit() or exit(1):"
    LC_ALL=C sort <<<"$quits"
    failed=1
fi

exit $failed
