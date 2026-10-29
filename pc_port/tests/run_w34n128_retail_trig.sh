#!/usr/bin/env bash
# W34N128: the world map's decomp-named trig calls must return what the retail
# entry points return (0x8003F8B0 = sine, 0x8003F8CC = cosine).  Links the real
# PsyCross LIBGTE.C; the oracle is the retail code/table from the user's disc.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d /tmp/w34n128.XXXXXX)
trap 'rm -rf "$OUT"' EXIT
common=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for s in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
    g++ -std=c++17 "${common[@]}" -O2 -fpermissive -w -include pc_port/src/port_compat.h \
        -c "pc_port/extern/PsyCross/src/$s" -o "$OUT/${s##*/}.o"
done
build() {
    local name=$1; shift
    gcc -std=gnu17 "${common[@]}" -O2 "$@" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -Wall -Wextra -Wconversion -Werror -Iinclude -Ipc_port/src \
        -c pc_port/tests/w34n128_retail_trig_test.c -o "$OUT/$name.o"
    gcc -std=gnu17 "${common[@]}" -O2 -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"
    g++ -no-pie -Wl,--gc-sections "$OUT/$name.o" "$OUT/cpu.o" "$OUT/LIBGTE.C.o" \
        "$OUT/INLINE_C.C.o" "$OUT/PsyX_GTE.cpp.o" -o "$OUT/$name"
}
build pass
"$OUT/pass"
build mutant -DW34N128_MUTANT_CONVENTIONAL
if "$OUT/mutant" > "$OUT/mutant.log" 2>&1; then
    echo "W34N128 FAIL: the conventional-trig mutant survived" >&2; exit 1
fi
grep -q "W34N128 FAIL" "$OUT/mutant.log"
echo "W34N128 negative control PASS: conventional PsyCross rcos/rsin under the decomp names is rejected"
echo "W34N128 RETAIL TRIG PASS"
