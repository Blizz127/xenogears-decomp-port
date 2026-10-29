#!/usr/bin/env bash
# W34N130: wm_80094A5C against the retail 0x80094A5C instructions from the disc.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d /tmp/w34n130.XXXXXX)
trap 'rm -rf "$OUT"' EXIT
build() {
    local name=$1; shift
    gcc -std=gnu17 -O2 "$@" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -DWM_94A5C_TEST_TRACE -DWM_94A5C_TEST_HOOK -Wall -Wextra -Werror \
        -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        pc_port/tests/w34n130_94a5c_retail_diff_test.c pc_port/src/world_map_func_94a5c.c \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$name"
}
build pass
"$OUT/pass"
build ubsan -fsanitize=undefined -fno-sanitize-recover=all
"$OUT/ubsan" > /dev/null
build mutant -DWM_94A5C_MUTANT_WS_BYTE2
build mutant2 -DWM_94A5C_MUTANT_CELL_OR
if "$OUT/mutant2" > "$OUT/mutant2.log" 2>&1; then echo "W34N130 FAIL: the OR-cell mutant survived" >&2; exit 1; fi
grep -q "W34N130 FAIL case" "$OUT/mutant2.log"
echo "W34N130 negative control PASS: the former OR of the 0x80000 cell is rejected"
if "$OUT/mutant" > "$OUT/mutant.log" 2>&1; then
    echo "W34N130 FAIL: the WS+2 mutant survived" >&2; exit 1
fi
grep -q "W34N130 FAIL case" "$OUT/mutant.log"
echo "W34N130 negative control PASS: the former WS+2 read is rejected"
echo "W34N130 94A5C RETAIL DIFF PASS"
