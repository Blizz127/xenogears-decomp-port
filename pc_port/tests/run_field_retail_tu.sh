#!/usr/bin/env bash
# Generic runner: retail differential of one production field TU against the
# pinned disc/field.bin on the port MIPS interpreter (O0/O2/UBSan + one mutant
# per body).  usage: run_field_retail_tu.sh <conf>   (conf in field_retail_tu/)
# conf defines: NAME SOURCE TESTC WEAKEN PROTOS(text) MUTANTS(py file) [LAYOUT(C text)]
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE/../.."
ulimit -c 0
LAYOUT=""; PROTOS=""; WEAKEN=""; EXTRA_COMMON=(); PROD_CC=clang
# shellcheck disable=SC1090
source "$HERE/field_retail_tu/$1.conf"
OUT=${FIELD_TU_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-ftu-$NAME.XXXXXXXX")}
mkdir -p "$OUT"
echo "FIELD TU $NAME OUTPUT $OUT"
sha=$(sha256sum disc/field.bin | awk '{print $1}')
[ "$sha" = 38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc ] \
  || { echo "ERROR: disc/field.bin SHA-256 mismatch: $sha" >&2; exit 1; }
printf '#include "common.h"\n%s\n' "$PROTOS" > "$OUT/native-prototypes.h"
mkdir -p "$OUT/inc"
cat > "$OUT/inc/common.h" <<'H'
#include_next "common.h"
#undef offsetof
#define offsetof(s, m) __builtin_offsetof(s, m)
H
COMMON=(-I"$OUT/inc" -include assert.h -include "$OUT/native-prototypes.h" -std=gnu17 -fno-pie -fno-builtin
        -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
        -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
        -Ipc_port/include_shim -Iinclude -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx "${EXTRA_COMMON[@]}")
build_source() { # src obj flags...
  local src=$1 obj=$2; shift 2
  if [ "$PROD_CC" = clang ]; then W=(-Wno-everything); else W=(-fpermissive -w); fi
  # Spied same-TU callees must stay opaque: with plain non-PIC code the compiler's
  # IPA (pure/const inference, modref, ipa-cp) assumes the in-TU body and e.g.
  # CSEs two calls of a "pure" callee that the retail code makes twice.
  # -fPIC + semantic interposition makes every global definition interposable.
  local IP=(); [ -n "$WEAKEN" ] && IP=(-fPIC -fsemantic-interposition)
  # mutant copies live in $OUT: keep the original directory for "relative" includes
  "$PROD_CC" "${COMMON[@]}" -iquote "$(dirname "$SOURCE")" "$@" "${IP[@]}" -fno-inline "${W[@]}" -c "$src" -o "$obj"
  # same-TU callees: spied by the test (strong spy overrides the weak definition);
  # the real body stays callable as real_<name> through a global alias symbol.
  for w in $WEAKEN; do objcopy --weaken-symbol="$w" --add-symbol "real_$w=.text.$w:0,global,function" "$obj"; done
}
if [ -n "$LAYOUT" ]; then
  printf '#include "common.h"\n#include "field/actor.h"\n%s\n' "$LAYOUT" > "$OUT/layout.c"
  clang "${COMMON[@]}" -Wno-everything -fsyntax-only "$OUT/layout.c"
fi
HOST=(-std=gnu17 -fno-pie -DXENO_PC_PORT -ffunction-sections -fdata-sections -Ipc_port/src -Ipc_port/tests/lib)
for mode in ${FIELD_TU_MODES:-O0 O2 UBSan}; do
  flags=(-"$mode")
  if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
  build_source "$SOURCE" "$OUT/$mode.source.o" "${flags[@]}"
  clang "${HOST[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$TESTC" -o "$OUT/$mode.test.o"
  clang "${HOST[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
  clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -lm -o "$OUT/$mode"
  "$OUT/$mode" disc/field.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log"; exit 1; }
  cat "$OUT/$mode.log"
done
if [ -n "${FIELD_TU_MODES:-}" ]; then echo "FIELD TU $NAME partial modes ${FIELD_TU_MODES}: done (no controls)"; exit 0; fi
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
echo "FIELD TU $NAME O0/O2/UBSan logs identical"
python3 "$HERE/lib/field_mutants.py" "$SOURCE" "$OUT" "$HERE/field_retail_tu/$MUTANTS"
while read -r fn; do
  build_source "$OUT/$fn.mut.c" "$OUT/$fn.source.o" -O2
  clang -no-pie -O2 -Wl,--gc-sections "$OUT/$fn.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -lm -o "$OUT/$fn.mut"
  rc=0
  "$OUT/$fn.mut" disc/field.bin "$fn" > "$OUT/$fn.mut.log" 2>&1 || rc=$?
  if [ "$rc" != 1 ] || ! grep -q ' FAIL ' "$OUT/$fn.mut.log"; then
    cat "$OUT/$fn.mut.log"; echo "FIELD TU $NAME control failed to detect defect: $fn rc=$rc" >&2; exit 1
  fi
  echo "FIELD TU $NAME CONTROL REJECTED $fn ($(grep -m1 ' FAIL ' "$OUT/$fn.mut.log" | cut -c1-100))"
done < "$OUT/controls.txt"
echo "FIELD TU $NAME ALL PASS (differential O0/O2/UBSan + $(wc -l < "$OUT/controls.txt") negative controls)"
