#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-list.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
# Hide only the predicate definition from same-TU interprocedural analysis.
# Renaming after compilation is too late: GCC can infer its boolean range
# and remove the caller's low-byte mask before the strong test spy is linked.
boundary='s/^u32 func_80083FF4(/u32 unused_eligibility_fixture_body(/'
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    # Expand includes with this build's flags before hiding the definition.
    cc -std=gnu17 "${flags[@]}" -E -P -DXENO_PC_PORT -DSKIP_ASM \
        -Iinclude -Ipc_port/include_shim src/battle/main35.c \
        -o "$out/$opt.expanded.c"
    sed "$boundary" "$out/$opt.expanded.c" > "$out/$opt.controlled.c"
    if cmp -s "$out/$opt.expanded.c" "$out/$opt.controlled.c"; then
        echo "TARGET LIST predicate boundary rename made no change" >&2; exit 1
    fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_list_retail_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    cc -std=gnu17 "${flags[@]}" -fno-inline -fno-ipa-ra -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$out/$opt.controlled.c" -o "$out/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" \
        "$out/$opt.cpu.o" "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
for mutant in chain_skip_builder chain_always_first; do
    case "$mutant" in
        chain_skip_builder) expression='s/func_800841E0(actor);/return;/' ;;
        chain_always_first) expression='s/if (found == 0)/if (1)/' ;;
    esac
    sed "/^void func_80084A7C(u32 arg0) {/,/^}/ { $expression
}" "$out/O2.controlled.c" > "$out/$mutant.c"
    if cmp -s "$out/O2.controlled.c" "$out/$mutant.c"; then
        echo "TARGET CHAIN mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -fno-inline -fno-ipa-ra -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
        "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "TARGET CHAIN mutant survived: $mutant" >&2; exit 1
    fi
    rg -q '^TARGET LIST FAIL native chain differs from retail' "$out/$mutant.log"
    echo "TARGET CHAIN mutant rejected: $mutant"
done
for mutant in predicate group signed_rank ties last_candidate return_zero full_sort; do
    case "$mutant" in
        predicate) expression='s/if (eligible \& 0xFF)/if (eligible)/' ;;
        group) expression='s/D_800C3EB4\[candidates\[i\]\]\[0\] ==/D_800C3EB4[candidates[i]][0] !=/' ;;
        signed_rank) expression='s/D_800CCD34\[D_800C3E90\[0\]\]\[0\] > D_800CCD34\[D_800C3E90\[i\]\]\[0\]/(s16)D_800CCD34[D_800C3E90[0]][0] > (s16)D_800CCD34[D_800C3E90[i]][0]/' ;;
        ties) expression='s/\] > D_800CCD34/] >= D_800CCD34/' ;;
        last_candidate) expression='s/i < 11/i < 10/; s/i < 3;/i < 2;/' ;;
        return_zero) expression='s/return D_800C3E90\[0\];/return 0;/' ;;
        full_sort) expression='/return D_800C3E90\[0\];/i\    { s32 a, b; for (a = 0; a < total; a++) for (b = a + 1; b < total; b++) if (D_800CCD34[D_800C3E90[b]][0] < D_800CCD34[D_800C3E90[a]][0]) { u8 t = D_800C3E90[a]; D_800C3E90[a] = D_800C3E90[b]; D_800C3E90[b] = t; } }' ;;
    esac
    sed "/^u32 func_800841E0(u32 arg0) {/,/^}/ { $expression
}" "$out/O2.controlled.c" > "$out/$mutant.c"
    if cmp -s "$out/O2.controlled.c" "$out/$mutant.c"; then
        echo "TARGET LIST mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -fno-inline -fno-ipa-ra -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
        "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "TARGET LIST mutant survived: $mutant" >&2; exit 1
    fi
    rg -q '^TARGET LIST FAIL native differs from retail' "$out/$mutant.log"
    echo "TARGET LIST mutant rejected: $mutant"
done
