#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${SHOP_C5CBC_BUILD_DIR:-$ROOT/pc_port/build_native/shop_text_pair_c5cbc}"
CC="${CC:-gcc}"
mkdir -p "$OUT"
cd "$ROOT"
export TMPDIR="${TMPDIR:-/var/tmp}"

SHOP_SHA="7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf"
SLICE_SHA="fb0b13308d4021fd68ef69849e8f9f8f9ff0dd6b5874903fdaba38249cad68cd"
test "$(sha256sum disc/shop_menu.bin | awk '{print $1}')" = "$SHOP_SHA"
test "$(dd if=disc/shop_menu.bin bs=1 skip=$((0xCBC)) count=$((0x1B0)) status=none | sha256sum | awk '{print $1}')" = "$SLICE_SHA"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
TEST_BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
           -DXENO_PC_PORT -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
           -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1" source="src/shop_menu/main/misc2.c"
    shift
    if [[ $# -gt 0 && -f "$1" ]]; then source="$1"; shift; fi
    "$CC" "${BASE[@]}" "${INC[@]}" -w "$@" -c "$source" -o "$OUT/$name.shop.o"
    "$CC" "${TEST_BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" -c pc_port/tests/shop_text_pair_c5cbc_retail_test.c -o "$OUT/$name.test.o"
    "$CC" -fno-pie -no-pie "$@" "$OUT/$name.test.o" "$OUT/$name.shop.o" -Wl,--gc-sections -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    test ! -s "$OUT/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
grep -Eq '^SHOP TEXT PAIR C5CBC certificate PASS checks=[0-9]+$' "$OUT/O0.stdout"
echo 'SHOP TEXT PAIR C5CBC O0/O2/UBSAN PASS'

set +e
sed '/^void func_801C5CBC(/,/^}/ s/stringIds\[i\]/stringIds[i + 1]/' src/shop_menu/main/misc2.c >"$OUT/mutant.c"
build_and_run mutant "$OUT/mutant.c" -O0 -g
rc=$?
set -e
if [[ "$rc" -eq 0 ]] || ! grep -Eq '^SHOP C5CBC FAIL' "$OUT/mutant.stderr"; then
    echo "C5CBC STRING-ID MUTANT NOT DETECTED rc=$rc" >&2
    exit 1
fi
echo 'SHOP TEXT PAIR C5CBC MUTANT DETECTED'

# Cross the real initializer/builder boundary: a matching producer/consumer
# fixture alone previously hid the 32-bit-offset versus native-layout bug.
for mutation in allocation stride rectangle; do
    python3 - "$mutation" "$OUT/$mutation.c" <<'PY'
import sys
from pathlib import Path
s = Path('src/shop_menu/main/misc2.c').read_text()
a, b = {
    'allocation': ('g_Menu->unk4E0[0].pVramBuffer = HeapAlloc', '*(void**)((u8*)g_Menu + 0x558) = HeapAlloc'),
    'stride': ('&strings[i + 1]', '(MenuString*)((u8*)strings + (i + 1) * 0x80)'),
    'rectangle': ('first->vramDest.w = 0x1C;', 'first->vramDest.w = 0;'),
}[sys.argv[1]]
assert a in s
Path(sys.argv[2]).write_text(s.replace(a, b))
PY
    set +e
    build_and_run "$mutation" "$OUT/$mutation.c" -O0 -g
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! grep -q '^SHOP C5CBC FAIL' "$OUT/$mutation.stderr"; then
        echo "C5CBC $mutation MUTANT NOT DETECTED rc=$rc" >&2
        exit 1
    fi
    echo "SHOP TEXT PAIR C5CBC $mutation MUTANT DETECTED"
done
