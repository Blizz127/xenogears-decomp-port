#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-full-target-chain.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_chain_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c src/battle/main35.c -o "$out/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" \
        "$out/$opt.cpu.o" "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
# Mutate expanded production source so shared-body includes remain covered.
# Keep the ordinary builds above: the expanded baseline must independently
# pass the same retail comparison before it can be used for negative controls.
cc -std=gnu17 -O2 -E -P -DXENO_PC_PORT -DSKIP_ASM \
    -Iinclude -Ipc_port/include_shim src/battle/main35.c \
    -o "$out/mutation-baseline.c"
cc -std=gnu17 -O2 -ffunction-sections -fdata-sections \
    -c "$out/mutation-baseline.c" -o "$out/mutation-baseline.o"
clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
    "$out/mutation-baseline.o" -o "$out/mutation-baseline.test"
"$out/mutation-baseline.test" 2>"$out/mutation-baseline.stderr" \
    | tee "$out/mutation-baseline.log"
test ! -s "$out/mutation-baseline.stderr"
for mutant in eligibility_status list_candidate selection_first status_bits group_index group_sort wide_selection; do
    case "$mutant" in
        eligibility_status) fn=80083FF4; expression='s/D_800CCD64\[target\]/D_800CCE08[target]/' ;;
        list_candidate) fn=800841E0; expression='s/i < 11/i < 10/; s/i < 3;/i < 2;/' ;;
        selection_first) fn=80084A7C; expression='s/if (found == 0)/if (1)/' ;;
        status_bits) fn=80083FF4; expression='s/0xC001/0x8000/g' ;;
        group_index) fn=800841E0; expression='s/D_800C3EB4\[candidates\[i\]\]\[0\] == D_800C3EB4\[arg0 \& 0xFF\]\[0\]/(candidates[i] \& 3) == (arg0 \& 3)/' ;;
        group_sort) fn=800841E0; expression='/return D_800C3E90\[0\];/i\    if (matching[0]) { s32 a, b; for (a = 0; a < total && matching[a]; a++) for (b = a + 1; b < total && matching[b]; b++) if (D_800CCD34[D_800C3E90[b]][0] < D_800CCD34[D_800C3E90[a]][0]) { u8 t = D_800C3E90[a]; D_800C3E90[a] = D_800C3E90[b]; D_800C3E90[b] = t; } }' ;;
        wide_selection) fn=80084A7C; expression='s/((u8 \*)D_800C3EAC)\[0x2E8\] = D_800C3EAC->rows\[actor\].target;/\*(u32 *)((u8 *)D_800C3EAC + 0x2E8) = *(u32 *)((u8 *)D_800C3EAC + actor * 64 + 0x3C);/' ;;
    esac
    sed "/^\(u32\|void\) func_$fn(/,/^}/ { $expression
}" "$out/mutation-baseline.c" > "$out/$mutant.c"
    if cmp -s "$out/mutation-baseline.c" "$out/$mutant.c"; then
        echo "TARGET FULL CHAIN mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
        "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "TARGET FULL CHAIN mutant survived: $mutant" >&2; exit 1
    fi
    rg -q '^TARGET FULL CHAIN FAIL native differs from retail' "$out/$mutant.log"
    echo "TARGET FULL CHAIN mutant rejected: $mutant"
done
