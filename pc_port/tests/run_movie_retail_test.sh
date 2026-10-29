#!/usr/bin/env bash
# Retail differential for the hand-written XENO_PC_PORT arms of src/movie/main.c
# vs the pinned disc/movie.bin (and disc/SLUS_006.64 for the real libgpu leaves)
# on the port MIPS interpreter (test oracle only).  See movie_retail_test.c.
# Five groups (leaf, sector, play, main, fat) = five link variants of one test;
# regimes O0 / O2 / UBSan (shift-base on), then one mutant per body.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
source pc_port/tests/lib/slus_oracle_lib.sh
OUT=${MOVIE_RETAIL_OUT:-pc_port/build_native/movie_retail}
mkdir -p "$OUT/inc"
slus_check_disc
MOVIE=${XENO_MOVIE:-disc/movie.bin}
msha=$(sha256sum "$MOVIE" | awk '{print $1}')
[ "$msha" = 50e1a9d9e08b90eed0c2da1c289507e71cbf51749893a92f457ce79a59701f9a ] \
  || { echo "ERROR: $MOVIE SHA-256 mismatch: $msha" >&2; exit 1; }
export XENO_MOVIE="$MOVIE"
SRC=${MOVIE_RETAIL_SOURCE:-src/movie/main.c}
# include/common.h defines offsetof() as the (T*)0->m idiom, which UBSan flags;
# the test-only wrapper (first on the include path) swaps in the builtin.
cat > "$OUT/inc/common.h" <<'H'
#include_next "common.h"
#undef offsetof
#define offsetof(s, m) __builtin_offsetof(s, m)
H
# TU callees that are spies in some group: the TU copy is weakened so the test's stub wins
WEAK=(func_80072D84 func_80072F98 func_80073328 func_800734B8 func_80074BA4 func_8007519C func_80075534
      func_800704E8 func_80072480 func_80075D8C func_8007625C func_800763BC)
GROUPS_=(1 2 3 4 5)
TUF=(-fPIC -fno-inline -fno-ipa-cp -fno-ipa-sra -fno-ipa-icf -fno-partial-inlining)
build_movie_tu() { # mode src obj
  local mode=$1 src=$2 obj=$3 s syms
  slus_flags "$mode"
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" -I"$OUT/inc" "${SLUS_FL[@]}" -fpermissive -w "${TUF[@]}" -c "$src" -o "$obj"
  syms=$(nm "$obj")
  for s in "${WEAK[@]}"; do
    if grep -q " T $s\$" <<<"$syms"; then objcopy --weaken-symbol="$s" "$obj"; fi
  done
}
# generated host layout of MovieRenderEnv
gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" -I"$OUT/inc" -O0 -fpermissive -w -no-pie \
  -Wl,--unresolved-symbols=ignore-all pc_port/tests/movie_layout_gen.c -o "$OUT/layout_gen"
"$OUT/layout_gen" > "$OUT/movie_retail_layout.h"
build_test() { # mode grp obj
  local mode=$1 grp=$2 obj=$3
  slus_flags "$mode"
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -DGRP="$grp" -I"$OUT" -w -c pc_port/tests/movie_retail_test.c -o "$obj"
}
link_one() { # mode exe objs...
  local mode=$1 exe=$2; shift 2
  slus_flags "$mode"
  clang++ -no-pie "${SLUS_FL[@]}" -Wl,--gc-sections "$@" "$OUT/$mode.dep."*.o -ldl -lm -o "$exe"
}
for mode in ${MOVIE_RETAIL_MODES:-O0 O2 UBSan}; do
  slus_build_deps "$mode"
  build_movie_tu "$mode" "$SRC" "$OUT/$mode.uut.o"
  slus_flags "$mode"
  gcc "${SLUS_COMMON[@]}" "${SLUS_FL[@]}" -std=gnu17 -include assert.h -w -c pc_port/tests/movie_gpu_leaf.c -o "$OUT/$mode.dep.gpuleaf.o"
  : > "$OUT/$mode.log"
  for g in "${GROUPS_[@]}"; do
    build_test "$mode" "$g" "$OUT/$mode.g$g.test.o"
    link_one "$mode" "$OUT/$mode.g$g" "$OUT/$mode.g$g.test.o" "$OUT/$mode.uut.o"
    rc=0
    "$OUT/$mode.g$g" ${MOVIE_RETAIL_ONLY:-} > "$OUT/$mode.g$g.log" 2> "$OUT/$mode.g$g.err" || rc=$?
    cat "$OUT/$mode.g$g.log"; cat "$OUT/$mode.g$g.log" >> "$OUT/$mode.log"
    if [ "$rc" != 0 ]; then echo "MOVIE RETAIL group $g regime $mode rc=$rc"; head -5 "$OUT/$mode.g$g.err" >&2; exit 1; fi
  done
