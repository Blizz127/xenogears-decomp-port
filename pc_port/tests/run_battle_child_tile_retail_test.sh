#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

OUT=$(mktemp -d pc_port/build_native/battle_child_tile_retail_test.XXXXXXXX)
export OUT
echo "TYPE9 artifacts: $OUT"

python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, sys

out = Path(sys.argv[1])
source_path = Path(os.environ.get('TYPE9_SOURCE_OVERRIDE',
                                  'src/slus_006.64/system/temp1e.c'))
source = source_path.read_text()
overrides_path = Path(os.environ.get('TYPE9_OVERRIDES_OVERRIDE',
                                     'pc_port/src/game_overrides.c'))
overrides = overrides_path.read_text()
disc = Path('disc/SLUS_006.64').read_bytes()
base = 0x8000f800
callback = disc[0x80025544-base:0x80025710-base]
assert len(callback) == 0x1cc
assert sha256(callback).hexdigest() == '66f10552f9af792cfe90720e086429dec9fe4801fa0cc31079a8fde65a34eba8'
assert sha256(disc).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'

def function(src, signature):
    # The definition, not a prototype: skip occurrences where a ';' comes
    # before the opening brace.
    start = src.index(signature)
    while ';' in src[start:src.index('{', start)]:
        start = src.index(signature, start + 1)
    brace = src.index('{', start)
    depth = 0
    for i in range(brace, len(src)):
        depth += (src[i] == '{') - (src[i] == '}')
        if depth == 0:
            return src[start:i+1]
    raise AssertionError(signature)

sprite_address = function(source, 'static u8* SpriteRenderAddress(')
tile = function(source, 'void func_80025544(')
table_start = overrides.index('static void (*const D_8004FD40')
table_end = overrides.index('\n};', table_start) + 3
table = overrides[table_start:table_end]

prologue = r'''
#include <stdint.h>
#include <stddef.h>
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32;
extern u8 D_800C3664; extern s32 D_80050100; extern MATRIX D_8004FBB8;
extern u_long *g_GfxCurOT; extern void *g_GfxCurWorkBuffer;
extern void *g_GfxCurWorkBufferEnd;
extern void SetRotMatrix(MATRIX *); extern void SetTransMatrix(MATRIX *);
extern int RotTransPers3(SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *);
extern void AddPrim(void *, void *);
extern void PcPort_AddPrimDomainAware(void *, void *);
extern void func_80025258(void *); extern void func_80025710(void *);
extern void func_80025718(void *); extern void func_8002541C(void *);
extern void func_80025544(u8 *); extern void func_800257F0(void *);
extern void WorkListSetTaskCallback(void *, void (*)(void *));
'''
body = prologue + '\n' + sprite_address + '\n' + tile + '\n' + table + r'''

void func_80025224(void *task, int handlerIndex)
{
    WorkListSetTaskCallback(task, D_8004FD40[handlerIndex & 0xf]);
}
'''
(out / 'production.c').write_text(body)
red_path = os.environ.get('TYPE9_RED_SOURCE_OVERRIDE')
if red_path:
    red_tile = function(Path(red_path).read_text(), 'void func_80025544(')
    (out / 'pre-repair.c').write_text(body.replace(tile, red_tile))


paths = [str(source_path), str(overrides_path),
         'pc_port/src/battle_mips_adapter.c', 'pc_port/src/guest_prim_link.c',
         'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
         'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
         'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
         'pc_port/extern/PsyCross/include/psx/inline_c.h',
         'pc_port/extern/PsyCross/include/psx/gtemac.h',
         'pc_port/extern/PsyCross/include/psx/gtereg.h',
         'pc_port/extern/PsyCross/include/psx/libgte.h',
         'pc_port/src/battle_mips_adapter.h',
         'pc_port/include_shim/guest_prim_link.h', 'pc_port/src/psx_memory.h',
         'pc_port/tests/battle_child_tile_retail_test.c',
         'pc_port/tests/run_battle_child_tile_retail_test.sh']
