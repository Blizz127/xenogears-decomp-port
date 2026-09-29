#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-prompt.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import pathlib, re, hashlib
b = pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest() == '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for kind, name in [('nonmatchings','801D2F4C'), ('matchings','801D1030'), ('matchings','801D32B4'), ('matchings','801D3B00')]:
    s = next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    for addr, word in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/', s):
        off = int(addr,16)-0x801c5000
        assert b[off:off+4] == bytes.fromhex(word), addr
print('PASS retail instruction pins')
PY
awk '/^(void func_801D2F4C\(u8|s32 func_801D32B4\(|void func_801D1030\()/ {body=1} body {print} /^}/ {body=0}' "${PROMPT_SOURCE:-src/menu/main/misc.c}" > "$OUT/production.inc"
awk '/^void func_801D3B00\(void\)/ {body=1} body {print} /^}/ {body=0}' "${PROMPT_SOURCE:-src/menu/main/misc.c}" > "$OUT/window.inc"
BASE=(-std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_prompt_lifecycle_test.c -o "$OUT/$mode"
    "$OUT/$mode"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -DPROMPT_REAL_ANIMATION pc_port/tests/menu_prompt_lifecycle_test.c -o "$OUT/$mode.animation"
    "$OUT/$mode.animation"
done
cp "$OUT/production.inc" "$OUT/good.inc"
for mutant in short-allocation missing-free wrong-ot; do
    case "$mutant" in
        short-allocation) sed 's/HeapAlloc(sizeof(MenuString), 0)/HeapAlloc(0x80, 0)/' "$OUT/good.inc" > "$OUT/production.inc" ;;
        missing-free) sed '/HeapFree(g_Menu->unk1DE0\[i\]);/d' "$OUT/good.inc" > "$OUT/production.inc" ;;
        wrong-ot) sed 's/ot\[4\]/ot[3]/' "$OUT/good.inc" > "$OUT/production.inc" ;;
    esac
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_prompt_lifecycle_test.c -o "$OUT/$mutant"
    if "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q 'Assertion' "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/production.inc"
