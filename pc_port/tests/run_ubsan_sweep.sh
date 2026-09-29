#!/usr/bin/env bash
# Sweep the pc_port differential harness and report, per suite, whether the
# UBSan regime ACTUALLY RAN.
#
# Run this inside the podman toolchain container.  UBSan does not work on a
# bare Fedora host here (no libubsan runtime); see
# docs/ai_context/HARNESS_REGIMES.md.
#
#   ./pc_port/tests/run_ubsan_sweep.sh [N]     # N = how many suites (default all)
#
# Outcomes are deliberately distinguishable, so "green" cannot absorb
# "never executed":
#
#   PASS         suite exited 0
#   UBSAN-NOT-RUN  suite exited 97 -- the sanitizer preflight refused
#   NO-UBSAN     suite has no UBSan regime at all (it is an O0/O2-only suite)
#   FAIL         suite exited nonzero for any other reason
#   TIMEOUT      suite exceeded the per-suite limit
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
LOGS="${UBSAN_SWEEP_LOGS:-pc_port/build_native/ubsweep}"
LIMIT="${2:-${UBSAN_SWEEP_TIMEOUT:-60}}"
N="${1:-0}"

# The toolchain image has no ripgrep, which the runners use purely as a line
# matcher.  Fall back to the documented grep shim rather than silently
# skipping every suite.
if ! command -v rg >/dev/null 2>&1; then
    PATH="$ROOT/pc_port/tests/lib/compat:$PATH"
    export PATH
    echo "note: ripgrep absent, using pc_port/tests/lib/compat/rg (grep shim)"
fi

rm -rf "$LOGS"; mkdir -p "$LOGS"
suites=(pc_port/tests/run_*.sh)
# do not recurse into ourselves
filtered=()
for s in "${suites[@]}"; do
    [ "$(basename "$s")" = "run_ubsan_sweep.sh" ] && continue
    filtered+=("$s")
done
[ "$N" -gt 0 ] && filtered=("${filtered[@]:0:$N}")

pass=0; fail=0; notrun=0; noubsan=0; timeout_n=0
: >"$LOGS/_report.tsv"
for s in "${filtered[@]}"; do
    b="$(basename "$s" .sh)"
    timeout "$LIMIT" bash "$s" >"$LOGS/$b.log" 2>&1
    rc=$?
    has_ubsan=no
    grep -q 'fsanitize=undefined' "$s" && has_ubsan=yes
    case "$rc" in
        0)   if [ "$has_ubsan" = yes ]; then st=PASS; pass=$((pass+1));
             else st=NO-UBSAN; noubsan=$((noubsan+1)); fi ;;
        97)  st=UBSAN-NOT-RUN; notrun=$((notrun+1)) ;;
        124) st=TIMEOUT; timeout_n=$((timeout_n+1)) ;;
        *)   st=FAIL; fail=$((fail+1)) ;;
    esac
    printf '%s\t%s\t%s\n' "$st" "$rc" "$b" >>"$LOGS/_report.tsv"
done

echo
echo "=== UBSan sweep ==="
echo "suites run           : ${#filtered[@]}"
echo "PASS (UBSan ran)     : $pass"
echo "NO-UBSAN regime      : $noubsan"
echo "UBSAN-NOT-RUN        : $notrun"
echo "FAIL                 : $fail"
echo "TIMEOUT              : $timeout_n"
echo
echo "=== UBSan runtime errors ==="
if grep -ho 'runtime error: .*' "$LOGS"/*.log 2>/dev/null | sort | uniq -c \
        | sort -rn | grep .; then
    echo "^ genuine UBSan findings, listed above"
else
    echo "none in the suites that executed"
fi

# The sweep itself is informational; a nonzero exit means something could not
# run, which is exactly the condition this script exists to surface.
[ "$notrun" -eq 0 ] && [ "$fail" -eq 0 ] && [ "$timeout_n" -eq 0 ]
