#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/object_initializer_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xb38c:0xb42c]).hexdigest()=='5287e22284932db7e892ba7743bf4f2de91317d355b3392ec5aa96a27395f728'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/object_initializer_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in capacity aux registry effects timing phase slots; do
    case "$mutant" in
        capacity) expression='s/D_801E86A8, arg0/D_801E86A8, 64/' ;;
        aux) expression='/func_801E0064(D_801E86A0, 16);/d' ;;
        registry) expression='s/D_801E85F4\[i\]\[0\] = 0/D_801E85F4[i][0] = D_801E85F4[i][1] = 0/' ;;
        effects) expression='s/D_801E8648\[13\] = 0/memset(D_801E8648, 0, sizeof(D_801E8648))/' ;;
        timing) expression='/D_801E8640 = 0;/d' ;;
        phase) expression='/D_801E869C = 0;/d' ;;
        slots) expression='s/i = 9/i = 8/' ;;
    esac
    sed "/^void func_801E738C(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/object_initializer_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OBJECT INIT mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OBJECT INIT FAIL' "$OUT/$mutant.log"
done
echo "OBJECT INIT negative controls PASS: capacity aux registry effects timing phase slots"
