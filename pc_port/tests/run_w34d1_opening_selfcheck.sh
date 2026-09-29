#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HELPER="$ROOT/pc_port/tools/n5_opening_harness"

# Keep the published direct-run interface intact.  A checkout that loses
# executable bits must fail here, before a runner reaches the live binary.
for executable in \
    "$ROOT/pc_port/tests/run_w34d1_opening_a.sh" \
    "$ROOT/pc_port/tests/run_w34d1_opening_b.sh" \
    "$HELPER/schedule.py" \
    "$HELPER/analyze_log.py"
do
    if [ ! -x "$executable" ]; then
        echo "W34D1 N5 OPENING HARNESS SELFCHECK FAIL: not executable: $executable" >&2
        exit 1
    fi
done

# Exercise the shebang-backed direct invocation as well as the Python entry
# point used by the tests.  The schedule output itself is intentionally not
# retained; this is only a packaging/interface check.
"$HELPER/schedule.py" --self-test
python3 "$HELPER/schedule.py" --self-test

python3 -m pytest "$HELPER/test_n5_opening.py" -q

echo "W34D1 N5 OPENING HARNESS SELFCHECK PASS"
