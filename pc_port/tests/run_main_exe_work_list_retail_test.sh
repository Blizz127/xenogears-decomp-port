#!/usr/bin/env bash
# Retail differential for the hand-written XENO_PC_PORT arms in
# pc_port/src/work_list_port.c vs disc/SLUS_006.64 on the port MIPS
# interpreter (test oracle only).  See main_exe_work_list_retail_test.c.
# Regimes O0 / O2 / UBSan (shift-base on), then one mutant per body.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
source pc_port/tests/lib/slus_oracle_lib.sh
OUT=${MAIN_EXE_WORK_LIST_OUT:-pc_port/build_native/main_exe_work_list}
mkdir -p "$OUT"
slus_check_disc
SRC=${MAIN_EXE_WORK_LIST_SOURCE:-pc_port/src/work_list_port.c}
WEAK=(none)
for mode in ${MAIN_EXE_WORK_LIST_MODES:-O0 O2 UBSan}; do
  slus_build_deps "$mode"
  slus_build_tu "$mode" "$SRC" "$OUT/$mode.uut.o" "${WEAK[@]}"
  slus_build_test "$mode" pc_port/tests/main_exe_work_list_retail_test.c "$OUT/$mode.test.o"
  slus_link "$mode" "$OUT/$mode" "$OUT/$mode.test.o" "$OUT/$mode.uut.o"
  rc=0
  "$OUT/$mode" > "$OUT/$mode.log" 2> "$OUT/$mode.err" || rc=$?
  cat "$OUT/$mode.log"
  if [ "$rc" != 0 ]; then echo "MAIN EXE WORK LIST regime $mode rc=$rc"; head -5 "$OUT/$mode.err" >&2; exit 1; fi
done
if [ -z "${MAIN_EXE_WORK_LIST_MODES:-}" ]; then
  cmp "$OUT/O0.log" "$OUT/O2.log"; cmp "$OUT/O0.log" "$OUT/UBSan.log"
  echo "MAIN EXE WORK LIST O0/O2/UBSan logs identical"
fi
[ -n "${MAIN_EXE_WORK_LIST_MODES:-}" ] && exit 0

# negative controls: one wrong body each; the test restricted to that function must fail
MUTS=(
 "WorkListAddTask|pEntry->onFreeCallback = WL_U32(WorkListRemoveTask);|pEntry->onFreeCallback = WL_U32(TimerWorkListRemoveTask);"
 "TimerWorkListAddTask|pEntry->unk14_3 = 1;|pEntry->unk14_3 = 0;"
)
n=0
for m in "${MUTS[@]}"; do
  fn=${m%%|*}; rest=${m#*|}; find=${rest%%|*}; repl=${rest#*|}
  # the body may have been re-translated; find a token present in the current body
  slus_mutate "$SRC" "$fn" "$find" "$repl" "$OUT/$fn.mut.c" || { echo "MUTANT SITE MISSING $fn" >&2; exit 1; }
  slus_build_tu O2 "$OUT/$fn.mut.c" "$OUT/$fn.mut.o" "${WEAK[@]}"
  slus_link O2 "$OUT/$fn.mut" "$OUT/O2.test.o" "$OUT/$fn.mut.o"
  r=0; "$OUT/$fn.mut" "$fn" > "$OUT/$fn.mut.log" 2>/dev/null || r=$?
  if [ "$r" != 1 ] || ! grep -q "DIVERGE fn=$fn" "$OUT/$fn.mut.log"; then
    cat "$OUT/$fn.mut.log"; echo "MAIN EXE WORK LIST control failed to detect defect: $fn rc=$r" >&2; exit 1
  fi
  echo "MAIN EXE WORK LIST CONTROL REJECTED $fn ($(grep -m1 DIVERGE "$OUT/$fn.mut.log" | cut -c1-100))"
  n=$((n+1))
done
echo "MAIN EXE WORK LIST ALL PASS (differential O0/O2/UBSan + $n negative controls)"
