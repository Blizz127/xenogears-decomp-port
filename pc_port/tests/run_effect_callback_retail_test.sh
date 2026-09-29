#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/effect_callback_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x74bc:0x7534]).hexdigest()=='2fb20201f7d265633f6235f707532ae68c58acc866fab5c437170cef5753f73b'
for lo,hi,digest in [
    (0x4850,0x48d4,'ebff99d63bd52cb38f17d8f3e9525ccc5737ce288099b65b0f5acb00ea798a01'),
    (0x48d4,0x4938,'f6a61aee076e45313301919d85d50f34e2e97d0864782fef87c76e87fea0b860'),
    (0x4938,0x4988,'e43f3ce8f4cdf522d12cbcdba2f1f5440b629e8b87b0dd0e38fa1d7e032e5f72'),
    (0x4988,0x4a00,'92002d1b6fe561eb31a285fb7e618dddb64e7bdf59cad9d7b630d4ca24eefe9b')]:
    assert sha256(b[lo:hi]).hexdigest()==digest
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/src/field_object_overlay.c -o "$OUT/$opt.owner.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/effect_callback_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.owner.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in phase trig boundary direction lower selector divide; do
    owner='s32 func_801E08D4'
    case "$mutant" in
        phase) expression='s/(s16)phase/(u16)phase/' ;;
        trig) owner='s32 func_801E0850'; expression='s/4096u/4095u/' ;;
        boundary) expression='s/value > 32/value >= 32/' ;;
        direction) owner='s32 func_801E0938'; expression='s/(u32)offset - (u32)quotient/(u32)offset + (u32)quotient/' ;;
        lower) owner='s32 func_801E0988'; expression='s/value < (s16)offset/value > (s16)offset/' ;;
        selector) owner='u32 func_801E34BC'; expression='s/default: return 0x801E0850/default: return 0x801E08D4/' ;;
        divide) expression='s/(s16)divisor/(s16)divisor == 0 ? 1 : (s16)divisor/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "EFFECT CALLBACK mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'EFFECT CALLBACK FAIL' "$OUT/$mutant.log"
done
echo "EFFECT CALLBACK negative controls PASS: phase trig boundary direction lower selector divide"