if red_path: paths.append(red_path)
proof = {
    'source_override': str(source_path),
    'generated_production_sha256': sha256(body.encode()).hexdigest(),
    'retail_disc_sha256': sha256(disc).hexdigest(),
    'retail_callback': {'start': hex(0x80025544), 'end': hex(0x80025710),
                        'sha256': sha256(callback).hexdigest()},
    'source_sha256': {p: sha256(Path(p).read_bytes()).hexdigest() for p in paths},
    'scope': 'Direct MIPS bus: exact retail callback and AddPrim in both modes; REAL_GTE also executes retail SetRotMatrix, SetTransMatrix and RTP3 instructions. Native actual SDK and guest_prim_link.c. Controlled GTE boundary mode separately labeled.',
    'retail_bodies': {hex(a): {'size': n, 'sha256': sha256(disc[a-base:a-base+n]).hexdigest()} for a,n in [(0x80025544,0x1cc),(0x80049efc,0x30),(0x80049f8c,0x20),(0x8004a67c,0x54),(0x80043b48,0x3c)]}
}
(out / 'provenance.json').write_text(json.dumps(proof, indent=2) + '\n')
PY

COMMON=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -include assert.h -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

verify_source_pins() {
    python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
proof = json.loads((Path(os.environ['OUT']) / 'provenance.json').read_text())
for path, expected in proof['source_sha256'].items():
    actual = sha256(Path(path).read_bytes()).hexdigest()
    assert actual == expected, f'source changed during test: {path}'
PY
}

build_controlled() {
    local opt=$1
    local -a flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all); fi
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/battle_child_tile_retail_test.c -o "$OUT/$opt.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wno-incompatible-pointer-types -c "$OUT/production.c" -o "$OUT/$opt.production.o"
    clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/guest_prim_link.c -o "$OUT/$opt.link.o"
    clang -no-pie "${flags[@]}" -Wl,--wrap=PcPort_AddPrimDomainAware -Wl,--gc-sections -Wl,--export-dynamic \
        "$OUT/$opt.test.o" "$OUT/$opt.production.o" "$OUT/$opt.cpu.o" "$OUT/$opt.link.o" -ldl -o "$OUT/$opt"
    local status=0
    "$OUT/$opt" > "$OUT/$opt.log" 2>&1 || status=$?
    cat "$OUT/$opt.log"
    return "$status"
}

for opt in O0 O2 UBSan; do
    if build_controlled "$opt"; then :; else
        status=$?
        cat "$OUT/$opt.log" >&2 || true
        if [ "${TYPE9_EXPECT_RED:-0}" = 1 ]; then
            echo "TYPE9 RED expected from source ${TYPE9_SOURCE_OVERRIDE:-src/slus_006.64/system/temp1e.c}" >&2
            exit 1
        fi
        echo "TYPE9 CONTROLLED FAILURE: $opt" >&2
        exit "$status"
    fi
done
verify_source_pins

python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
source = (Path(sys.argv[1]) / 'production.c').read_text()
mutants = {
    'null-binding': ('(void (*)(void*))func_80025544, /* 9:', 'NULL, /* 9:'),
    'packed64': ('SpriteRenderAddress(*(u32*)(pEntry + 0x04))',
                 '((u8*)*(u8**)(pEntry + 0x04))'),
    'v2-not-repeat': ('RotTransPers3(&v0, &v1, &v0,',
                      'RotTransPers3(&v0, &v1, &v1,'),
    'ceil-half': ('halfSize = dimension >> 1;',
                  'halfSize = (dimension + 1) >> 1;'),
    'raw-size': ('*(u16*)(pTile + 0x0E) = (u16)dimension;',
                 '*(u16*)(pTile + 0x0E) = size;'),
    'raw-size-x': ('*(u16*)(pTile + 0x0C) = (u16)dimension;',
                   '*(u16*)(pTile + 0x0C) = size;'),
    'unmasked-shift': ('depth >>= (D_80050100 & 31);',
                       'depth >>= D_80050100;'),
    'ot-stride': ('otOffset = (u32)depth << 2;',
                  'otOffset = (u32)depth << 5;'),
    'inclusive-first': ('if (next >= (u32)(uintptr_t)g_GfxCurWorkBufferEnd) return;',
                        'if (next > (u32)(uintptr_t)g_GfxCurWorkBufferEnd) return;'),
    'wrong-packed-y': ('*(u32*)(pTile + 0x08) = (u32)xy0;',
                       '*(u16*)(pTile + 0x08) = (u16)xy0;'),
    'plain-link': ('    PcPort_AddPrimDomainAware(', '    AddPrim('),
    'depth-filter': ('*(u16*)(pSprite + 0x2E) = (u16)depth;', '*(u16*)(pSprite + 0x2E) = (u16)depth; if (depth <= 0) return;'),
    'reverse-link-order': ('pMode);', 'pTile);'),
}
old = 'if (next >= (u32)(uintptr_t)g_GfxCurWorkBufferEnd) return;'
assert source.count(old) == 2
prefix, sep, suffix = source.rpartition(old)
(Path(sys.argv[1]) / 'mutant-inclusive-second.c').write_text(prefix + sep.replace('>=', '>') + suffix)
for name, (old, new) in mutants.items():
    count = source.count(old)
    if count == 0:
        raise SystemExit(f'{name}: replacement did not match')
    replacement_count = 1 if name == 'inclusive-first' else count
    (Path(sys.argv[1]) / f'mutant-{name}.c').write_text(
        source.replace(old, new, replacement_count))
