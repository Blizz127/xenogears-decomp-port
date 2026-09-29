#!/usr/bin/env bash
# Animation-render callback index 15 regression: retail-backed differential for
# func_800257F0 (main exe) and its renderer dependency func_800B1F6C /
# func_800B1F0C (battle overlay), plus deliberate mutants the test must reject.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/anim_render_index15_retail_test.XXXXXXXX)
export OUT
echo "ANIMRENDER15 artifacts: $OUT"

python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, re, sys

out = Path(sys.argv[1])
slus = Path('disc/SLUS_006.64').read_bytes()
battle = Path('disc/battle.bin').read_bytes()
assert sha256(slus).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
assert sha256(battle).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'

SLUS_BASE = 0x8000F800
BATTLE_BASE = 0x8006FAF0

def slice_of(image, base, start, end):
    return image[start - base:end - base]

pins = {
    'disc/SLUS_006.64': sha256(slus).hexdigest(),
    'disc/battle.bin': sha256(battle).hexdigest(),
}
slices = [
    ('func_800257F0', slus, SLUS_BASE, 0x800257F0, 0x80025A88,
     '33b97b5d0459461ebbcb6f68766ef00ff00cfd28415e85ae813282f043b5644c'),
    ('func_80025A88', slus, SLUS_BASE, 0x80025A88, 0x80025C04,
     'da5e30b733f88a53160c8f0aeee80c333f53f04ca88dfba49b0bfe0fdad18bbb'),
    ('func_800B1F0C', battle, BATTLE_BASE, 0x800B1F0C, 0x800B1F6C,
     '5b092f69365f143f814da4eda798bc348f7fc0157f75fd511d06a5edd930b523'),
    ('func_800B1F6C', battle, BATTLE_BASE, 0x800B1F6C, 0x800B2AEC,
     '31ddad0018967c4f9bd3d5dc2afabf76152c6d7b39723d6693efe2891917f8e5'),
    ('D_8004FD40', slus, SLUS_BASE, 0x8004FD40, 0x8004FD80,
     '604b19b8756542d9d9914258e6f8e892346572603150a7df945cc96ca42ffe2d'),
]
for name, image, base, start, end, expected in slices:
    actual = sha256(slice_of(image, base, start, end)).hexdigest()
    assert actual == expected, f'{name} slice hash changed: {actual}'
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}

table = slice_of(slus, SLUS_BASE, 0x8004FD40, 0x8004FD80)
import struct
entries = struct.unpack('<16I', table)
assert entries[15] == 0x800257F0, hex(entries[15])

# Annotated instruction bytes must match the disc images exactly.
asm_checks = [
    ('asm/slus_006.64/matchings/system/temp1/func_800257F0.s', slus, SLUS_BASE, 166),
    ('asm/slus_006.64/matchings/system/temp1/func_80025A88.s', slus, SLUS_BASE, 95),
    ('asm/battle/nonmatchings/main77/func_800B1F6C.s', battle, BATTLE_BASE, 736),
    ('asm/battle/nonmatchings/main77/func_800B1F0C.s', battle, BATTLE_BASE, 24),
]
def locate(path):
    """The split layout moves functions between TUs and between matchings/
    nonmatchings (temp1 -> temp1e, ...); find the listing by name."""
    p = Path(path)
    if p.exists():
        return p
    hits = sorted(Path(path.split('/')[0], path.split('/')[1]).rglob(p.name))
    assert len(hits) == 1, f'{p.name}: {len(hits)} listings found'
    return hits[0]

for path, image, base, expected_count in asm_checks:
    path = str(locate(path))
    text = Path(path).read_text()
    rows = re.findall(r'/\* ([0-9A-F]+) ([0-9A-F]+) ([0-9A-F]+) \*/', text)
    assert len(rows) == expected_count, f'{path}: {len(rows)} rows'
    for _, address, word in rows:
        offset = int(address, 16) - base
        assert image[offset:offset + 4] == bytes.fromhex(word), \
            f'{path}:{address} mismatch'
    print(f'ANIMRENDER15 {path}: {len(rows)} annotated bytes match disc')

