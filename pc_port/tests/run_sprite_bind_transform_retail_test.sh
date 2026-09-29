#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/sprite_bind_transform_retail_test.XXXXXXXX)
export OUT
echo "TRANSFORM artifacts: $OUT"

python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, sys
out=Path(sys.argv[1])
slus=Path('disc/SLUS_006.64').read_bytes()
assert sha256(slus).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
base=0x8000f800
start,end=0x80023538,0x80023804
retail=slus[start-base:end-base]
assert len(retail)==end-start
assert sha256(retail).hexdigest()=='580e480f78964895cbce1b6340f8fd8f9961eaae017cb5a1e1f38fd090e5f655'
(out/'retail_23538.bin').write_bytes(retail)
native_path=Path(os.environ.get('TRANSFORM_NATIVE_SOURCE',
                                'src/slus_006.64/system/temp1.c'))
source=native_path.read_text()
sig='void func_80023538(void* pSpriteData, void* pAnimation) {'
begin=source.index(sig)
finish=source.index('\n}',begin)+2
body=source[begin:finish]
header='''#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef uint32_t u32; typedef int32_t s32;
typedef void SpriteData;
extern s32 D_800591A8; extern s32 D_80059198; extern u8 D_800591AD;
extern void SpriteComputeTransformMatrix(void*);
extern void SpriteSetScale(void*, short);
extern void func_800234AC(void*);
'''
(out/'native.c').write_text(header+body+'\n')
paths=[str(native_path),'pc_port/src/battle_mips_adapter.c',
 'pc_port/tests/sprite_bind_transform_retail_test.c',
 'pc_port/tests/run_sprite_bind_transform_retail_test.sh']
pins={'SLUS_006.64':sha256(slus).hexdigest(),
 'retail_23538':{'start':hex(start),'end':hex(end),'sha256':sha256(retail).hexdigest()},
 'native_function_source_sha256':sha256(body.encode()).hexdigest(),
 'source_files':{p:sha256(Path(p).read_bytes()).hexdigest() for p in paths},
 'scope':'Focused func_80023538 helper boundary: actual retail Scale/Compute calls, order, arguments and controlled marker effects; safe denominator/coefficients, no full matrix algorithm parity.'}
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
PY

# The exact retail function slice is pinned above; retain this explicit check in
# the log rather than accepting an unverified broad image.
python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys
actual=sha256((Path(sys.argv[1])/'retail_23538.bin').read_bytes()).hexdigest()
print('TRANSFORM retail slice sha256='+actual)
PY

common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT
    -DSKIP_ASM -D_LANGUAGE_C -include assert.h -I"$OUT" -Ipc_port/src
    -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
    -Ipc_port/extern/PsyCross/include/psx)

build_case() {
    local name=$1 opt=$2 source=${3:-pc_port/tests/sprite_bind_transform_retail_test.c}
    local native_source=${4:-$OUT/native.c}
    local -a flags
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$source" -o "$OUT/$name.test.o" > "$OUT/$name.build.log" 2>&1
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$name.cpu.o" >> "$OUT/$name.build.log" 2>&1
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$native_source" -o "$OUT/$name.native.o" >> "$OUT/$name.build.log" 2>&1
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$name.test.o" "$OUT/$name.cpu.o" "$OUT/$name.native.o" -o "$OUT/$name.test" >> "$OUT/$name.build.log" 2>&1
}

verify_source_pins() {
    python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins=json.loads((Path(os.environ['OUT'])/'provenance.json').read_text())
for path,expected in pins['source_files'].items():
    assert sha256(Path(path).read_bytes()).hexdigest()==expected, 'Source changed during test: '+path
PY
}

red=0
for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then cat "$OUT/$opt.build.log" >&2; echo "TRANSFORM INFRASTRUCTURE FAILURE: $opt" >&2; exit 2; fi
    export TRANSFORM_RETAIL_IMAGE="$OUT/retail_23538.bin"
    status=0
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$OUT/$opt.log"
    elif rg -q '^TRANSFORM RED branch/order' "$OUT/$opt.log"; then
        cat "$OUT/$opt.log" >&2
        red=1
    else
        cat "$OUT/$opt.log" >&2
        echo "TRANSFORM UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done
