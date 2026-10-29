#!/usr/bin/env bash
# Retail-differential unit test for pc_port/src/psyq_compat_transpose_matrix.c
# (Psy-Q TransposeMatrix, absent from PsyCross; the world map's native
# func_80087FD0 / func_80087B84 call it). Compares the native body with the
# retail bytes at 0x8004A8EC run in the MIPS interpreter, then checks that
# each fault-injected variant is rejected.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${PSYQ_TRANSPOSE_MATRIX_OUT:-pc_port/build_native/psyq_transpose_matrix}
PSX=${XENO_PSYCROSS_DIR:-pc_port/extern/PsyCross}
EXE=${PSYQ_TRANSPOSE_MATRIX_EXE:-disc/SLUS_006.64}
mkdir -p "$OUT"
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
 -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
 -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$PSX/include" -I"$PSX/include/psx")
for mode in O0 O2 UBSan; do
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/psyq_compat_transpose_matrix.c -o "$OUT/$mode.compat.o"
 gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/psyq_transpose_matrix_retail_test.c -o "$OUT/$mode.test.o"
 gcc "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 gcc -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" "$EXE" | tee "$OUT/$mode.log"
done
for mutant in NAIVE_INPLACE DROP_M22 TOUCH_T RETURN_INPUT; do
 gcc "${COMMON[@]}" -O2 -w -DTRANSPOSE_MATRIX_MUTANT_"$mutant" -c pc_port/src/psyq_compat_transpose_matrix.c -o "$OUT/mutant.o"
 gcc -no-pie -O2 -Wl,--gc-sections "$OUT/mutant.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/mutant"
 rc=0; "$OUT/mutant" "$EXE" > "$OUT/$mutant.log" 2>&1 || rc=$?
 if [ "$rc" = 0 ] || ! grep -q '^TRANSPOSE MATRIX FAIL' "$OUT/$mutant.log"; then
  echo "TRANSPOSE MATRIX MUTANT SURVIVED: $mutant" >&2; exit 1
 fi
 echo "TRANSPOSE MATRIX MUTANT REJECTED: $mutant ($(grep -m1 '^TRANSPOSE MATRIX FAIL' "$OUT/$mutant.log"))"
done
