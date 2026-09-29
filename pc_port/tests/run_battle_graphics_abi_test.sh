#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/battle_graphics_abi_test
mkdir -p "$OUT"
read -r sprite_hash _ < <(dd if=disc/battle.bin bs=1 \
    skip=$((0x800c11cc - 0x8006faf0)) count=$((0xe80)) status=none | sha256sum)
test "$sprite_hash" = f7a1f233c785d48e5815a92abf5edc576fa02d7a0c4037d5a75067002649841d
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c pc_port/src/psyq_compat.c -o "$OUT/$opt.compat.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/battle_graphics_abi_test.c -o "$OUT/$opt.test.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$opt.vblank.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/god_mode.c -o "$OUT/$opt.god.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.vblank.o" "$OUT/$opt.compat.o" "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" "$OUT/$opt.god.o" -ldl -o "$OUT/$opt"
    "$OUT/$opt"
done
# A no-op at the native-to-retail boundary must fail the real opcode test.
sed '/^void func_800C11CC(void\* sprite)/,/^}/c\void func_800C11CC(void* sprite) { (void)sprite; }' \
    pc_port/src/battle_mips_runtime.c > "$OUT/noop-runtime.c"
sed "s|../src/battle_mips_runtime.c|$PWD/$OUT/noop-runtime.c|" \
    pc_port/tests/battle_graphics_abi_test.c > "$OUT/noop-test.c"
gcc "${COMMON[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/noop-test.c" -o "$OUT/noop-test.o"
clang -no-pie -Wl,--gc-sections "$OUT/O2.vblank.o" "$OUT/O2.compat.o" "$OUT/noop-test.o" \
    "$OUT/O2.cpu.o" "$OUT/O2.god.o" -ldl -o "$OUT/noop-test"
if "$OUT/noop-test" > "$OUT/noop-test.log" 2>&1; then
    echo 'BATTLE SPRITE REENTRY FAIL no-op mutant survived' >&2
    exit 1
fi
rg -q 'BATTLE SPRITE REENTRY FAIL case=1' "$OUT/noop-test.log"
echo 'BATTLE SPRITE REENTRY no-op mutant rejected'
python3 - <<'PY'
import hashlib
from pathlib import Path
image = Path("disc/SLUS_006.64").read_bytes()
for start, end, expected in [
    (0x80047518, 0x80047638, "724e45f2d0b641872e2f981d218330d73335d8996b36040a56e67c2fa07d6ba3"),
    (0x8004a0bc, 0x8004a0dc, "2f9e759091975bad3585277ae26c3eb50addd764857351a5213c14fb2c857a89"),
    (0x8004a67c, 0x8004a6d0, "6dd2d4421dbd97f4d19df2c97fcc9c62ae767fbc5ffe3650e46470f37489e9e3"),
    (0x8004a64c, 0x8004a67c, "a7543a7fc880359e87aa85722d3155a9ec4ba246a771a939b181225039e14c49"),
    (0x8004a73c, 0x8004a7bc, "1f11e4a24b15b89878f31d5d558c0b8131cb7bbeb9c81a43b1c8d9c9f54f5dd3"),
    (0x80044ad8, 0x80044b70, "1be1dff52b237617f48f8a1ba2a420a1e17c7e3f22904345ff9a3ff732285a60"),
    (0x8005698c, 0x800569a0, "80f8aa953cca92e52e8d1f8e11f0634ebe78678a342953735ee44a7b5f87a792"),
]:
    actual = hashlib.sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, (hex(start), actual, expected)
print("BATTLE GRAPHICS ABI RETAIL SLICES PASS")
PY

# Guest code addresses must remain callback identities at the lookup boundary.
sed 's/index == 0 && strcmp(name, "func_8001D164")/index == 1 \&\& strcmp(name, "func_8001D164")/' \
    pc_port/src/battle_mips_runtime.c > "$OUT/callback-mutant-runtime.c"
sed "s|../src/battle_mips_runtime.c|$PWD/$OUT/callback-mutant-runtime.c|" \
    pc_port/tests/battle_graphics_abi_test.c > "$OUT/callback-mutant-test.c"
gcc "${COMMON[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/callback-mutant-test.c" -o "$OUT/callback-mutant-test.o"
clang -no-pie -Wl,--gc-sections "$OUT/O2.vblank.o" "$OUT/O2.compat.o" "$OUT/callback-mutant-test.o" \
    "$OUT/O2.cpu.o" "$OUT/O2.god.o" -ldl -o "$OUT/callback-mutant-test"
if "$OUT/callback-mutant-test" > "$OUT/callback-mutant-test.log" 2>&1; then
    echo 'TIMER CALLBACK LOOKUP ABI FAIL translation mutant survived' >&2
    exit 1
fi
rg -q 'TIMER CALLBACK LOOKUP ABI FAIL callback=800bcbb4' "$OUT/callback-mutant-test.log"
echo 'TIMER CALLBACK LOOKUP ABI translation mutant rejected'
