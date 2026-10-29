#!/usr/bin/env bash
# M1(b) retail-differential proof for the hand-written sprite draw set:
#   func_8001E3D8 / func_8001DAE8 / func_8001D53C / func_8001E9BC (rendering.c)
#   func_80025FA8 / func_800278F8 (temp1f.c)
# Native port bodies vs the retail SLUS instructions on the port MIPS
# interpreter, identical guest memory (see sprite_draw_retail_harness.h).
#
# Phases
#   A  baseline: production sources as they are. Result per function is
#      PASS or DIVERGES. The documented ledger below is the KNOWN state; the
#      script exits 0 when the observed state equals the ledger and 1 when it
#      changes (a fixed divergence or a new one). XENO_SDR_STRICT=1 exits 1
#      while any divergence remains.
#   B  witness: scratch copies (never src/) with the minimal fixes for the
#      ledger's divergences applied. Must be PASS in O0/O2/UBSan; proves the
#      ledger lists the divergences and nothing else.
#   C  negative controls: single-fault mutants of the witness. Each must FAIL.
# Needs: disc/SLUS_006.64 (or XENO_SLUS), pc_port/extern/PsyCross (patched, see
# pc_port/build_port.sh), gcc/g++/clang++, python3.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
export XENO_SLUS=${XENO_SLUS:-disc/SLUS_006.64}
CASES=${XENO_SDR_CASES:-3000}
OUT=pc_port/build_native/sprite_draw_retail_test
mkdir -p "$OUT"
read -r sha _ < <(sha256sum "$XENO_SLUS")
test "$sha" = dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 || { echo "SPRITE DRAW RETAIL: wrong SLUS_006.64 ($XENO_SLUS)" >&2; exit 2; }
[ -d pc_port/extern/PsyCross/src ] || { echo "SPRITE DRAW RETAIL: patched PsyCross tree missing (build_port.sh pin)" >&2; exit 2; }

COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/tests -Ipc_port/build_native)
sed -n -e '/^u_short GetClut(/,/^}/p' -e '/^u_short GetTPage(/,/^}/p' -e '/^void AddPrim(/,/^}/p' \
    -e '/^void SetSemiTrans(/,/^}/p' -e '/^void SetPolyFT4(/,/^}/p' -e '/^void SetShadeTex(/,/^}/p' \
    pc_port/extern/PsyCross/src/psx/LIBGPU.C > "$OUT/gpu.c"

# UBSan excludes shift-base: the port bodies left-shift negative signed sprite
# offsets (benign on the targets; reported separately in the proof report).
flags_for() { if [ "$1" = UBSan ]; then echo "-O1 -fsanitize=undefined -fno-sanitize=shift-base -fno-sanitize-recover=all"; else echo "-$1"; fi; }

build_deps() {
    local opt=$1; read -r -a fl <<< "$(flags_for "$opt")"
    [ -f "$OUT/$opt.deps.done" ] && return 0
    for s in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        g++ -std=c++17 "${COMMON[@]}" "${fl[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$s" -o "$OUT/$opt.dep.${s##*/}.o"
    done
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.dep.adapter.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w -c pc_port/src/guest_prim_link.c -o "$OUT/$opt.dep.gpl.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w -include common.h -include psyq/libgpu.h -c "$OUT/gpu.c" -o "$OUT/$opt.dep.gpu.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w -c pc_port/src/psyq_compat.c -o "$OUT/$opt.dep.compat.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -Wall -Wextra -Werror -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$opt.dep.leaf.o"
    touch "$OUT/$opt.deps.done"
}
link() { # opt out objs...
    local opt=$1 out=$2; shift 2; read -r -a fl <<< "$(flags_for "$opt")"
    clang++ -no-pie "${fl[@]}" -Wl,--gc-sections "$@" "$OUT/$opt.dep."*.o -ldl -o "$out"
}
build_render() { # opt rendering.c-path name
    local opt=$1 src=$2 name=$3; read -r -a fl <<< "$(flags_for "$opt")"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w "-DSPRITE_RENDER_SOURCE=\"$ROOT/$src\"" \
        -c pc_port/tests/sprite_draw_retail_test.c -o "$OUT/$name.sdr.o"
    link "$opt" "$OUT/$name.sdr" "$OUT/$name.sdr.o"
}
build_temp1f() { # opt temp1f.c-path name
    local opt=$1 src=$2 name=$3; read -r -a fl <<< "$(flags_for "$opt")"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w -c "$src" -o "$OUT/$name.t1f.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w -c pc_port/tests/horizon_strip_retail_test.c -o "$OUT/$name.hz.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${fl[@]}" -fpermissive -w -c pc_port/tests/atlas_quad_sweep_retail_test.c -o "$OUT/$name.aq.o"
    link "$opt" "$OUT/$name.hz" "$OUT/$name.hz.o" "$OUT/$name.t1f.o"
    link "$opt" "$OUT/$name.aq" "$OUT/$name.aq.o" "$OUT/$name.t1f.o"
}

