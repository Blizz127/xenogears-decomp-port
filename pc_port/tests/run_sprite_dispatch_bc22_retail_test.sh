#!/usr/bin/env bash
# Opcode 0xBC sub-command 0x22 regression: retail-backed differential for the
# new player-relative body in func_8001FBE4 (src/slus_006.64/system/
# animation_scripts.c), plus deliberate mutants the test must reject.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_bc22_retail_test
mkdir -p "$OUT"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import re

SLUS = Path('disc/SLUS_006.64').read_bytes()
assert sha256(SLUS).hexdigest() == \
    'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119', \
    'disc/SLUS_006.64 changed'
BASE = 0x8000F800

def sl(start, end):
    return SLUS[start - BASE:end - BASE]

# Pin the dispatch table and every byte the 0x22 path executes.
pins = {
    'jtbl_800185A8': (0x800185A8, 0x80018644,
                      '1fe2582511b7ce9ae1f0c3c58f3a645b6e90b3c6d2d8a00d0c0b0290a3034f89'),
    'sub22_entry': (0x8002049C, 0x800204AC,
                    '9e5a3f3b88ff7a01be630654d1d5696158994e5c6e900b04bef7c51cc5116bb6'),
    'sub22_body': (0x80020550, 0x8002058C,
                   '328a22fc1eb45c249f53b3d4ba9adfe5d7206b4f0e6b274e4189ba4cab471a9c'),
    'bc_handler': (0x800202F4, 0x80020BAC,
                   '74fc07f0212c03d5b342ac956e17e93f21d4240ad02d76bfe5513297d706dd07'),
}
for name, (start, end, digest) in pins.items():
    actual = sha256(sl(start, end)).hexdigest()
    assert actual == digest, f'{name} slice hash changed: {actual}'
    print(f'SPRITE BC22 pin {name} {start:#x}..{end:#x} {actual}')

# The jump table entry and the two visited basic blocks decode exactly.
assert int.from_bytes(sl(0x800185A8 + 0x22 * 4, 0x800185A8 + 0x23 * 4), 'little') \
    == 0x8002049C
words = [int.from_bytes(sl(0x8002049C + i * 4, 0x8002049C + i * 4 + 4), 'little')
         for i in range(4)]
assert words == [0x3C10800C, 0x8E103E1C, 0x08008154, 0x00000000], \
    [hex(w) for w in words]

# The annotated matching listing must agree with the disc byte-for-byte.
rows = re.findall(r'/\* ([0-9A-F]+) ([0-9A-F]+) ([0-9A-F]+) \*/',
                  Path('asm/slus_006.64/matchings/system/animation_scripts/'
                       'func_8001FBE4.s').read_text())
checked = 0
for _, address, word in rows:
    addr = int(address, 16)
    if 0x8002049C <= addr < 0x800204AC or 0x80020550 <= addr < 0x8002058C:
        off = addr - BASE
        assert SLUS[off:off + 4] == bytes.fromhex(word), f'{address} mismatch'
        checked += 1
assert checked == 4 + 15, f'expected 19 annotated sub-0x22 instructions, got {checked}'
print(f'SPRITE BC22 {checked} annotated sub-0x22 instruction bytes match disc')
PY

COMMON=(-std=gnu17 -fpermissive -include stdint.h -D_LANGUAGE_C -fno-pie
    -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM
    -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
    -Ipc_port/extern/PsyCross/include/psx)

# GCC on this host can be built without the UBSan runtime; probe the link and
# fall back to clang explicitly rather than skipping the regime.
UBSAN_CC=gcc
if ! printf 'int main(void){return 0;}\n' | gcc -x c - -fsanitize=undefined \
        -o "$OUT/ubsan_probe" >/dev/null 2>&1; then
    UBSAN_CC=clang
fi
rm -f "$OUT/ubsan_probe"
echo "SPRITE BC22 UBSan regime: $UBSAN_CC"