# -- Production extraction -------------------------------------------------
inc = Path('src/battle/anim_render_index15.inc').read_text()
overrides = Path('pc_port/src/game_overrides.c').read_text()

def function(src, sig):
    start = src.index(sig)
    brace = src.index('{', start)
    depth = 0
    for i in range(brace, len(src)):
        depth += (src[i] == '{') - (src[i] == '}')
        if depth == 0:
            return src[start:i + 1]
    raise AssertionError(sig)

helper = function(inc, 'static u8* BattleAnimRenderAddress(')
f_tri = function(inc, 'static void BattleAnimRenderTriangle(')
f_quad = function(inc, 'static void BattleAnimRenderQuad(')
f1f0c = function(inc, 'void func_800B1F0C(')
f1f6c = function(inc, 'void func_800B1F6C(')
f257f0 = function(inc, 'void func_800257F0(')
f25a88 = function(inc, 'void func_80025A88(')
assert 'func_800257F0' in overrides
assert '(void (*)(void*))func_800257F0' in overrides

defs = r'''
#define ANIM_RENDER_COLOR_MATRIX  0x8004FD80u
#define ANIM_RENDER_LIGHT_MATRIX  0x8004FDA0u
#define ANIM_RENDER_TPAGE_PACKET  0x800C3BF8u
'''
common_prologue = r'''
#include <stdint.h>
#include <stddef.h>
#include "psx_memory.h"
#include "guest_prim_link.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include <psx/inline_c.h>
#include <psx/gtereg.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32;
extern s32 D_80050100;
extern s32 g_GfxCurContext;
extern u_long* g_GfxCurOT;
extern MATRIX D_8004FBB8;
extern u32 g_GfxCurWorkBuffer;
extern u32 g_GfxCurWorkBufferEnd;
''' + defs

callback_production = common_prologue + r'''
extern void func_80022038(void*);
extern void func_800B1F6C(void*, void*, void*, s32, s32, s32);
''' + '\n' + helper + '\n\n' + f25a88 + '\n\n' + f257f0 + '\n'
(out / 'production_callback.c').write_text(callback_production)

renderer_production = common_prologue + r'''
void func_800B1F0C(u32* otEntry);
''' + '\n' + helper + '\n\n' + f_tri + '\n\n' + f_quad + '\n\n' + f1f0c + '\n\n' + f1f6c + '\n'
(out / 'production_renderer.c').write_text(renderer_production)

paths = [
    'src/battle/anim_render_index15.inc',
    'pc_port/src/game_overrides.c',
    'pc_port/port_owned_overrides.txt',
    'pc_port/src/battle_mips_adapter.c',
    'pc_port/src/battle_mips_adapter.h',
    'pc_port/src/guest_prim_link.c',
    'pc_port/include_shim/guest_prim_link.h',
    'pc_port/src/psx_memory.h',
    'pc_port/tests/anim_render_index15_retail_test.c',
    'pc_port/tests/anim_render_renderer_retail_test.c',
    'pc_port/tests/run_anim_render_index15_retail_test.sh',
    'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
    'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
    'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
    'pc_port/extern/PsyCross/include/psx/inline_c.h',
    'pc_port/extern/PsyCross/include/psx/gtereg.h',
    'pc_port/extern/PsyCross/include/psx/libgte.h',
]
proof = {
    'retail_disc_sha256': pins,
    'generated_callback_sha256': sha256(callback_production.encode()).hexdigest(),
    'generated_renderer_sha256': sha256(renderer_production.encode()).hexdigest(),
    'source_sha256': {p: sha256(Path(p).read_bytes()).hexdigest() for p in paths},
    'scope': ('Actual SLUS 800257F0 callback slice with all callees bridged to the same host '
              'GTE the native body calls, and the whole battle.bin overlay renderer with the '
              'interpreter COP2 bus wired to the same PsyCross GTE.  Finite boundary census; '
              'no exhaustive descriptor-universe claim.'),
}
(out / 'provenance.json').write_text(json.dumps(proof, indent=2) + '\n')
print('ANIMRENDER15 pins OK, production extracted')
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

