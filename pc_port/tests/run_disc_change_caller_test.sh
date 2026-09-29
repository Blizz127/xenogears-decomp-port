#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/disc-change-caller.XXXXXX")
echo "OUTPUT $OUT"
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
# Keep surrounding preprocessor branches when extracting the production body.
awk '/^void func_801C8694\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/disc_caller.inc"
test -s "$OUT/disc_caller.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/disc_change_caller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 60s "$OUT/$mode"
done
cp "$OUT/disc_caller.inc" "$OUT/good.inc"
for mutant in raw-state zero-state-exit byte-target full-input short-delay skipped-success-close; do
    case "$mutant" in
        raw-state) sed 's/g_Menu->transitionEffectState/(*(u8*)((u8*)g_Menu + 0x329))/g' "$OUT/good.inc" > "$OUT/disc_caller.inc" ;;
        zero-state-exit) sed '/^    func_801D1E80();/a\    if (g_Menu->transitionEffectState == 0) { func_801D2484(); return; }' "$OUT/good.inc" > "$OUT/disc_caller.inc" ;;
        byte-target) sed 's/s32 targetDisc = (u8)discNum + 1;/u8 targetDisc = (u8)discNum + 1;/' "$OUT/good.inc" > "$OUT/disc_caller.inc" ;;
        full-input) sed 's/s32 targetDisc = (u8)discNum + 1;/s32 targetDisc = (u32)discNum + 1u;/' "$OUT/good.inc" > "$OUT/disc_caller.inc" ;;
        short-delay) sed 's/delay = 0x1D/delay = 0x1C/' "$OUT/good.inc" > "$OUT/disc_caller.inc" ;;
        skipped-success-close) sed '/MENU_CALL_D32B4();/{N;/waiting = 0;/s/^[^\n]*\n//}' "$OUT/good.inc" > "$OUT/disc_caller.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/disc_caller.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/disc_change_caller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 60s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
