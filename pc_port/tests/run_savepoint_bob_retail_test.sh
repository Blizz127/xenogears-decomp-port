#!/usr/bin/env bash
# Save-point bob loop (map 15 actor 60, sprite package 5 child): native
# AnimScriptTick + func_80022CDC against the retail bodies on the port MIPS
# adapter, frame by frame. See pc_port/tests/savepoint_bob_retail_test.c.
#
# SAVEPT_TEMP1 overrides the temp1.c under test (e.g. an older branch's copy)
# to show a known-bad body diverging; it is not used by default.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
source pc_port/tests/lib/sprite_d2_lib.sh
BUILD_DIR="${D2_BUILD_DIR:-$ROOT/pc_port/build_native/savepoint_bob}"
mkdir -p "$BUILD_DIR"
d2_check_disc

# The real loop bytes are read from the owner's disc at test time and kept in
# the build directory only (never in source).
DISC_LOOP="$BUILD_DIR/map15_pkg5_child_loop.bin"
python3 - "$DISC_LOOP" <<'PY'
import hashlib, sys
from pathlib import Path
sys.path.insert(0, 'tools/scripts/psx')
import scan_field_anim_opcodes as m
disc = Path('disc/disc1.bin')
if not disc.exists():
    print('SAVEPT disc/disc1.bin absent: disc scenario skipped')
    Path(sys.argv[1]).unlink(missing_ok=True)
    raise SystemExit(0)
a = m.DiscArchive(disc)
f = a.read_entry(m.FIELD_MAP_FIRST_ENTRY + 15 * 2)
sd = m.lzss_decompress(f[m.u32(f, 0x13C):])
offs = [m.u32(sd, 4 + i * 4) for i in range(m.u32(sd, 0))]
pk = sd[offs[5]:offs[6] if len(offs) > 6 else len(sd)]
# animation 0 spawns its child with E0 at +0x3F; the child record's script
# starts at record + u16(record+2) + 2 and its loop begins at the A3 opcode.
ao = m.u32(pk, 4)
start0 = ao + m.u16(pk, ao + 2)
start0 = start0 + m.u16(pk, start0 + 2) + 2
e0 = pk.index(b'\xe0', start0)
rec = e0 + 1 + m.s16(pk, e0 + 1)
child = rec + m.u16(pk, rec + 2) + 2
loop = pk.index(b'\xa3', child)
end = pk.index(b'\xe1', loop) + 3
code = pk[loop:end]
assert code[2:4] == b'\xbc\x96', code[:4].hex()
target = (end - 3) + m.s16(pk, end - 2)
assert target == loop + 2, (hex(target), hex(loop))
digest = hashlib.sha256(code).hexdigest()
assert digest == '2c419d3b9853d7be8dd3d30d301c91ff04f8d9b5a8b72d5e1b965f03cea7ed2f', digest
Path(sys.argv[1]).write_bytes(code)
print(f'SAVEPT disc loop map15/pkg5 child +0x{loop:x}..+0x{end:x} ({len(code)} bytes) sha256 {digest[:16]}')
PY
ARGS=()
[[ -f "$DISC_LOOP" ]] && ARGS+=("$DISC_LOOP")

TEST_SRC=pc_port/tests/savepoint_bob_retail_test.c
UUT_SRC="${SAVEPT_TEMP1:-src/slus_006.64/system/temp1.c}"
WEAKEN=(--weaken-symbol=func_800BA8F4 --weaken-symbol=func_800BC158 --weaken-symbol=func_800C11CC)
TEST_ARGS=("${ARGS[@]}")

build_body() { # name flags... -> $BUILD_DIR/name.anim.o
    local name=$1; shift
    "$CC" "${D2_BASE[@]}" "${D2_INC[@]}" -w "$@" \
        -c src/slus_006.64/system/animation_scripts.c -o "$BUILD_DIR/$name.anim.o"
}

