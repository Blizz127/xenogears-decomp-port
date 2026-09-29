#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/track_decoder_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x1bf8:0x2f10]).hexdigest()=='d17b35c9d57add1f9f53f67d2f9b7a0cb7da37cce10542c2d3b545430308a5d8'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/track_decoder_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in rotation translation scaling accumulate stride order exception; do
    owner='u32 func_801DDBF8'
    case "$mutant" in
        rotation) expression='/status = FieldDecodeRotationTrack/d' ;;
        translation) expression='/status = FieldDecodeTranslationTrack/d' ;;
        scaling) expression='/status = FieldDecodeScaleTrack/d' ;;
        accumulate) expression='s/eventTag, status/eventTag, 0/g' ;;
        stride) expression='s/node += OVLY_NODE_STRIDE/node += 0/' ;;
        order) expression='s/FieldDecodeRotationTrack/FieldDecodeSwapTrack/g; s/FieldDecodeScaleTrack/FieldDecodeRotationTrack/g; s/FieldDecodeSwapTrack/FieldDecodeScaleTrack/g' ;;
        exception) owner='static s32 FieldTrackDivide'; expression='s/__builtin_trap()/return 0/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/track_decoder_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "TRACK DECODER mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'TRACK DECODER FAIL' "$OUT/$mutant.log"
done
echo "TRACK DECODER negative controls PASS: rotation translation scaling accumulate stride order exception"