psx_flags() {
    local opt=$1
    if [ "$opt" = UBSan ]; then
        echo '-O1 -fsanitize=undefined -fno-sanitize-recover=all'
    else
        echo "-$opt"
    fi
}

# GCC on this host can be built without the UBSan runtime; probe the link and
# fall back to clang explicitly rather than skipping the regime.
UBSAN_CC=gcc; UBSAN_LINK=g++
if ! printf 'int main(void){return 0;}\n' | gcc -x c - -fsanitize=undefined \
        -o "$OUT/ubsan_probe" >/dev/null 2>&1; then
    UBSAN_CC=clang; UBSAN_LINK=clang++
fi
rm -f "$OUT/ubsan_probe"
if [ "$UBSAN_CC" = clang ]; then
    echo "ANIMRENDER15 UBSan regime: clang (gcc -fsanitize=undefined cannot link on this host)"
else
    echo "ANIMRENDER15 UBSan regime: gcc"
fi

build_psx() {
    local opt=$1
    local flags; flags=$(psx_flags "$opt")
    local cxx=g++; local extra=()
    if [ "$opt" = UBSan ]; then cxx=$UBSAN_LINK; extra=(-Wno-c++11-narrowing); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        local name=${source##*/}
        "$cxx" -std=c++17 "${COMMON[@]:1}" $flags -fpermissive -w "${extra[@]}" \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" \
            -o "$OUT/$opt.$name.o" > "$OUT/$opt.$name.log" 2>&1 || {
                cat "$OUT/$opt.$name.log" >&2; return 1; }
    done
}

build_case() {
    local opt=$1 which=$2 test_c=$3 production=$4
    local flags; flags=$(psx_flags "$opt")
    local cc=gcc link=g++
    if [ "$opt" = UBSan ]; then cc=$UBSAN_CC; link=$UBSAN_LINK; fi
    local clangonly=()
    if [ "$cc" = clang ]; then clangonly=(-Wno-unknown-warning-option); fi
    "$cc" "${COMMON[@]}" $flags -Wall -Wextra -Werror -Wno-stringop-overflow \
        -Wno-missing-field-initializers "${clangonly[@]}" \
        -c "pc_port/tests/$test_c" -o "$OUT/$opt.$which.test.o" > "$OUT/$opt.$which.log" 2>&1 || {
            cat "$OUT/$opt.$which.log" >&2; return 1; }
    "$cc" "${COMMON[@]}" $flags -Wno-incompatible-pointer-types \
        -c "$OUT/$production" -o "$OUT/$opt.$which.production.o" >> "$OUT/$opt.$which.log" 2>&1 || {
            cat "$OUT/$opt.$which.log" >&2; return 1; }
    "$cc" "${COMMON[@]}" $flags -c pc_port/src/battle_mips_adapter.c \
        -o "$OUT/$opt.cpu.o" >> "$OUT/$opt.$which.log" 2>&1
    "$cc" "${COMMON[@]}" $flags -c pc_port/src/guest_prim_link.c \
        -o "$OUT/$opt.link.o" >> "$OUT/$opt.$which.log" 2>&1
    "$link" -no-pie $flags -Wl,--gc-sections \
        "$OUT/$opt.$which.test.o" "$OUT/$opt.$which.production.o" \
        "$OUT/$opt.cpu.o" "$OUT/$opt.link.o" \
        "$OUT/$opt.LIBGTE.C.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" \
        -lstdc++ -lm -o "$OUT/$opt.$which" >> "$OUT/$opt.$which.log" 2>&1 || {
            cat "$OUT/$opt.$which.log" >&2; return 1; }
}

