#!/usr/bin/env bash
# Retail differential for hand-written menu functions (memory-card screen and
# window-border builders) in the production src/menu/main/misc.c TU against
# the pinned disc/menu.bin bytes on the port MIPS interpreter.  See
# pc_port/tests/menu_card_retail_test.c.  Regimes O0 / O2 / UBSan (shift-base
# ENABLED), then one deliberately-wrong mutant per body.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
CC=${CC:-gcc}
SOURCE=${MENU_CARD_SOURCE:-src/menu/main/misc.c}
OUT=${MENU_CARD_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-menu-card.XXXXXXXX")}
mkdir -p "$OUT/inc"
echo "MENU CARD RETAIL OUTPUT $OUT"
sha=$(sha256sum disc/menu.bin | awk '{print $1}')
[ "$sha" = 82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d ] \
  || { echo "ERROR: disc/menu.bin SHA-256 mismatch: $sha" >&2; exit 1; }
# include/common.h defines offsetof() as the (T*)0->m idiom, which UBSan flags;
# the test-only wrapper below (first on the include path) swaps in the builtin.
cat > "$OUT/inc/common.h" <<'H'
#include_next "common.h"
#undef offsetof
#define offsetof(s, m) __builtin_offsetof(s, m)
H
COMMON=(-I"$OUT/inc" -include assert.h -std=gnu17 -fno-pie -fno-builtin
        -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
        -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -fpermissive -w
        -Ipc_port/include_shim -Iinclude -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
# Host struct offsets come from the production headers, never hand-copied.
"$CC" "${COMMON[@]}" -no-pie -O0 pc_port/tests/menu_card_retail_layout_gen.c -o "$OUT/layout_gen"
"$OUT/layout_gen" > "$OUT/inc/menu_retail_layout.h"
build_source() { # src obj flags...  (callee spies defined by the test are weakened at link)
  local src=$1 obj=$2; shift 2
  # -fPIC makes every global function interposable: the optimiser may not assume the TU's own callee bodies
  # (which the test replaces with spies) when it propagates return ranges or constants.
  "$CC" "${COMMON[@]}" -iquote "$(dirname "$SOURCE")" "$@" -fPIC -fno-inline -fno-ipa-cp -fno-ipa-sra -fno-ipa-icf -fno-partial-inlining -c "$src" -o "$obj"
}
HOST=(-std=gnu17 -fno-pie -DXENO_PC_PORT -ffunction-sections -fdata-sections -I"$OUT/inc" -Ipc_port/src)
TESTC=pc_port/tests/menu_card_retail_test.c
link_one() { # source-obj test-obj cpu-obj outbin flags...
  local src=$1 tst=$2 cpu=$3 bin=$4; shift 4
  local weak=()
  # every symbol the test defines strongly and the TU also defines is a spy/alias: weaken the TU's copy
  while read -r sym; do weak+=(--weaken-symbol="$sym"); done < <(
    comm -12 <(nm --defined-only -g "$tst" | awk '{print $3}' | sort -u) \
             <(nm --defined-only -g "$src" | awk '{print $3}' | sort -u))
  [ ${#weak[@]} -eq 0 ] || objcopy "${weak[@]}" "$src"
  "$CC" -no-pie "$@" -Wl,--gc-sections "$src" "$tst" "$cpu" -o "$bin"
}
for mode in ${MENU_CARD_MODES:-O0 O2 UBSan}; do
  flags=(-"$mode")
  if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi # shift-base stays ENABLED
  build_source "$SOURCE" "$OUT/$mode.source.o" "${flags[@]}"
  "$CC" "${HOST[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$TESTC" -o "$OUT/$mode.test.o"
  "$CC" "${HOST[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
  link_one "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode" "${flags[@]}"
  "$OUT/$mode" disc/menu.bin ${MENU_CARD_ONLY:-} > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log"; exit 1; }
  cat "$OUT/$mode.log"
done
if [ -z "${MENU_CARD_MODES:-}" ]; then
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
echo "MENU CARD RETAIL O0/O2/UBSan logs identical"
fi
if [ -n "${MENU_CARD_MODES:-}" ]; then echo "MENU CARD RETAIL partial modes ${MENU_CARD_MODES}: done (no controls)"; exit 0; fi

# Negative controls: each wrong body must make the differential go red.
python3 - "$SOURCE" "$OUT" <<'PY'
import sys, re
from pathlib import Path
s = Path(sys.argv[1]).read_text(); out = Path(sys.argv[2])
muts = [
 ('func_801C8324', 's->accX = (s32)((u32)s->accX - (u32)s->stepX);', 's->accX = (s32)((u32)s->accX + (u32)s->stepX);'),
 ('func_801C9270', 'pData[port * 0x10 + i + MENU_CARD_OFF(0x4F8E)] = 1;', 'pData[port * 0x10 + i + MENU_CARD_OFF(0x4F8E)] = 2;'),
 ('func_801C9BCC', 'g_Menu->unk4CC[0xC] = 2;', 'g_Menu->unk4CC[0xC] = 1;'),
 ('func_801C9D34', 's32 result = 0xFF;', 's32 result = 0xFE;'),
 ('func_801CD710', 'g_Menu->unk334 = 6;', 'g_Menu->unk334 = 5;'),
 ('func_801CAA38', 'selected = 1;', 'selected = 2;'),
 ('func_801CF5E4', 'poly[0x1D] = (u8)(source + 0x10);', 'poly[0x1D] = (u8)(source + 0x11);'),
 ('func_801CB184', 'func_80033B34(bufWords, out, i / 2);', 'func_80033B34(bufWords, out, (i + 2) / 2);'),
 ('func_801CBA4C', 'buf[0x23] = saveNum;', 'buf[0x23] = (u8)(saveNum + 1);'),
 ('func_801CF8D8', 'other ? 0x115 : 0x122', 'other ? 0x115 : 0x123'),
 ('func_801D3FF8', 'p1->u1 = 7;', 'p1->u1 = 6;'),
 ('func_801D433C', 'p2->u3 = 0xF;', 'p2->u3 = 0xE;'),
 ('func_801D4688', 'halfH = ((h & 0xFFFF) - 0x10) / 2;', 'halfH = ((h & 0xFFFF) - 0x11) / 2;'),
 ('func_801D49D0', 'p1->v0 = 0x8C;', 'p1->v0 = 0x8D;'),
]
ctl = []
for fn, old, new in muts:
    a = b = -1
    for m in re.finditer(r'\n(?:void|s32|u32|u8|int) %s\(' % fn, s):
        e = s.find('\n}\n', m.start())
        if s.find(old, m.start(), e) != -1:
            a, b = m.start(), e; break
    assert a != -1, (fn, 'body not found')
    body = s[a:b]
    assert body.count(old) == 1, (fn, old, body.count(old))
    (out / (fn + '.mut.c')).write_text(s[:a] + body.replace(old, new) + s[b:])
    ctl.append(fn)
(out / 'controls.txt').write_text('\n'.join(ctl) + '\n')
PY
while read -r fn; do
  build_source "$OUT/$fn.mut.c" "$OUT/$fn.source.o" -O2
  link_one "$OUT/$fn.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" "$OUT/$fn.mut" -O2
  rc=0
  "$OUT/$fn.mut" disc/menu.bin "$fn" > "$OUT/$fn.mut.log" 2>&1 || rc=$?
  if [ "$rc" != 1 ] || ! grep -q '^MENU CARD RETAIL FAIL ' "$OUT/$fn.mut.log"; then
    cat "$OUT/$fn.mut.log"; echo "MENU CARD RETAIL control failed to detect defect: $fn rc=$rc" >&2; exit 1
  fi
  echo "MENU CARD RETAIL CONTROL REJECTED $fn ($(grep -m1 '^MENU CARD RETAIL FAIL' "$OUT/$fn.mut.log" | cut -c1-110))"
done < "$OUT/controls.txt"
echo "MENU CARD RETAIL ALL PASS (differential O0/O2/UBSan + $(wc -l < "$OUT/controls.txt") negative controls)"
