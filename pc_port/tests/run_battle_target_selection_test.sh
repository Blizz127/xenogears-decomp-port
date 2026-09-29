#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-selection.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_selection_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c src/battle/main35.c -o "$out/$opt.body.o"
    objcopy --weaken-symbol=func_800841E0 "$out/$opt.body.o"
    bodies=("$out/$opt.body.o")
    # Oracle mode explicitly tests retail only; the production TU still has
    # strong references to the not-yet-decompiled target from its caller.
    if [ "${1:-}" = --oracle-only ]; then bodies=(); fi
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" \
        "$out/$opt.cpu.o" "${bodies[@]}" -o "$out/$opt.test"
    "$out/$opt.test" "$@" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
if [ "${1:-}" = --oracle-only ]; then exit 0; fi
# Expand includes before mutation, and verify the expanded baseline itself.
cc -std=gnu17 -O2 -E -P -DXENO_PC_PORT -DSKIP_ASM \
    -Iinclude -Ipc_port/include_shim src/battle/main35.c \
    -o "$out/mutation-baseline.c"
cc -std=gnu17 -O2 -ffunction-sections -fdata-sections \
    -c "$out/mutation-baseline.c" -o "$out/mutation-baseline.o"
objcopy --weaken-symbol=func_800841E0 "$out/mutation-baseline.o"
clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
    "$out/mutation-baseline.o" -o "$out/mutation-baseline.test"
"$out/mutation-baseline.test" 2>"$out/mutation-baseline.stderr" \
    | tee "$out/mutation-baseline.log"
test ! -s "$out/mutation-baseline.stderr"
for mutant in stride empty_fallback stale_owner stale_target missing_prewrite last_entry; do
    case "$mutant" in
        stride) expression='s/rows\[actor\].target/rows[(actor + 1) \& 255].target/g' ;;
        empty_fallback) expression='s/if (found == 0)/if (found == 0 \&\& D_800D3274 != 0)/' ;;
        stale_owner) expression='/s32 found = 0;/a\    u8 *old_owner = (u8 *)D_800C3EAC;
s/((u8 \*)D_800C3EAC)\[0x2E8\] = D_800C3E90\[0\];/old_owner[0x2E8] = D_800C3E90[0];/' ;;
        stale_target) expression='/u32 actor = arg0 \& 0xFF;/a\    u8 saved_target = D_800C3EAC->rows[actor].target;
s/D_800C3E90\[i\] == D_800C3EAC->rows\[actor\].target/D_800C3E90[i] == saved_target/' ;;
        missing_prewrite) expression='/\[0x2E8\] = D_800C3EAC->rows\[actor\].target;/d' ;;
        last_entry) expression='s/i < D_800D3274/i + 1 < D_800D3274/' ;;
    esac
    sed "/^void func_80084A7C(u32 arg0) {/,/^}/ { $expression
}" "$out/mutation-baseline.c" > "$out/$mutant.c"
    if cmp -s "$out/mutation-baseline.c" "$out/$mutant.c"; then
        echo "TARGET SELECTION mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$out/$mutant.c" -o "$out/$mutant.o"
    objcopy --weaken-symbol=func_800841E0 "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
        "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "TARGET SELECTION mutant survived: $mutant" >&2; exit 1
    fi
    rg -q '^TARGET SELECTION FAIL (native differs from retail|callee must observe)' "$out/$mutant.log"
    echo "TARGET SELECTION mutant rejected: $mutant"
done
