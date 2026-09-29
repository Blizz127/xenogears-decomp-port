#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/battle_asset_loader_test
mkdir -p "$OUT"
read -r loader_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800379d8 - 0x8000f800)) \
    count=$((0x1b0)) status=none | sha256sum)
read -r caller_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8001bb0c - 0x8000f800)) \
    count=$((0x44)) status=none | sha256sum)
if [ "$loader_sha" != 0ab73e7e3353da11925b9db4700117e358e0347fb55fb60a6c111ae6e446ab98 ] ||
   [ "$caller_sha" != da393db3491f47b18db8a84fef891c641551670955975381b634b334d015a128 ]; then
    echo "BATTLE ASSET RETAIL SLICE MISMATCH" >&2
    exit 1
fi
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c src/slus_006.64/system/temp3.c -o "$OUT/$opt.caller.o"
    gcc "${COMMON[@]}" "${flags[@]}" -DTEST_WRAPPER_ONLY -Wall -Wextra -Werror \
        -c pc_port/tests/battle_asset_loader_test.c -o "$OUT/$opt.wrapper_test.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.caller.o" "$OUT/$opt.wrapper_test.o" -o "$OUT/$opt.wrapper"
    "$OUT/$opt.wrapper"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c src/slus_006.64/system/asset_loader.c -o "$OUT/$opt.loader.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_asset_loader_test.c -o "$OUT/$opt.loader_test.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.loader.o" \
        "$OUT/$opt.loader_test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.loader"
    "$OUT/$opt.loader"
done
python3 - <<'PY'
import runpy
generator = runpy.run_path("tools/scripts/gen_battle_bridge_map.py")
rows = generator["parse_symbols"](["config/symbol_addrs.slus_006.64.txt"], set())
for address in (0x80059470, 0x8005949c, 0x80059520, 0x800658c8):
    assert (address, f"D_{address:08X}", 4, False) in rows
print("BATTLE ASSET POINTER BRIDGE MAP PASS")
PY