failed=0
for opt in O0 O2 UBSan; do
    build_psx "$opt" || { echo "ANIMRENDER15 INFRASTRUCTURE FAILURE: $opt psx" >&2; exit 2; }
    for which in callback renderer; do
        if [ "$which" = callback ]; then
            test_c=anim_render_index15_retail_test.c
            production=production_callback.c
        else
            test_c=anim_render_renderer_retail_test.c
            production=production_renderer.c
        fi
        if ! build_case "$opt" "$which" "$test_c" "$production"; then
            echo "ANIMRENDER15 INFRASTRUCTURE FAILURE: $opt $which" >&2
            exit 2
        fi
        status=0
        "$OUT/$opt.$which" > "$OUT/$opt.$which.run.log" 2>&1 || status=$?
        if [ "$status" -eq 0 ]; then
            grep -E '^INDEX15|^ANIMRENDER' "$OUT/$opt.$which.run.log"
        elif grep -qE '^INDEX15 FAIL|^ANIMRENDER FAIL' "$OUT/$opt.$which.run.log"; then
            cat "$OUT/$opt.$which.run.log" >&2
            failed=1
        else
            cat "$OUT/$opt.$which.run.log" >&2
            echo "ANIMRENDER15 UNEXPECTED FAILURE: $opt $which rc=$status" >&2
            exit 2
        fi
    done
done
verify_source_pins

# -- Mutants ---------------------------------------------------------------
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
mutants = {
    'cb-packed64-sprite': ('production_callback.c',
        'u8* pSprite = BattleAnimRenderAddress((uintptr_t)*(u32*)(pArg + 4));',
        'u8* pSprite = *(u8**)(pArg + 4);'),
    'cb-packed64-data': ('production_callback.c',
        'pData = BattleAnimRenderAddress((uintptr_t)*(u32*)(pSprite + 0x20));',
        'pData = *(u8**)(pSprite + 0x20);'),
    'cb-context-stride': ('production_callback.c',
        '(u32)g_GfxCurContext * 4u', '(u32)g_GfxCurContext * 8u'),
    'cb-no-geom-restore': ('production_callback.c',
        '    if (*(s32*)(pSprite + 0x3C) < 0)\n        SetGeomOffset((s32)geomX, (s32)geomY);',
        ''),
    'cb-no-shift-restore': ('production_callback.c',
        'D_80050100 = saved;', 'D_80050100 = D_80050100;'),
    'cb-halved-bit': ('production_callback.c',
        '>> 25) & 1', '>> 24) & 1'),
    'sb-packed64-sub': ('production_callback.c',
        'pSub = BattleAnimRenderAddress((uintptr_t)*(u32*)(pSprite + 0x20));',
        'pSub = *(u8**)(pSprite + 0x20);'),
    'sb-a5-mask': ('production_callback.c',
        '(s32)((u32)(*(u8*)(pSprite + 0x3C)) >> 5)',
        '(s32)((*(u32*)(pSprite + 0x3C) >> 5) & 1)'),
    'sb-no-shift-restore': ('production_callback.c',
        'D_80050100 = oldShift;', 'D_80050100 = D_80050100;'),
    'sb-result-sum': ('production_callback.c',
        '*(s32*)(pSub + 0x20) = D_8004FBB8.t[0] + result.vx;',
        '*(s32*)(pSub + 0x20) = result.vx;'),
    'rd-vertex-stride': ('production_renderer.c',
        ') * 8u;', ') * 4u;'),
    'rd-packet-stride': ('production_renderer.c',
        'out += ((u32)record[0] + 1u) * 4u;', 'out += ((u32)record[0] + 1u) * 2u;'),
    'rd-record-stride': ('production_renderer.c',
        'record += ((u32)record[1] + 1u) * 4u;', 'record += ((u32)record[1] + 1u) * 8u;'),
    'rd-shift-mask': ('production_renderer.c',
        '((u32)D_80050100 & 31u)', '(u32)D_80050100'),
    'rd-min-clamp': ('production_renderer.c',
        'if (depth < 5)', 'if (depth < 4)'),
    'rd-tpage-shift': ('production_renderer.c',
        '((tpage - 1) & 3) << 5', 'tpage << 5'),
    'rd-work-cmp': ('production_renderer.c',
        '(uintptr_t)work + 8u >= (uintptr_t)end', '(uintptr_t)work + 8u > (uintptr_t)end'),
}
for name, (source_name, old, new) in mutants.items():
    source = (out / source_name).read_text()
    count = source.count(old)
    assert count >= 1, f'{name}: pattern not found'
    (out / f'mutant-{name}.c').write_text(source.replace(old, new))
