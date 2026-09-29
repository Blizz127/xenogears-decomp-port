#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/timed_script_setup_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x9c74:0x9cd8]).hexdigest()=='aa7406e2a5c218eda80812514fa67b6fad6eb6f82f1325fcc91140decdf6fb09'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/timed_script_setup_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in inactive loop count offset pointer load_alias; do
 case "$mutant" in
  inactive) expression='s/(object + 0x98) = 0xFFFF/(object + 0x98) = 0/' ;;
  loop) expression='s/loop != 0/loop == 1/' ;;
  count) expression='s/(object + 0x9E) = \*(u16\*)(data + 0x12)/(object + 0x9E) = *(u16*)(data + 2)/' ;;
  offset) expression='s/data + 0x14/data + 0x10/' ;;
  pointer) expression='s/memcpy(object + 0xA4, \&stream, sizeof(stream));/(void)stream;/' ;;
  load_alias) expression='s/memcpy(\&stream, data + 0x14, sizeof(stream));/stream = *(u32*)(data + 0x14);/' ;;
 esac
 sed "/^void func_801E5C74(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/timed_script_setup_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "TIMED SCRIPT SETUP mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'TIMED SCRIPT SETUP FAIL' "$OUT/$mutant.log"
done
echo 'TIMED SCRIPT SETUP negative controls PASS: inactive loop count offset pointer load_alias'
