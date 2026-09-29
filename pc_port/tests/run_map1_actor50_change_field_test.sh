#!/usr/bin/env bash
# Disc-backed map 1 actor 50 CHANGE_FIELD certificate + production
# func_800932D0 (opcode 152) handler: no story-var gate.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${MAP1_ACTOR50_OUT:-$ROOT/pc_port/build_native/map1_actor50}"
mkdir -p "$OUT"
python3 pc_port/tests/map1_actor50_change_field_test.py | tee "$OUT/decode.stdout"

CC="${CC:-gcc}"
BASE=(-std=gnu17 -fno-pie -no-pie -m64 -fno-builtin
      -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -w -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
"$CC" "${BASE[@]}" "${INC[@]}" -O0 \
    pc_port/tests/change_field_handler_prod_test.c \
    src/field/main/misc11.c \
    -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all \
    -o "$OUT/change_field_O0"
"$OUT/change_field_O0" | tee "$OUT/handler.O0.stdout"
"$CC" "${BASE[@]}" "${INC[@]}" -O2 \
    pc_port/tests/change_field_handler_prod_test.c \
    src/field/main/misc11.c \
    -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all \
    -o "$OUT/change_field_O2"
"$OUT/change_field_O2" | tee "$OUT/handler.O2.stdout"
echo "MAP1_ACTOR50 overall=PASS"
