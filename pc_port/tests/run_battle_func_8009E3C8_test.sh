#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."

OUT="${XENO_E3C8_TEST_OUTDIR:-$(mktemp -d pc_port/build_native/func_8009e3c8.XXXXXXXX)}"
mkdir -p "$OUT"
COMMON=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DXENO_BATTLE_OVERLAY_HOST_BODIES
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src)

for mode in O0 O2 UBSan; do
    case "$mode" in
        O0) flags=(-O0) ;;
        O2) flags=(-O2) ;;
        UBSan) flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all) ;;
    esac
    gcc "${COMMON[@]}" "${flags[@]}" -c src/battle/main62.c -o "$OUT/$mode.main62.o"
    gcc "${COMMON[@]}" "${flags[@]}" -c pc_port/tests/battle_func_8009E3C8_test.c \
        -o "$OUT/$mode.test.o"
    gcc -no-pie "${flags[@]}" -Wl,--gc-sections \
        "$OUT/$mode.test.o" "$OUT/$mode.main62.o" -o "$OUT/$mode.test"
    "$OUT/$mode.test"
done
