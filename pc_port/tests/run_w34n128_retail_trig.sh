#!/usr/bin/env bash
# Compare renderer trig with two guest entries in the owner's SLUS executable.
set -euo pipefail
cd "$(dirname "$0")/../.."
printf '%s  %s\n' dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 \
    disc/SLUS_006.64 | sha256sum --check -
export XENO_DATA_DIR="$PWD/disc"
export XENO_SLUS="$PWD/disc/SLUS_006.64"
export XENO_RETAIL_DATA_OPTIONAL=0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/w34n128.XXXXXX")
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
build swapped -DW34N128_SWAP_RENDERER_RESULTS
status=0
"$OUT/swapped" > "$OUT/swapped.log" 2>&1 || status=$?
if [[ $status != 1 ]] || ! grep -Fq 'W34N128 FAIL angle=' "$OUT/swapped.log"; then
    echo "W34N128 FAIL: swapped sine/cosine did not trigger a comparison failure" >&2
    exit 1
fi
echo "W34N128 PASS: swapped renderer sine/cosine results were detected"