# ---- scratch witness / mutant sources (never written to src/) ----
python3 - <<'PY'
import os
from pathlib import Path
out = Path(os.environ.get('OUT', 'pc_port/build_native/sprite_draw_retail_test'))
r = Path('src/slus_006.64/system/rendering.c').read_text()
t = Path('src/slus_006.64/system/temp1f.c').read_text()
def rep(s, old, new, n=1):
    assert s.count(old) == n, (s.count(old), old)
    return s.replace(old, new)
def seg(s, marker):
    a = s.index(marker); b = s.index('\n}\n', a) + 3
    return a, b
def mut_in(s, marker, old, new):
    a, b = seg(s, marker); body = s[a:b]
    assert body.count(old) >= 1, (marker, old)
    return s[:a] + body.replace(old, new, 1) + s[b:]
# The production sources carry the retail-faithful fixes; the witness is the
# production source itself (kept as a scratch copy so mutants derive from it).
w = r
(out / 'witness_rendering.c').write_text(w)
(out / 'baseline_rendering_copy.c').write_text(r)
wt = t
(out / 'witness_temp1f.c').write_text(wt)
# negative controls on the witness ----------------------------------------------
E = 'void func_8001E3D8(void* pSpriteData, void* ot) {'
ctl = []
def add(name, kind, text): (out / (name + ('.rendering.c' if kind == 'r' else '.temp1f.c'))).write_text(text); ctl.append((name, kind))
add('e3d8_flip_uv_clamp', 'r', mut_in(w, E, 'texU--;', 'texU++;'))
add('e3d8_base_x_negate', 'r', mut_in(w, E, 'baseXOffset = -baseXOffset;', 'baseXOffset = baseXOffset;'))
add('e3d8_flipy', 'r', mut_in(w, E, 'y1 = -y1;', 'y1 = y1;'))
add('e3d8_ot_depth', 'r', mut_in(w, E, '(u8*)ot - direction * 4, poly', '(u8*)ot - direction * 2, poly'))
add('e3d8_dir_vz_sign', 'r', mut_in(w, E, 'rotation.vz = (s16)-(s32)(u16)rotation.vz;', 'rotation.vz = rotation.vz;'))
add('e3d8_dir_round', 'r', mut_in(w, E, 'ScaleSpriteFrameByte32(dirXOffset', '(dirXOffset = 0, ScaleSpriteFrameByte32)(dirXOffset'))
add('dae8_u_base8', 'r', mut_in(w, 'void func_8001DAE8(void* pSpriteData, u16 frameIndex, u32 animPackageAddr) {', 'texU = texUBase8 + (texX << 1);', 'texU = texUBase8 + (texX << 2);'))
add('dae8_v_base', 'r', mut_in(w, 'void func_8001DAE8(void* pSpriteData, u16 frameIndex, u32 animPackageAddr) {', '*(u8*)(pPrim + 0x5) = texY + baseClutY;', '*(u8*)(pPrim + 0x5) = texY;'))
add('d53c_clut_row', 'r', mut_in(w, 'void func_8001D53C(void* pSpriteData, u16 frameIndex, void* pAnimPackage) {', '((tileHeader >> 9) & 0xF) + 0x1CC', '((tileHeader >> 9) & 0xF) + 0x1CD'))
add('d53c_tile_u', 'r', mut_in(w, 'void func_8001D53C(void* pSpriteData, u16 frameIndex, void* pAnimPackage) {', '*(u8*)(pPrim + 0x4) = texXOffset + pTile[1];', '*(u8*)(pPrim + 0x4) = texXOffset;'))
add('e9bc_half_scale', 'r', mut_in(w, 'void func_8001E9BC(void* sprite, void* ot) {', 'scale.vy = scale.vx / 2;', 'scale.vy = scale.vx / 4;'))
add('a25fa8_u_dec', 'k', mut_in(wt, 's32 func_80025FA8(', 'if (rotation == 0xc00) u--;', 'if (rotation == 0xc00) u++;'))
add('a25fa8_flipx', 'k', mut_in(wt, 's32 func_80025FA8(', 'item[26] ? x + width : x;', 'item[27] ? x + width : x;'))
add('h78f8_fadeplus', 'k', mut_in(wt, 'void func_800278F8(u8* ctx, s32 scroll, s32 screenY, s32 fade, void* ot,\n                   s32 renderCtx) {', 's32 fadePlus = fade + 0x100;', 's32 fadePlus = fade + 0x101;'))
add('h78f8_half_sign', 'k', mut_in(wt, 'void func_800278F8(u8* ctx, s32 scroll, s32 screenY, s32 fade, void* ot,\n                   s32 renderCtx) {', 'half = (t + (s32)((u32)t >> 31)) >> 1;', 'half = (t + (t >> 31)) >> 1;'))
(out / 'controls.tsv').write_text(''.join(n + '\t' + k + '\n' for n, k in ctl))
PY
export OUT

