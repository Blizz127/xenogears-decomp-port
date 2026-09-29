#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/battle_heap_reservation_test
mkdir -p "$OUT"
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c src/slus_006.64/system/temp3.c -o "$OUT/$opt.prod.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/battle_heap_reservation_test.c -o "$OUT/$opt.test.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.prod.o" "$OUT/$opt.test.o" -o "$OUT/$opt"
    "$OUT/$opt"
done
retail_sha="$(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8001bbac - 0x8000f800)) count=$((0x194)) status=none | sha256sum | cut -d ' ' -f 1)"
test "$retail_sha" = 2a5926e7e942988d6311c01c77e970dc1a133808f2f8b878b5f41ac797d9dca5
echo "BATTLE AUDIO LOADER RETAIL SLICE PASS"
python3 - <<'PY'
import runpy
generator = runpy.run_path("tools/scripts/gen_battle_bridge_map.py")
rows = generator["parse_symbols"](["config/symbol_addrs.slus_006.64.txt"], set())
assert (0x800595a8, "D_800595A8", 4, False) in rows, "missing exact second-buffer guest/native binding"
print("BATTLE SECOND AUDIO BUFFER BRIDGE MAP PASS")
PY
