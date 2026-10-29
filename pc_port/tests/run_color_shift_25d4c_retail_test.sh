#!/usr/bin/env bash
# func_80025D4C's port body (pc_port/src/retail_leaf_adapters.c) against the
# retail handwritten GTE loop 80025D4C..80025FA8 from the user's
# disc/SLUS_006.64, run in the test-only interpreter
# (color_shift_25d4c_retail_test.c), at O0 / O2 / UBSan.  Then mutants of the
# port body, one per behaviour, must each be rejected.
# Output goes to COLOR_SHIFT_OUT (default pc_port/build_native/color_shift_25d4c).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${COLOR_SHIFT_OUT:-pc_port/build_native/color_shift_25d4c}
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
b = Path('disc/SLUS_006.64').read_bytes()
assert sha256(b[0x1654c:0x167a8]).hexdigest() == \
    '8beea0886035c1f0694c1503bae9c336ace1eea3ecafe6ded8adf06fc13a5e67', \
    'retail 80025D4C..80025FA8 differs from the expected bytes'
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
cflags=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/color_shift_25d4c_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaf.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
    status=0
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1 || status=$?
    cat "$OUT/$opt.log"
    if [ "$status" -ne 0 ] || ! grep -q '^COLOR SHIFT PASS ' "$OUT/$opt.log" ||
       grep -q 'runtime error' "$OUT/$opt.log"; then
        echo "COLOR SHIFT FAIL regime $opt (rc=$status)" >&2; exit 1
    fi
done
mutants="clamp halfmask grey zeroskip stp command negclamp start"
for mutant in $mutants; do
    case "$mutant" in
        clamp) expression='/if (factor > 32) factor = 32;/d' ;;
        halfmask) expression='s/(pixel \& 0x03C0) >> 1/(pixel \& 0x03E0) >> 1/' ;;
        grey) expression='s/sum \/ 3u/sum \/ 4u/' ;;
        zeroskip) expression='s/if (pixel != 0)/if (pixel != 1)/' ;;
        stp) expression='s/) | 0x8000)/))/' ;;
        command) expression='s/0x0198003D/0x0190003D/' ;;
        negclamp) expression='/if ((s16)outG < 0) outG = 0;/d' ;;
        start) expression='s/u32 r = 0, g = 0, b = 0;/u32 r = 1, g = 0, b = 0;/' ;;
    esac
    sed "/^void func_80025D4C(/,/^}/ { $expression; }" pc_port/src/retail_leaf_adapters.c > "$OUT/$mutant.c"
    if cmp -s "$OUT/$mutant.c" pc_port/src/retail_leaf_adapters.c; then
        echo "COLOR SHIFT mutant $mutant did not apply" >&2; exit 1
    fi
    gcc "${common[@]}" "${cflags[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
        "$OUT/$mutant.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "COLOR SHIFT mutant survived: $mutant" >&2; exit 1
    fi
    grep -q 'COLOR SHIFT FAIL' "$OUT/$mutant.log"
    echo "MUTANT REJECTED: $mutant ($(grep -m1 'COLOR SHIFT FAIL' "$OUT/$mutant.log" | cut -c1-140))"
done
echo "COLOR SHIFT negative controls PASS: $mutants"