summary() { grep -E "^(SPRITE DRAW func_|HORIZON STRIP func_|ATLAS SWEEP func_)" "$1" | sed -E 's/ (pass|fail)=.*//' ; }

# ---------------- phase A: baseline ledger ----------------
LEDGER='SPRITE DRAW func_8001E3D8                      PASS    
SPRITE DRAW func_8001E3D8(west/flip targets)   PASS    
SPRITE DRAW func_8001DAE8                      PASS    
SPRITE DRAW func_8001D53C                      PASS    
SPRITE DRAW func_8001DAE8->D53C route          PASS    
SPRITE DRAW func_8001E9BC                      PASS    
HORIZON STRIP func_800278F8 PASS
ATLAS SWEEP func_80025FA8 PASS'
build_deps O0
build_render O0 "$OUT/baseline_rendering_copy.c" base
build_temp1f O0 src/slus_006.64/system/temp1f.c base
: > "$OUT/baseline.log"
for b in base.sdr base.hz base.aq; do "$OUT/$b" "$CASES" >> "$OUT/baseline.log" 2>&1 || true; done
cat "$OUT/baseline.log" | grep -v "^note" | grep -v "^  " 
summary "$OUT/baseline.log" > "$OUT/baseline.summary"
printf '%s\n' "$LEDGER" | sed -E 's/ +PASS +$/ PASS/; s/ +(DIVERGES|PASS)$/ \1/' > "$OUT/ledger.norm"
sed -E 's/ +(DIVERGES|PASS) *$/ \1/' "$OUT/baseline.summary" > "$OUT/baseline.norm"
if ! diff -u <(sed -E 's/  +/ /g' "$OUT/ledger.norm") <(sed -E 's/  +/ /g' "$OUT/baseline.norm"); then
    echo "SPRITE DRAW RETAIL: observed baseline differs from the documented ledger (a divergence was fixed or a new one appeared)" >&2
    exit 1
fi
echo "SPRITE DRAW RETAIL phase A: production baseline matches ledger (all PASS)"

# ---------------- phase B: witness must be PASS in every mode ----------------
for opt in O0 O2 UBSan; do
    build_deps "$opt"
    build_render "$opt" "$OUT/witness_rendering.c" "w$opt"
    build_temp1f "$opt" "$OUT/witness_temp1f.c" "w$opt"
    : > "$OUT/w$opt.log"
    "$OUT/w$opt.sdr" "$CASES" >> "$OUT/w$opt.log" 2>&1 || { cat "$OUT/w$opt.log" >&2; echo "SPRITE DRAW RETAIL: witness not green in $opt" >&2; exit 1; }
    "$OUT/w$opt.hz" 6000 >> "$OUT/w$opt.log" 2>&1 || { tail -5 "$OUT/w$opt.log" >&2; echo "SPRITE DRAW RETAIL: horizon witness not green in $opt" >&2; exit 1; }
    "$OUT/w$opt.aq" 6000 >> "$OUT/w$opt.log" 2>&1 || { tail -5 "$OUT/w$opt.log" >&2; echo "SPRITE DRAW RETAIL: atlas not green in $opt" >&2; exit 1; }
    grep -E "^(SPRITE DRAW (func_|GREEN)|HORIZON STRIP func_|ATLAS SWEEP func_)" "$OUT/w$opt.log" | sed "s/^/[$opt witness] /"
done
# ---------------- phase C: negative controls ----------------
build_deps O2
while IFS=$'\t' read -r name kind; do
    if [ "$kind" = r ]; then
        build_render O2 "$OUT/$name.rendering.c" "$name"
        bin="$OUT/$name.sdr"; env=()
    else
        build_temp1f O2 "$OUT/$name.temp1f.c" "$name"
        case "$name" in a25fa8*) bin="$OUT/$name.aq" ;; *) bin="$OUT/$name.hz" ;; esac; env=()
    fi
    if env "${env[@]}" "$bin" 6000 > "$OUT/$name.log" 2>&1; then
        echo "SPRITE DRAW RETAIL: negative control SURVIVED: $name" >&2; exit 1
    fi
    grep -q " FAIL " "$OUT/$name.log" || { echo "SPRITE DRAW RETAIL: control $name died without a FAIL line" >&2; tail -3 "$OUT/$name.log" >&2; exit 1; }
    echo "negative control rejected: $name: $(grep -m1 ' FAIL ' "$OUT/$name.log" | cut -c1-200)"
done < "$OUT/controls.tsv"
echo "SPRITE DRAW RETAIL: witness GREEN O0/O2/UBSan, all $(wc -l < "$OUT/controls.tsv") negative controls rejected"
if [ "${XENO_SDR_STRICT:-0}" = 1 ] && grep -q DIVERGES "$OUT/baseline.norm"; then
    echo "SPRITE DRAW RETAIL STRICT: production divergences still present (see ledger)" >&2; exit 1
fi
