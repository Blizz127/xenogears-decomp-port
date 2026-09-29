#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/build-psycross-failure.XXXXXX")
echo "OUTPUT $OUT"
# Run the actual dependency-build command/guard, not the rest of the driver.
awk '/^cmake --build/ {print; exit} /^if ! cmake --build/ {body=1} body {print} body && /^fi$/ {exit}' pc_port/build_port.sh > "$OUT/build.inc"
test -s "$OUT/build.inc"
set +e
(
    set +e
    PSYX_BUILD="$OUT/never-built"
    cmake() { return 73; }
    source "$OUT/build.inc"
    echo 'FAIL: stale dependency accepted'
    exit 0
) > "$OUT/result.log" 2>&1
result=$?
set -e
test "$result" -ne 0
! grep -q 'stale dependency accepted' "$OUT/result.log"
grep -q 'PsyCross build failed' "$OUT/result.log"
echo 'PASS dependency failure stops before stale library can be linked'