build_body() {
    local opt=$1 src=$2
    local flags
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    else
        flags=(-"$opt")
    fi
    # The game TU uses K&R implicit declarations, so it is always compiled by
    # gcc; UBSan instrumentation links under clang when this host's gcc cannot
    # provide the runtime (probed above).
    gcc "${COMMON[@]}" "${flags[@]}" -c "$src" -o "$OUT/$opt.body.o"
    gcc -std=gnu17 "${flags[@]}" \
        -Dmain=noop_main -DApplyMatrixSV=ApplyMatrixSV_guard \
        -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c \
        -o "$OUT/$opt.guards.o"
    local link=gcc
    if [ "$opt" = UBSan ]; then link=$UBSAN_CC; fi
    local clangonly=()
    if [ "$link" = clang ]; then clangonly=(-Wno-unknown-warning-option); fi
    "$link" -std=gnu17 -Wall -Wextra -Werror -Ipc_port/src -Ipc_port/include_shim \
        -Iinclude -no-pie "${flags[@]}" "${clangonly[@]}" -Wl,--gc-sections \
        pc_port/tests/sprite_dispatch_bc22_retail_test.c \
        pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" \
        -o "$OUT/$opt.test"
}

for opt in O0 O2 UBSan; do
    build_body "$opt" src/slus_006.64/system/animation_scripts.c
    "$OUT/$opt.test"
done
echo 'SPRITE BC22 O0/O2/UBSAN PASS'

python3 - <<'PY'
from pathlib import Path
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
mutations = {
    # Wrong +0x38 source field on the player.
    'height_off': ('*(u16*)(pPlayer + 0x38)', '*(u16*)(pPlayer + 0x36)'),
    # Half the height instead of the full +0x38 (that would be sub 0x15).
    'half': ('(s32)*(u16*)(pPlayer + 0x38)',
             '(s32)(*(u16*)(pPlayer + 0x38) >> 1)'),
    # Add the height instead of subtracting it.
    'add_height': ('- (s32)*(u16*)(pPlayer + 0x38)',
                   '+ (s32)*(u16*)(pPlayer + 0x38)'),
    # Wrong X halfword.
    'x_off': ('*(s16*)(pPlayer + 0x2)', '*(s16*)(pPlayer + 0x0)'),
    # Wrong Y halfword.
    'y_off': ('*(s16*)(pPlayer + 0x6)', '*(s16*)(pPlayer + 0x4)'),
    # Flip the preserved camera-relative gate.
    'camera_flip': ('if (cameraRelative != 0) {', 'if (cameraRelative == 0) {'),
    # Drop native host-pointer support in the player lookup.
    'host_ptr': ('return (u8*)PSX_ADDR(address);\n    }\n'
                 '    return (u8*)(uintptr_t)address;',
                 'return (u8*)PSX_ADDR(address);\n    }\n'
                 '    return (u8*)PSX_ADDR(address);'),
}
for name, (old, new) in mutations.items():
    assert source.count(old) == 1, f'mutant {name}: {source.count(old)} matches'
    Path(f'pc_port/build_native/sprite_dispatch_bc22_retail_test/{name}.c') \
        .write_text(source.replace(old, new))
PY
for mutant in height_off half add_height x_off y_off camera_flip host_ptr; do
    build_body O2 "$OUT/$mutant.c"
    if "$OUT/O2.test" > "$OUT/$mutant.log" 2>&1; then
        echo "BC22 MUTANT SURVIVED $mutant"
        exit 1
    fi
    if ! grep -q 'SPRITE BC22 FAIL' "$OUT/$mutant.log"; then
        echo "BC22 MUTANT REJECTED WITHOUT DIAGNOSTIC $mutant"
        cat "$OUT/$mutant.log" >&2
        exit 1
    fi
    echo "BC22 MUTANT REJECTED $mutant"
done
echo 'SPRITE BC22 MUTANTS PASS'
