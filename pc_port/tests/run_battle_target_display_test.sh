#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-display.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
cc -std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
    -include pc_port/tests/battle_target_display_api.h -fsyntax-only src/battle/main35.c
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_display_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -include pc_port/tests/battle_target_display_api.h \
        -c src/battle/mainc114.c -o "$out/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" "$out/$opt.cpu.o" \
        "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
cc -std=gnu17 -E -P -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
    -include pc_port/tests/battle_target_display_api.h src/battle/mainc114.c -o "$out/expanded.c"
for mutant in baseline truncate_mask recheck_flag stale_source; do
    case "$mutant" in
        baseline) expression='' ;;
        truncate_mask) expression='s/func_800BC460(mask);/func_800BC460((u16)mask);/' ;;
        recheck_flag) expression='s/func_800BC460(mask);/if (D_800C37C8[0] == 0) func_800BC460(mask);/' ;;
        stale_source) expression='s/void func_800BC404(u32 mask) {/void func_800BC404(u32 mask) { u16 saved_source = D_800C3CDC[0];/; s/D_80059454\[0\] = D_800C3CDC\[0\];/D_80059454[0] = saved_source;/' ;;
    esac
    sed "/^void func_800BC404(/,/^}/ { $expression
}" "$out/expanded.c" > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/expanded.c" "$out/$mutant.c"; then
        echo "DISPLAY mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -ffunction-sections -fdata-sections -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/$mutant.o" "$out/O2.test.o" "$out/O2.cpu.o" -o "$out/$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/$mutant.test" 2>"$out/$mutant.stderr" | tee "$out/$mutant.log"
        test ! -s "$out/$mutant.stderr"
    else
        if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
            echo "DISPLAY mutant survived: $mutant" >&2; exit 1
        fi
        rg -q 'Assertion.*(a==|calls==|D_80059454\[0\]).*failed' "$out/$mutant.log"
        echo "DISPLAY mutant rejected: $mutant"
    fi
done
