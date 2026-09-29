#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC="$ROOT/src/slus_006.64/system/system.c"
ADAPTER="$ROOT/pc_port/src/battle_mips_adapter.c"
TEST="$SCRIPT_DIR/glyph_raster_retail_test.c"
DISC="$ROOT/disc/SLUS_006.64"

if [[ -n "${GLYPH_RASTER_TEST_OUT:-}" ]]; then
    OUT="$GLYPH_RASTER_TEST_OUT"
    mkdir -p "$OUT"
else
    OUT="$(mktemp -d "${TMPDIR:-/tmp}/xeno-glyph-raster-retail.XXXXXX")"
fi

: > "$OUT/mutants.log"
cd "$ROOT"

RETAIL="$OUT/retail-glyph.bin"
dd if="$DISC" bs=1 skip=$((0x80034ffc - 0x8000f800)) count=1696 \
    of="$RETAIL" status=none

python3 - "$DISC" "$RETAIL" >"$OUT/retail-verification.txt" <<'PY'
import hashlib
import pathlib
import sys

slus, slice_path = map(pathlib.Path, sys.argv[1:])
full_expected = "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
slice_expected = "3591ad76c469c1d359529e264ad5aa81c0f1814327c68307c0ddd69e41e91a34"
full = slus.read_bytes()
blob = slice_path.read_bytes()
assert hashlib.sha256(full).hexdigest() == full_expected
assert len(blob) == 1696
assert hashlib.sha256(blob).hexdigest() == slice_expected
print("FULL_SLUS_SHA256 PASS", full_expected)
print("RETAIL_SLICE_SHA256 PASS", slice_expected)
PY

{
    echo "output=$OUT"
    date -u +%Y-%m-%dT%H:%M:%SZ
    sha256sum "$DISC" "$RETAIL" \
        "$ROOT/pc_port/src/battle_mips_adapter.c" \
        "$ROOT/pc_port/src/battle_mips_adapter.h" "$SRC" "$TEST" \
        "$0"
    clang --version
    (ld.lld --version 2>/dev/null || true)
    (rg --version 2>/dev/null || true)
} > "$OUT/manifest.txt"

COMMON=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -fno-pie -fno-builtin
    -ffunction-sections -fdata-sections -include assert.h -include stdint.h
    -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-macro-redefined
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

build_run() {
    local name="$1"
    local -a flags=(-O0)
    local -a sanitizer_flags=()
    if [[ "$name" == O2* ]]; then flags=(-O2); fi
    if [[ "$name" == UBSan* ]]; then
        flags=(-O1)
        sanitizer_flags=(-fsanitize=undefined -fno-sanitize-recover=all)
    fi
    clang "${COMMON[@]}" "${flags[@]}" "${sanitizer_flags[@]}" -w \
        -c "$SRC" -o "$OUT/$name.system.o"
    clang "${COMMON[@]}" "${flags[@]}" "${sanitizer_flags[@]}" -Wall -Wextra -Werror \
        -c "$TEST" -o "$OUT/$name.test.o"
    clang "${COMMON[@]}" "${flags[@]}" "${sanitizer_flags[@]}" -Wall -Wextra -Werror \
        -c "$ADAPTER" -o "$OUT/$name.adapter.o"
    clang -no-pie "${flags[@]}" "${sanitizer_flags[@]}" -Wl,--gc-sections \
        "$OUT/$name.system.o" "$OUT/$name.test.o" "$OUT/$name.adapter.o" \
        -o "$OUT/$name"
    "$OUT/$name" "$RETAIL" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
}

build_run O0
build_run O2
build_run UBSan

for name in O0 O2 UBSan; do
    rg -q '^GLYPH RETAIL DIFFERENTIAL PASS cases=144 ' "$OUT/$name.stdout"
    test ! -s "$OUT/$name.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"

for mutant in WM_34FFC_MUTANT_REVERSE_ROW0_OUTLINE WM_34FFC_MUTANT_REVERSE_ROW1_OUTLINE; do
    name="mutant-${mutant#WM_34FFC_MUTANT_REVERSE_}"
    clang "${COMMON[@]}" -O0 -D"$mutant" -w -c "$SRC" \
        -o "$OUT/$name.system.o"
    clang "${COMMON[@]}" -O0 -Wall -Wextra -Werror -c "$TEST" \
        -o "$OUT/$name.test.o"
    clang "${COMMON[@]}" -O0 -Wall -Wextra -Werror -c "$ADAPTER" \
        -o "$OUT/$name.adapter.o"
    clang -no-pie -O0 -Wl,--gc-sections "$OUT/$name.system.o" \
        "$OUT/$name.test.o" "$OUT/$name.adapter.o" -o "$OUT/$name"
    set +e
    "$OUT/$name" "$RETAIL" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^DIFF case=' "$OUT/$name.stderr"; then
        echo "MUTANT SURVIVED: $mutant rc=$rc" >&2
        exit 1
    fi
    echo "MUTANT DETECTED: $mutant" >> "$OUT/mutants.log"
done

echo 'GLYPH RETAIL DIFFERENTIAL CERTIFICATE PASS; row0/row1 mutants rejected'
echo "GLYPH_RASTER_TEST_OUT=$OUT"
