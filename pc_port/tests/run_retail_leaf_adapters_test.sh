#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t retail-leaf-adapters.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx)
BASE=(-std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C -fno-pie -no-pie \
      -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -Wall -Wextra \
      -ffunction-sections -fdata-sections)

for opt in O0 O2; do
    flag="-${opt}"
    g++ -std=c++17 "$flag" -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -ffunction-sections -fdata-sections -fpermissive -w "${INC[@]}" \
        -include pc_port/src/port_compat.h -c pc_port/extern/PsyCross/src/psx/LIBGTE.C \
        -o "$OUT/$opt.trig.o"
    gcc "${BASE[@]}" "$flag" "${INC[@]}" \
        pc_port/src/retail_leaf_adapters.c \
        pc_port/tests/retail_leaf_adapters_test.c "$OUT/$opt.trig.o" \
        -Wl,--gc-sections -o "$OUT/test-$opt"
    if ! "$OUT/test-$opt" >"$OUT/$opt.log" 2>&1; then
        cat "$OUT/$opt.log" >&2
        exit 1
    fi
done
cmp "$OUT/O0.log" "$OUT/O2.log"

g++ -std=c++17 -O1 -fsanitize=undefined -fno-sanitize-recover=all \
    -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections \
    -fpermissive -w "${INC[@]}" -include pc_port/src/port_compat.h \
    -c pc_port/extern/PsyCross/src/psx/LIBGTE.C -o "$OUT/UBSan.trig.o"
clang "${BASE[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all \
    "${INC[@]}" \
    pc_port/src/retail_leaf_adapters.c \
    pc_port/tests/retail_leaf_adapters_test.c "$OUT/UBSan.trig.o" \
    -Wl,--gc-sections -o "$OUT/test-ubsan"
"$OUT/test-ubsan" >"$OUT/ubsan.log" 2>&1
cmp "$OUT/O0.log" "$OUT/ubsan.log"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path

payload = Path("disc/SLUS_006.64").read_bytes()
base = 0x80010000
file_base = 0x800
expected = {
    (0x80043D78, 0x80043D8C): "3c2c2f96ba065c302ab697a498aaae48bf32c10c2b8f27bbe21cf3e5c15ebef6",
    (0x8004A0DC, 0x8004A0E8): "4ecf2f5bbdb450de400e5eeafac260aaf27c3f5d36b3f67ef8ceb47855a2e800",
    (0x8003633C, 0x8003634C): "05cfa6b5feb5026d486bd3c3d114b5e4e97709bc58a6136b364c027bdaf03f3f",
    (0x800379B4, 0x800379C8): "3d605a8b1358817bf8cc409053c61d4e28f5a7f4a1f1742ec729db876d432b3c",
    (0x800379C8, 0x800379D0): "bb07e09aa0b46cd47bc742347dfbe6819313de64f21a7e578daf55b4b548e961",
    (0x80026F44, 0x80026FE8): "0f39286a441c9ff365837b8e101e26b547e40d0a27ba8368910241d1500d6a87",
    (0x80043C60, 0x80043C74): "ba11f573fe5aa96ec29b9dbf7084639ca609d934650321c168a470715bd2edde",
    (0x8004ABBC, 0x8004AE4C): "2bae58e17627d27d6d995c28d3a47370f8ea840db2d84014896432edf069cd79",
}
for (start, end), digest in expected.items():
    begin = file_base + start - base
    actual = sha256(payload[begin:begin + end - start]).hexdigest()
    assert actual == digest, (hex(start), actual, digest)
print("RETAIL_LEAF_ADAPTERS RETAIL_SLICES PASS")
PY

cat "$OUT/O0.log"