PY

check_red() {
    local name=$1 source=$2 base=$3 opt=O2
    local flags; flags=$(psx_flags "$opt")
    local cc=gcc link=g++
    if [ "$name" = rd-shift-mask ]; then
        opt=UBSan; flags=$(psx_flags UBSan); cc=$UBSAN_CC; link=$UBSAN_LINK
    fi
    if ! "$cc" "${COMMON[@]}" $flags -Wno-incompatible-pointer-types -c "$source" \
        -o "$OUT/$name.o" > "$OUT/$name.build.log" 2>&1; then
        cat "$OUT/$name.build.log" >&2
        echo "ANIMRENDER15 MUTANT BUILD FAILURE: $name" >&2
        exit 2
    fi
    "$link" -no-pie $flags -Wl,--gc-sections \
        "$OUT/$opt.$base.test.o" "$OUT/$name.o" "$OUT/$opt.cpu.o" "$OUT/$opt.link.o" \
        "$OUT/$opt.LIBGTE.C.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" \
        -lstdc++ -lm -o "$OUT/$name" > "$OUT/$name.link.log" 2>&1 || {
            cat "$OUT/$name.link.log" >&2
            echo "ANIMRENDER15 MUTANT LINK FAILURE: $name" >&2
            exit 2
        }
    local status=0
    "$OUT/$name" > "$OUT/$name.run.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        echo "ANIMRENDER15 CONTROL FAILED TO REJECT: $name" >&2
        failed=1
    elif [ "$status" -eq 90 ] && grep -q 'POINTER_FAULT SIGSEGV' "$OUT/$name.run.log"; then
        echo "ANIMRENDER15 CONTROL PASS: $name rejected (trapped SIGSEGV)"
    elif [ "$status" -eq 1 ] && grep -qE '^INDEX15 FAIL|^ANIMRENDER FAIL' "$OUT/$name.run.log"; then
        echo "ANIMRENDER15 CONTROL PASS: $name rejected"
    elif [ "$name" = rd-shift-mask ] && [ "$status" -eq 1 ] && \
         grep -q 'runtime error: shift exponent' "$OUT/$name.run.log"; then
        echo "ANIMRENDER15 CONTROL PASS: $name rejected (UBSan shift exponent)"
    else
        cat "$OUT/$name.run.log" >&2
        echo "ANIMRENDER15 CONTROL UNEXPECTED FAILURE: $name rc=$status" >&2
        exit 2
    fi
}

for source in "$OUT"/mutant-*.c; do
    name=${source##*/}; name=${name%.c}; name=${name#mutant-}
    case "$name" in
        cb-*) check_red "$name" "$source" callback ;;
        sb-*) check_red "$name" "$source" callback ;;
        rd-*) check_red "$name" "$source" renderer ;;
    esac
done

verify_source_pins
if [ "$failed" -ne 0 ]; then
    echo "ANIMRENDER15 RED: native/retail mismatch" >&2
    exit 1
fi
echo "ANIMRENDER15 GREEN: O0/O2/UBSan callback + renderer retail differential; mutants rejected; $OUT"
