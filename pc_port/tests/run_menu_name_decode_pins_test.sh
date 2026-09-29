#!/usr/bin/env bash
# Pins func_801CB184's single-buffer name decode (see menu_name_decode_pins_test.c)
# and checks that the old even/odd split is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-name-decode.XXXXXX")
echo "OUTPUT $OUT"
SRC="${NAME_DECODE_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801CB184\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/decode.inc"
test "$(grep -c '^void func_801CB184' "$OUT/decode.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 UBSan; do
    flags=(-O0); [[ "$mode" == UBSan ]] && flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_name_decode_pins_test.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/decode.inc" "$OUT/good.inc"
for mutant in split-buffers wrong-pair-count; do
    cp "$OUT/good.inc" "$OUT/decode.inc"
    case "$mutant" in
        split-buffers) sed -i 's/buf\[i + 1\] = e\[i + 1\];/out[i \/ 2] = e[i + 1];/' "$OUT/decode.inc" ;;
        wrong-pair-count) sed -i 's/, i \/ 2);/, i);/' "$OUT/decode.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/decode.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_name_decode_pins_test.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" >/dev/null 2>&1; then echo "FAIL mutant $mutant accepted"; exit 1; fi
    echo "REJECTED $mutant"
done
