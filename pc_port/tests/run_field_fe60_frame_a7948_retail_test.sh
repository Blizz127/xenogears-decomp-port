#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_FE60_FRAME_A7948_BUILD_DIR:-$ROOT/pc_port/build_native/field_fe60_frame_a7948}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
FN_SHA="3fe7fe87eafd03df1f72192a78593df5791a5330691f0df2c2abcc02ab1b0abf"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x800A7948 - 0x8006FAF0)) \
    count=$((0x310)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$FN_SHA" ]]; then
    echo "ERROR: retail func_800A7948 slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

compile_fixture() {
    local name="$1"
    local source="$2"
    local linker="$CC"
    shift 2
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline "$@" \
        -iquote "$ROOT/src/field/main" -c "$source" -o "$BUILD_DIR/$name.misc5.o"
    objcopy --weaken-symbol=func_800A732C \
            "$BUILD_DIR/$name.misc5.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_fe60_frame_a7948_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.misc5.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name"
}

build_and_run() {
    local name="$1"
    shift
    compile_fixture "$name" src/field/main/misc5.c "$@"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^FIELD FE60 FRAME A7948 certificate PASS checks=43$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD FE60 FRAME A7948 O0/O2/UBSAN PASS"

mutation_dir="$(mktemp -d "$BUILD_DIR/keep-wide.XXXXXX")"
trap 'rm -rf -- "$mutation_dir"' EXIT
cp src/field/main/misc5.c "$mutation_dir/misc5.c"
python3 - src/field/main/field_movie_wide_frame.inc "$mutation_dir/field_movie_wide_frame.inc" <<'PY'
from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:])
restore = "PcPort_SetFieldMovieWidth(320, 1);"
body = source.read_text()
if body.count(restore) != 1:
    raise SystemExit("Expected one field-movie width restoration")
destination.write_text(body.replace(restore, "PcPort_SetFieldMovieWidth(640, 1);"))
PY

compile_fixture mutant_keep_wide "$mutation_dir/misc5.c" -O0 -g
mutant_rc=0
"$BUILD_DIR/mutant_keep_wide" >"$BUILD_DIR/mutant_keep_wide.stdout" \
    2>"$BUILD_DIR/mutant_keep_wide.stderr" || mutant_rc=$?
if [[ "$mutant_rc" != 1 ]] || \
   ! rg -qx 'ASSERTION fe60.frame field=run.draw2.w actual=640 expected=320' \
       "$BUILD_DIR/mutant_keep_wide.stderr"; then
    echo "KEEP-WIDE MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_keep_wide.stderr" >&2 || true
    exit 1
fi

echo "FIELD FE60 FRAME A7948 MUTANT DETECTED"
