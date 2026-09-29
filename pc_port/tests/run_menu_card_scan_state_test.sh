#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-scan-state.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801C8EE8.s')).read_text()
for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail scan-state instruction pins')
PY
awk '/^void func_801C8EE8\(void\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_SCAN_STATE_SOURCE:-src/menu/main/misc.c}" > "$OUT/scan_state.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_scan_state_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/scan_state.inc" "$OUT/good.inc"
for mutant in wide-result unconditional-refresh missing-mode2 wrong-port; do
    case "$mutant" in
        wide-result) sed 's/(u8)func_801C8D78(\([01]\)) != 0/func_801C8D78(\1) != 0/' "$OUT/good.inc" > "$OUT/scan_state.inc" ;;
        unconditional-refresh) sed 's/if ((u8)func_801C8D78(\([01]\)) != 0)/func_801C8D78(\1); if (1)/' "$OUT/good.inc" > "$OUT/scan_state.inc" ;;
        missing-mode2) sed 's/case 2:/case 3:/' "$OUT/good.inc" > "$OUT/scan_state.inc" ;;
        wrong-port) sed 's/func_801C8D78(1)/func_801C8D78(0)/' "$OUT/good.inc" > "$OUT/scan_state.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/scan_state.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_scan_state_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 5s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/scan_state.inc"