PY

# Semantic mutations must exit 1 with a TYPE9 FAIL marker. Only the packed
# pointer mutant and pre-repair draft may hit the specifically trapped SIGSEGV.
check_red() {
    local name=$1 source=$2
    local -a flags=(-O2)
    local mode=O2
    if [ "$name" = mutant-unmasked-shift ]; then
        mode=UBSan
        flags=(-O1 -fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all)
    fi
    clang "${COMMON[@]}" "${flags[@]}" -Wno-incompatible-pointer-types -c "$source" -o "$OUT/$name.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections -Wl,--wrap=PcPort_AddPrimDomainAware \
        "$OUT/$mode.test.o" "$OUT/$name.o" "$OUT/$mode.cpu.o" "$OUT/$mode.link.o" -o "$OUT/$name"
    local status=0
    "$OUT/$name" > "$OUT/$name.log" 2>&1 || status=$?
    if [ "$name" = mutant-unmasked-shift ]; then
        [ "$status" = 1 ] && rg -q 'runtime error: shift exponent 32 is too large' "$OUT/$name.log"
    elif [ "$name" = mutant-packed64 ] || [ "$name" = pre-repair ]; then
        [ "$status" = 90 ] && rg -q 'TYPE9 POINTER_FAULT SIGSEGV during native case' "$OUT/$name.log" &&
            [ "$(rg 'TYPE9 NATIVE case=' "$OUT/$name.log" | tail -1)" = "TYPE9 NATIVE case=gate-nonzero" ]
    else
        [ "$status" = 1 ] && rg -q 'TYPE9 FAIL' "$OUT/$name.log"
    fi || { cat "$OUT/$name.log" >&2; echo "TYPE9 INVALID RED: $name status=$status" >&2; exit 1; }
    echo "TYPE9 semantic RED: $name status=$status"
}
for source in "$OUT"/mutant-*.c; do
    name=${source##*/}; name=${name%.c}
    check_red "$name" "$source"
done
if [ -f "$OUT/pre-repair.c" ]; then check_red pre-repair "$OUT/pre-repair.c"; fi

build_real_gte() {
    local opt=$1
    local -a flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        local name=${source##*/}
        g++ -std=c++17 "${COMMON[@]:1}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" \
            -o "$OUT/$opt.$name.o"
    done
    clang "${COMMON[@]}" "${flags[@]}" -D TYPE9_REAL_GTE -Wall -Wextra -Werror \
        -c pc_port/tests/battle_child_tile_retail_test.c -o "$OUT/$opt.real.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -D TYPE9_REAL_GTE -Wno-incompatible-pointer-types \
        -c "$OUT/production.c" -o "$OUT/$opt.real.production.o"
    clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.real.cpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--wrap=PcPort_AddPrimDomainAware -Wl,--gc-sections -Wl,--export-dynamic \
        "$OUT/$opt.real.test.o" "$OUT/$opt.real.production.o" "$OUT/$opt.real.cpu.o" "$OUT/$opt.link.o" \
        "$OUT/$opt.LIBGTE.C.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" \
        -ldl -lstdc++ -lm -o "$OUT/$opt.real"
    "$OUT/$opt.real" > "$OUT/$opt.real.log" 2>&1
    cat "$OUT/$opt.real.log"
}

for opt in O0 O2; do build_real_gte "$opt"; done
verify_source_pins
echo "TYPE9 GREEN controlled+real-GTE retail/native regression: $OUT"