verify_source_pins
if [ "$red" -ne 0 ]; then
    echo "TRANSFORM RED: native func_80023538 still uses exclusive Scale/Compute branch" >&2
    exit 1
fi

python3 - "$OUT" <<'PY'
from pathlib import Path
import os, sys
out = Path(sys.argv[1])
source_path = Path(os.environ.get('TRANSFORM_NATIVE_SOURCE',
                                 'src/slus_006.64/system/temp1.c'))
source = source_path.read_text()
old = '        }\n        if (D_800591AD) {'
new = '        } else if (D_800591AD) {'
if source.count(old) != 1:
    raise SystemExit(f'old else-if mutation matched {source.count(old)} sites')
source = source.replace(old, new, 1)
sig = 'void func_80023538(void* pSpriteData, void* pAnimation) {'
begin = source.index(sig)
end = source.index('\n}', begin) + 2
header = '''#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef void SpriteData;
extern s32 D_800591A8; extern s32 D_80059198; extern u8 D_800591AD;
extern void SpriteComputeTransformMatrix(void*);
extern void SpriteSetScale(void*, short);
extern void func_800234AC(void*);
'''
(out / 'old.native.c').write_text(header + source[begin:end] + '\n')
PY
if ! build_case old O2 pc_port/tests/sprite_bind_transform_retail_test.c "$OUT/old.native.c"; then
    cat "$OUT/old.build.log" >&2
    echo "TRANSFORM CONTROL INFRASTRUCTURE FAILURE" >&2
    exit 2
fi
if TRANSFORM_RETAIL_IMAGE="$OUT/retail_23538.bin" "$OUT/old.test" > "$OUT/old.log" 2>&1; then
    echo "TRANSFORM CONTROL FAILED TO REJECT old else-if" >&2
    exit 1
elif rg -q '^TRANSFORM RED branch/order' "$OUT/old.log"; then
    echo "TRANSFORM CONTROL PASS: old else-if rejected"
else
    cat "$OUT/old.log" >&2
    echo "TRANSFORM CONTROL UNEXPECTED FAILURE" >&2
    exit 2
fi

python3 - "$OUT" <<'PY'
from pathlib import Path
import os, sys
out = Path(sys.argv[1])
source_path = Path(os.environ.get('TRANSFORM_NATIVE_SOURCE',
                                 'src/slus_006.64/system/temp1.c'))
source = source_path.read_text()
old = '''        if (D_800591AD) {
            SpriteComputeTransformMatrix(pData);'''
new = '''        if (1) {
            SpriteComputeTransformMatrix(pData);'''
if source.count(old) != 1:
    raise SystemExit(f'cached-condition mutation matched {source.count(old)} sites')
source = source.replace(old, new, 1)
sig = 'void func_80023538(void* pSpriteData, void* pAnimation) {'
begin = source.index(sig)
end = source.index('\n}', begin) + 2
header = '''#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef void SpriteData;
extern s32 D_800591A8; extern s32 D_80059198; extern u8 D_800591AD;
extern void SpriteComputeTransformMatrix(void*);
extern void SpriteSetScale(void*, short);
extern void func_800234AC(void*);
'''
(out / 'cached.native.c').write_text(header + source[begin:end] + '\n')
PY
if ! build_case cached O2 pc_port/tests/sprite_bind_transform_retail_test.c "$OUT/cached.native.c"; then
    cat "$OUT/cached.build.log" >&2
    echo "TRANSFORM CACHED CONTROL INFRASTRUCTURE FAILURE" >&2
    exit 2
fi
if TRANSFORM_RETAIL_IMAGE="$OUT/retail_23538.bin" "$OUT/cached.test" > "$OUT/cached.log" 2>&1; then
    echo "TRANSFORM CONTROL FAILED TO REJECT cached condition" >&2
    exit 1
elif rg -q '^TRANSFORM RED branch/order' "$OUT/cached.log"; then
    echo "TRANSFORM CONTROL PASS: cached condition rejected"
else
    cat "$OUT/cached.log" >&2
    echo "TRANSFORM CACHED CONTROL UNEXPECTED FAILURE" >&2
    exit 2
fi
echo "TRANSFORM GREEN: O0/O2/UBSan focused retail/native helper order regression; $OUT"