done
if [ -z "${MOVIE_RETAIL_MODES:-}" ]; then
  cmp "$OUT/O0.log" "$OUT/O2.log"; cmp "$OUT/O0.log" "$OUT/UBSan.log"
  echo "MOVIE RETAIL O0/O2/UBSan logs identical"
fi
[ -n "${MOVIE_RETAIL_MODES:-}" ] && exit 0

# negative controls: one wrong body each; the test restricted to that function must fail
MUTS=(
 "1|func_80072D84|D_80076F8C[i][1] = 8;|D_80076F8C[i][1] = 9;"
 "1|func_80072F98|D_80076FAC[i]++;|D_80076FAC[i] += 2;"
 "1|func_80073328|D_80076FCC[i][0] = 0xFF;|D_80076FCC[i][0] = 0xFE;"
 "1|func_800734B8|len = D_80076FFC[i];|len = D_80076FFC[i] + 1;"
 "1|func_80074AF0|u32 b = (u32)D_80076EF8 * 7u + 3u;|u32 b = (u32)D_80076EF8 * 6u + 3u;"
 "2|func_80074BA4|sectorSize = 0x920;|sectorSize = 0x900;"
 "2|func_8007519C|return hdr->frameNumber;|return hdr->frameNumber + 1;"
 "3|func_800763BC|D_80077028 = noSkip;|D_80077028 = noSkip + 1;"
 "3|func_8007625C|D_80077124[0].disp.isrgb24 = 1;|D_80077124[0].disp.isrgb24 = 0;"
 "3|func_80076488|pace += 30;|pace += 31;"
 "4|MovieMain|D_8007739C = 0xE9;|D_8007739C = 0xEA;"
 "5|func_80075D8C|first -= 0x14;|first -= 0x13;"
)
n=0
for m in "${MUTS[@]}"; do
  g=${m%%|*}; rest=${m#*|}; fn=${rest%%|*}; rest=${rest#*|}; find=${rest%%|*}; repl=${rest#*|}
  slus_mutate "$SRC" "$fn" "$find" "$repl" "$OUT/$fn.mut.c" || { echo "MUTANT SITE MISSING $fn" >&2; exit 1; }
  build_movie_tu O2 "$OUT/$fn.mut.c" "$OUT/$fn.mut.o"
  link_one O2 "$OUT/$fn.mut" "$OUT/O2.g$g.test.o" "$OUT/$fn.mut.o"
  r=0; "$OUT/$fn.mut" "$fn" > "$OUT/$fn.mut.log" 2>/dev/null || r=$?
  if [ "$r" != 1 ] || ! grep -q "MOVIE RETAIL DIVERGE fn=$fn" "$OUT/$fn.mut.log"; then
    cat "$OUT/$fn.mut.log"; echo "MOVIE RETAIL control failed to detect defect: $fn rc=$r" >&2; exit 1
  fi
  echo "MOVIE RETAIL CONTROL REJECTED $fn ($(grep -m1 'MOVIE RETAIL DIVERGE' "$OUT/$fn.mut.log" | cut -c1-110))"
  n=$((n+1))
done
echo "MOVIE RETAIL ALL PASS (differential O0/O2/UBSan + $n negative controls)"