regime() { # name flags...
    local name=$1; shift
    build_body "$name" "$@"
    EXTRA_OBJS="$BUILD_DIR/$name.anim.o" d2_build_run "$name" "$UUT_SRC" "$@"
}

rc=0
regime O0 -O0 -g || rc=$?
cat "$BUILD_DIR/O0.stdout"
if [[ -n "${SAVEPT_TEMP1:-}" ]]; then
    TEST_ARGS=("${ARGS[@]}" --native-only --frames=${SAVEPT_FRAMES:-5400} --trace=${SAVEPT_TRACE:-0})
    EXTRA_OBJS="$BUILD_DIR/O0.anim.o" d2_build_run O0native "$UUT_SRC" -O0 -g || true
    cat "$BUILD_DIR/O0native.stdout"
    echo "SAVEPT override $UUT_SRC: differential rc=$rc"
    exit 0
fi
[[ $rc -eq 0 ]] || { cat "$BUILD_DIR/O0.stderr" >&2; exit 1; }
regime O2 -O2 || { cat "$BUILD_DIR/O2.stdout" "$BUILD_DIR/O2.stderr" >&2; exit 1; }
regime UBSan -O1 -g -fsanitize=undefined -fno-sanitize=shift-base -fno-sanitize-recover=all \
    || { cat "$BUILD_DIR/UBSan.stdout" "$BUILD_DIR/UBSan.stderr" >&2; exit 1; }
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
echo "SAVEPT BOB O0/O2/UBSAN PASS"

# Negative controls: each must make the differential go red.
mutant() { # name file fn_regex sed_expr
    local name=$1 file=$2 fn=$3 expr=$4 r=0
    sed "/$fn/,/^}/ { $expr; }" "$file" > "$BUILD_DIR/$name.c"
    if cmp -s "$file" "$BUILD_DIR/$name.c"; then
        echo "MUTANT NO-OP (sed did not change source): $name" >&2; exit 1
    fi
    if [[ "$file" == *animation_scripts.c ]]; then
        "$CC" "${D2_BASE[@]}" "${D2_INC[@]}" -w -O0 -g -c "$BUILD_DIR/$name.c" -o "$BUILD_DIR/$name.anim.o"
        EXTRA_OBJS="$BUILD_DIR/$name.anim.o" d2_build_run "$name" "$UUT_SRC" -O0 -g || r=$?
    else
        EXTRA_OBJS="$BUILD_DIR/O0.anim.o" d2_build_run "$name" "$BUILD_DIR/$name.c" -O0 -g || r=$?
    fi
    if [[ $r -eq 0 ]]; then echo "MUTANT NOT DETECTED: $name" >&2; exit 1; fi
    echo "  mutant rejected: $name ($(grep -m1 'SAVEPT DIVERGE' "$BUILD_DIR/$name.stdout" | cut -c1-120))"
}
# r6.7.2's invented 0xB4 inline wait (+4 advance, no push).
mutant invented_b4_wait "$UUT_SRC" '^void func_800248D4(' \
    's/^    if (opcode == 0xB3) {/    if (opcode == 0xB4) { *(u32*)(pData + 0x64) = (u32)(uintptr_t)(pc + 4); *(s16*)(pData + 0x9E) = *(u16*)(pData + 0x9E) + pc[1] + 2; return; }\n    if (opcode == 0xB3) {/'
mutant fly_shift "$UUT_SRC" '^void func_80022B2C(' 's/vel >> 4) << 4)/vel >> 4) << 3)/'
mutant a6_scale src/slus_006.64/system/animation_scripts.c '^void func_8001FBE4(' 's/value = (value >> 12) << 16;/value = (value >> 12) << 15;/'
mutant snap_y src/slus_006.64/system/animation_scripts.c '^void func_8001FBE4(' 's/vec.vy = \*(s16\*)(pParent + 0x6);/vec.vy = *(s16*)(pParent + 0x6) - 1;/'
echo "SAVEPT BOB negative controls PASS"
