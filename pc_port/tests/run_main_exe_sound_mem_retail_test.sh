#!/usr/bin/env bash
# Retail differential for the hand-written XENO_PC_PORT arms in
# src/slus_006.64/system/sound.c vs disc/SLUS_006.64 on the port MIPS
# interpreter (test oracle only).  See main_exe_sound_mem_retail_test.c.
# Regimes O0 / O2 / UBSan (shift-base on), then one mutant per body.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
source pc_port/tests/lib/slus_oracle_lib.sh
OUT=${MAIN_EXE_SOUND_MEM_OUT:-pc_port/build_native/main_exe_sound_mem}
mkdir -p "$OUT"
slus_check_disc
SRC=${MAIN_EXE_SOUND_MEM_SOURCE:-src/slus_006.64/system/sound.c}
WEAK=(none)
for mode in ${MAIN_EXE_SOUND_MEM_MODES:-O0 O2 UBSan}; do
  slus_build_deps "$mode"
  slus_build_tu "$mode" "$SRC" "$OUT/$mode.uut.o" "${WEAK[@]}"
  slus_build_test "$mode" pc_port/tests/main_exe_sound_mem_retail_test.c "$OUT/$mode.test.o"
  slus_link "$mode" "$OUT/$mode" "$OUT/$mode.test.o" "$OUT/$mode.uut.o"
  rc=0
  "$OUT/$mode" > "$OUT/$mode.log" 2> "$OUT/$mode.err" || rc=$?
  cat "$OUT/$mode.log"
  if [ "$rc" != 0 ]; then echo "MAIN EXE SOUND MEM regime $mode rc=$rc"; head -5 "$OUT/$mode.err" >&2; exit 1; fi
done
if [ -z "${MAIN_EXE_SOUND_MEM_MODES:-}" ]; then
  cmp "$OUT/O0.log" "$OUT/O2.log"; cmp "$OUT/O0.log" "$OUT/UBSan.log"
  echo "MAIN EXE SOUND MEM O0/O2/UBSan logs identical"
fi
[ -n "${MAIN_EXE_SOUND_MEM_MODES:-}" ] && exit 0

# negative controls: one wrong body each; the test restricted to that function must fail
MUTS=(
 "SoundSpuMemoryAllocateBlock|pNew->size = size;|pNew->size = size + 1;"
 "func_800394B8|pNew->spuAddress = bestAddr;|pNew->spuAddress = bestAddr + 1;"
 "SoundSpuMemoryAllocateBlockAtAddress|pNext->spuAddress = addr;|pNext->spuAddress = addr + 1;"
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
    cat "$OUT/$fn.mut.log"; echo "MAIN EXE SOUND MEM control failed to detect defect: $fn rc=$r" >&2; exit 1
  fi
  echo "MAIN EXE SOUND MEM CONTROL REJECTED $fn ($(grep -m1 DIVERGE "$OUT/$fn.mut.log" | cut -c1-100))"
  n=$((n+1))
done
echo "MAIN EXE SOUND MEM ALL PASS (differential O0/O2/UBSan + $n negative controls)"
