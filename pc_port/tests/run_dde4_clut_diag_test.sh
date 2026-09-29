#!/usr/bin/env bash
# Differential regression test for func_8002DDE4 (SLUS multi-block VRAM
# uploader) and its XENO_FIELD_DIAG CLUT readback: retail bytes on the MIPS
# interpreter vs the native body extracted verbatim from
# pc_port/src/game_overrides.c, at -O0 / -O2 (both -fstack-protector-all) and
# ASan+UBSan, then deliberately mutated native bodies that MUST compile and
# MUST then be rejected at runtime (a mutant that fails to compile is reported
# as a harness error, never counted as a rejection).
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/dde4_clut_diag_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
d = open('disc/SLUS_006.64', 'rb').read()
assert sha256(d).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119', 'SLUS_006.64 hash'
s = d[0x1e5e4:0x1e5e4 + 0x1fc]
assert sha256(s).hexdigest() == '758f48b55e8638ed3f177d4aad90c94501937acd9b6f690ae2e98ca6c45e4d8e', 'func_8002DDE4 slice hash'
print('SLUS_006.64 ok; func_8002DDE4 [8002DDE4,8002DFE0) sha256', sha256(s).hexdigest())
PY
extract_body() {   # source, destination
    sed -n '/^s32 func_8002DDE4(void\* pImageData, s32 texMode, s32 texX, s32 texY,$/,/^}/p' "$1" > "$2"
    if ! grep -q 'clutBuf' "$2" || ! grep -q '^}' "$2"; then
        echo "DDE4 harness error: could not extract func_8002DDE4 from $1" >&2; exit 1
    fi
}
extract_body pc_port/src/game_overrides.c "$OUT/body.c"
common=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -fpermissive -w)
CC="${CC:-gcc}"
build_test() {   # tag, body, flags...
    local tag=$1 body=$2; shift 2
    "$CC" "${common[@]}" "$@" "-DDDE4_BODY=\"$body\"" \
        -c pc_port/tests/dde4_clut_diag_test.c -o "$OUT/$tag.test.o" &&
    "$CC" -std=gnu17 "$@" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$tag.cpu.o" &&
    "$CC" "$@" "$OUT/$tag.test.o" "$OUT/$tag.cpu.o" -o "$OUT/$tag.test"
}
flags_for() {
    case "$1" in
        O0)   echo "-O0 -fstack-protector-all" ;;
        O2)   echo "-O2 -fstack-protector-all" ;;
        ASan) echo "-O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector-all" ;;
    esac
}
for opt in O0 O2 ASan; do
    # shellcheck disable=SC2046
    build_test "$opt" "$(pwd)/$OUT/body.c" $(flags_for "$opt")
    if ! "$OUT/$opt.test" 2> "$OUT/$opt.stderr.log"; then
        echo "DDE4 FAIL [$opt]" >&2; tail -n 20 "$OUT/$opt.stderr.log" >&2; exit 1
    fi
done
# Negative controls: each mutant is applied to the extracted body only, must
# compile at O2 and under ASan, and must be rejected at runtime by BOTH builds.
for mutant in oldbuf guard stride mode2 badmagic clutmode payload; do
    case "$mutant" in
        oldbuf)   expression='s/u16 clutBuf\[256 \* 4\];/u16 clutBuf[256];/' ;;
        guard)    expression='s/rect.h > 0 \&\& rect.h <= 4) {/rect.h > 0 \&\& rect.h <= 8) {/' ;;
        stride)   expression='s/^        pBlock += 8;$/        pBlock += 6;/' ;;
        mode2)    expression='s/rect.x = baseX + texX + offsX;/rect.x = texX + offsX;/' ;;
        badmagic) expression='s/^            return 1;$/            return 0;/' ;;
        clutmode) expression='s/if (clutMode16 == 1) {/if (clutMode16 == 3) {/' ;;
        payload)  expression='s/pBlock += (s16)rect.w \* (s16)rect.h \* 2;/pBlock += (s16)rect.w * (s16)rect.h;/' ;;
    esac
    sed "$expression" "$OUT/body.c" > "$OUT/$mutant.c"
    if cmp -s "$OUT/$mutant.c" "$OUT/body.c"; then
        echo "DDE4 harness error: mutant did not apply: $mutant" >&2; exit 1
    fi
    for opt in O2 ASan; do
        # shellcheck disable=SC2046
        if ! build_test "$mutant.$opt" "$(pwd)/$OUT/$mutant.c" $(flags_for "$opt") 2> "$OUT/$mutant.$opt.cc.log"; then
            echo "DDE4 harness error: mutant failed to COMPILE (not a rejection): $mutant [$opt]" >&2
            cat "$OUT/$mutant.$opt.cc.log" >&2; exit 1
        fi
        rc=0
        "$OUT/$mutant.$opt.test" > "$OUT/$mutant.$opt.log" 2>&1 || rc=$?
        if [ "$rc" -eq 0 ]; then
            echo "DDE4 FAIL mutant NOT rejected: $mutant [$opt]" >&2
            tail -n 5 "$OUT/$mutant.$opt.log" >&2; exit 1
        fi
        echo "DDE4 mutant rejected: $mutant [$opt] rc=$rc -- $(grep -a -m1 -E 'DDE4 FAIL|stack smashing|AddressSanitizer|runtime error' "$OUT/$mutant.$opt.log" | cut -c1-100)"
    done
done
echo "DDE4 ALL PASS (O0/O2/ASan; 7 mutants rejected under O2 and ASan)"
