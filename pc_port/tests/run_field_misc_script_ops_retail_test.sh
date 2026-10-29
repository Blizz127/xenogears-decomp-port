#!/usr/bin/env bash
# Retail differential for the hand-written field script-opcode handlers in the
# production src/field/main/misc.c TU against the pinned disc/field.bin bytes on
# the port MIPS interpreter.  See pc_port/tests/field_misc_script_ops_retail_test.c.
# Regimes O0 / O2 / UBSan, then one deliberately-wrong mutant per body.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
SOURCE=${FIELD_MISC_OPS_SOURCE:-src/field/main/misc.c}
OUT=${FIELD_MISC_OPS_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-field-misc-ops.XXXXXXXX")}
mkdir -p "$OUT"
echo "FIELD MISC OPS OUTPUT $OUT"
sha=$(sha256sum disc/field.bin | awk '{print $1}')
[ "$sha" = 38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc ] \
  || { echo "ERROR: disc/field.bin SHA-256 mismatch: $sha" >&2; exit 1; }
cat > "$OUT/native-prototypes.h" <<'H'
/* Test-only forward declarations of existing later full-TU definitions. */
#include "common.h"
void FieldScriptSetParticleBankDirections(int);
void func_8008B978(s32);
void func_8008F7B8(void);
H
# include/common.h defines offsetof() as the (T*)0->m idiom, which UBSan flags;
# the test-only wrapper below (first on the include path) swaps in the builtin.
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
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
build_source() { # src obj flags...  (func_8008D2E0 is same-TU: keep the call, weaken it)
  local src=$1 obj=$2; shift 2
  clang "${COMMON[@]}" "$@" -fno-inline-functions -Wno-everything -c "$src" -o "$obj"
  objcopy --weaken-symbol=func_8008D2E0 "$obj"
}
# The test models SpriteData's host layout for field_0x84 and the retail layouts
# of FieldActor/ActorData; verify those constants against the production headers.
cat > "$OUT/layout.c" <<'C'
#include "common.h"
#include "field/actor.h"
_Static_assert(offsetof(SpriteData, field_0x84) == 0xBC, "SPR84_HOST");
_Static_assert(offsetof(SpriteData, position) == 0 && sizeof(((SpriteData*)0)->position) == 12, "sprite position");
_Static_assert(sizeof(FieldActor) == 0x5C, "FieldActor stride");
_Static_assert(offsetof(FieldActor, pActorData) == 0x4C && offsetof(FieldActor, pSpriteData) == 4, "actor pointer slots");
_Static_assert(offsetof(ActorData, scriptInstructionPointer) == 0xCC, "ip offset");
C
clang "${COMMON[@]}" -Wno-everything -fsyntax-only "$OUT/layout.c"
# Test and adapter are header-free hosts: no game headers (they clash with libc).
HOST=(-DSPR84_HOST=0xBC -std=gnu17 -fno-pie -DXENO_PC_PORT -ffunction-sections -fdata-sections -Ipc_port/src)
TESTC=pc_port/tests/field_misc_script_ops_retail_test.c
for mode in ${FIELD_MISC_OPS_MODES:-O0 O2 UBSan}; do
  flags=(-"$mode")
  if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi # shift-base stays ENABLED: port arms must be UB-free
  build_source "$SOURCE" "$OUT/$mode.source.o" "${flags[@]}"
  clang "${HOST[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$TESTC" -o "$OUT/$mode.test.o"
  clang "${HOST[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
  clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
  "$OUT/$mode" disc/field.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log"; exit 1; }
  cat "$OUT/$mode.log"
done
if [ -z "${FIELD_MISC_OPS_MODES:-}" ]; then
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
echo "FIELD MISC OPS O0/O2/UBSan logs identical"
fi
if [ -n "${FIELD_MISC_OPS_MODES:-}" ]; then echo "FIELD MISC OPS partial modes ${FIELD_MISC_OPS_MODES}: done (no controls)"; exit 0; fi

# Negative controls: each wrong body must make the differential go red.
python3 - "$SOURCE" "$OUT" <<'PY'
import sys, json
from pathlib import Path
s = Path(sys.argv[1]).read_text(); out = Path(sys.argv[2])
muts = [
 ('FieldActorCopyPlacement', 'dstActor->position.vy = srcActor->position.vy;', 'dstActor->position.vy = srcActor->position.vx;'),
 ('func_80087C34', '*(s16*)(pGS + 0x182E)', '*(s16*)(pGS + 0x182C)'),
 ('func_80087E98', 'u32 mask = 0xFEFFBFFF;', 'u32 mask = 0xFEFFBFFE;'),
 ('func_80088674', 'case 2: D_800B2378 = 0x20; break;', 'case 2: D_800B2378 = 0x21; break;'),
 ('func_80088790', 'scriptInstructionPointer += 8;', 'scriptInstructionPointer += 7;'),
 ('func_80088C1C', '(s32)((u32)arg1 << 8)', '(s32)((u32)arg1 << 7)'),
 ('func_80089DCC', 'D_800B22E2[arg1] = -1;', 'D_800B22E2[arg1] = 0;'),
 ('func_8008AEC8', 'D_800B221C[idx + 2]', 'D_800B221C[idx + 1]'),
 ('func_8008DA04', 'pIP[0x13] = 0x80;', 'pIP[0x13] = 0x81;'),
 ('func_8008EA58', 'D_800C3A34 = 0x100;', 'D_800C3A34 = 0x101;'),
 ('func_8008764C', 'pDst + 160) = *(s32*)(pSrc + 160);', 'pDst + 160) = *(s32*)(pSrc + 156);'),
 ('func_8008F90C', 'dy = (s32)ny / steps;', 'dy = (s32)nz / steps;'),
 ('func_8008EC30', 'D_800ADB80 = arg7 & 0xC0;', 'D_800ADB80 = arg7 & 0x40;'),
]
ctl = []
for fn, old, new in muts:
    # pick the definition (not an extern/forward declaration): the occurrence whose body holds `old`
    a = -1; b = -1
    pos = s.find('\nvoid %s(' % fn)
    while pos != -1:
        e = s.find('\n}\n', pos)
        if s.find(old, pos, e) != -1:
            a, b = pos, e; break
        pos = s.find('\nvoid %s(' % fn, pos + 1)
    assert a != -1, (fn, 'body not found')
    body = s[a:b]
    assert body.count(old) == 1, (fn, old, body.count(old))
    (out / (fn + '.mut.c')).write_text(s[:a] + body.replace(old, new) + s[b:])
    ctl.append(fn)
(out / 'controls.txt').write_text('\n'.join(ctl) + '\n')
PY
while read -r fn; do
  build_source "$OUT/$fn.mut.c" "$OUT/$fn.source.o" -O2
  clang -no-pie -O2 -Wl,--gc-sections "$OUT/$fn.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$fn.mut"
  rc=0
  "$OUT/$fn.mut" disc/field.bin "$fn" > "$OUT/$fn.mut.log" 2>&1 || rc=$?
  if [ "$rc" != 1 ] || ! grep -q '^FIELD MISC OPS FAIL ' "$OUT/$fn.mut.log"; then
    cat "$OUT/$fn.mut.log"; echo "FIELD MISC OPS control failed to detect defect: $fn rc=$rc" >&2; exit 1
  fi
  echo "FIELD MISC OPS CONTROL REJECTED $fn ($(grep -m1 '^FIELD MISC OPS FAIL' "$OUT/$fn.mut.log" | cut -c1-110))"
done < "$OUT/controls.txt"
echo "FIELD MISC OPS ALL PASS (differential O0/O2/UBSan + $(wc -l < "$OUT/controls.txt") negative controls)"
