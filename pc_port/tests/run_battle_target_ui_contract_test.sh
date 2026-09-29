#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-ui.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_ui_contract_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -DXBT_UI_NATIVE \
        -c pc_port/tests/battle_target_ui_contract_test.c -o "$out/$opt.native.o"
    clang -no-pie "${flags[@]}" "$out/$opt.native.o" "$out/$opt.cpu.o" -o "$out/$opt.native.test"
    "$out/$opt.native.test" 2>"$out/$opt.native.stderr" | tee "$out/$opt.native.log"
    test ! -s "$out/$opt.native.stderr"
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Dfunc_800BC404=PcPortUiNativeDisplay \
        -Iinclude -Ipc_port/include_shim -include pc_port/tests/battle_target_display_api.h \
        -c src/battle/mainc114.c -o "$out/$opt.display.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -DXBT_UI_NATIVE -DXBT_UI_REAL_DISPLAY \
        -c pc_port/tests/battle_target_ui_contract_test.c -o "$out/$opt.integrated.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.integrated.o" "$out/$opt.cpu.o" \
        "$out/$opt.display.o" -o "$out/$opt.integrated.test"
    "$out/$opt.integrated.test" 2>"$out/$opt.integrated.stderr" | tee "$out/$opt.integrated.log"
    test ! -s "$out/$opt.integrated.stderr"
done
cc -std=gnu17 -E -P -DXENO_PC_PORT -DSKIP_ASM -Dfunc_800BC404=PcPortUiNativeDisplay \
    -Iinclude -Ipc_port/include_shim -include pc_port/tests/battle_target_display_api.h \
    src/battle/mainc114.c -o "$out/display-expanded.c"
for mutant in baseline drop_copy recheck_flag stale_source corrupt_source unconditional skip_nonzero_copy; do
    case "$mutant" in
        baseline) expression='' ;;
        drop_copy) expression='s/D_80059454\[0\] = D_800C3CDC\[0\];/(void)0;/' ;;
        recheck_flag) expression='s/func_800BC460(mask);/if (D_800C37C8[0] == 0) func_800BC460(mask);/' ;;
        stale_source) expression='s/void PcPortUiNativeDisplay(u32 mask) {/void PcPortUiNativeDisplay(u32 mask) { u16 saved_source = D_800C3CDC[0];/; s/D_80059454\[0\] = D_800C3CDC\[0\];/D_80059454[0] = saved_source;/' ;;
        corrupt_source) expression='s/D_80059454\[0\] = D_800C3CDC\[0\];/D_80059454[0] = D_800C3CDC[0]; D_800C3CDC[0] ^= 1;/' ;;
        unconditional) expression='s/if (D_800C37C8\[0\] == 0)/if (1)/' ;;
        skip_nonzero_copy) expression='s/D_80059454\[0\] = D_800C3CDC\[0\];/if (D_800C37C8[0] == 0) D_80059454[0] = D_800C3CDC[0];/' ;;
    esac
    sed "/^void PcPortUiNativeDisplay(/,/^}/ { $expression
}" "$out/display-expanded.c" > "$out/display-$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/display-expanded.c" "$out/display-$mutant.c"; then
        echo "UI display mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -ffunction-sections -fdata-sections \
        -c "$out/display-$mutant.c" -o "$out/display-$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.integrated.o" "$out/O2.cpu.o" \
        "$out/display-$mutant.o" -o "$out/display-$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/display-$mutant.test" 2>"$out/display-$mutant.stderr" | tee "$out/display-$mutant.log"
        test ! -s "$out/display-$mutant.stderr"
    else
        if "$out/display-$mutant.test" >"$out/display-$mutant.log" 2>&1; then
            echo "UI integrated display mutant survived: $mutant" >&2; exit 1
        fi
        case "$mutant" in
            unconditional) pattern='bridge: Assertion.*entry_gate==0.*failed' ;;
            corrupt_source|recheck_flag) pattern='Assertion.*D_800C3CDC\[0\]==display_value.*failed' ;;
            *) pattern='Assertion.*D_80059454\[0\]==display_value.*failed' ;;
        esac
        rg -q "$pattern" "$out/display-$mutant.log"
        echo "UI integrated display mutant rejected: $mutant"
    fi
done
cc -std=gnu17 -E -P -Ipc_port/src -DXBT_UI_NATIVE \
    pc_port/tests/battle_target_ui_contract_test.c -o "$out/native-expanded.c"
for mutant in baseline actor_mask stale_owner early_write early_event entry_owner bad_result; do
    case "$mutant" in
        baseline) expression='' ;;
        actor_mask) expression='s/arg0 \& 0xFF/arg0 \& 0x7F/g' ;;
        stale_owner) expression='s/row\[0x3C\] = selected;/((u8 *)(ram+bases[0]))[offset + 0x3C] = selected;/' ;;
        early_write) expression='s/D_800D3014 = 8;/((u8 *)D_800C3EAC)[offset + 0x3C] = selected; D_800D3014 = 8;/' ;;
        early_event) expression='s/selected = func_80084854/D_800D3014 = 8; selected = func_80084854/g' ;;
        entry_owner) expression='s/u16 own_mask =/D_800C3EAC = (struct BattleCommandContext *)(ram+bases[1]); u16 own_mask =/g' ;;
        bad_result) expression='s/return status \& 0xFF;/return 0;/' ;;
    esac
    sed "/^u32 func_80084B40(/,/^}/ { $expression
}" "$out/native-expanded.c" > "$out/native-$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/native-expanded.c" "$out/native-$mutant.c"; then
        echo "UI mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -c "$out/native-$mutant.c" -o "$out/native-$mutant.o"
    clang -no-pie "$out/native-$mutant.o" "$out/O2.cpu.o" -o "$out/native-$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/native-$mutant.test" 2>"$out/native-$mutant.stderr" | tee "$out/native-$mutant.log"
        test ! -s "$out/native-$mutant.stderr"
    else
        if "$out/native-$mutant.test" >"$out/native-$mutant.log" 2>&1; then
            echo "UI native mutant survived: $mutant" >&2; exit 1
        fi
        rg -q 'Assertion.*(cpu->gpr\[4\]|memcmp|func_80084B40|D_800D3014==call->entry_event|D_800C3EAC==ram\+bases\[call->entry_owner\]).*failed' "$out/native-$mutant.log"
        echo "UI native mutant rejected: $mutant"
    fi
done
for mutant in BAD_CANCEL STALE_OWNER EARLY_WRITE; do
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -D"XBT_UI_$mutant" \
        -c pc_port/tests/battle_target_ui_contract_test.c -o "$out/$mutant.o"
    clang -no-pie "$out/$mutant.o" "$out/O2.cpu.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
        echo "UI mutant survived: $mutant" >&2; exit 1
    fi
    case "$mutant" in
        STALE_OWNER) pattern='Assertion.*stack \|\| event \|\| target.*failed' ;;
        *) pattern='Assertion.*terminal_event!=5.*failed' ;;
    esac
    rg -q "$pattern" "$out/$mutant.log"
    echo "UI mutant rejected: $mutant"
done
