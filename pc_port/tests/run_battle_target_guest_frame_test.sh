#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-guest-frame.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in pc_port/tests/battle_target_guest_frame_test.c pc_port/src/battle_mips_adapter.c \
        pc_port/src/battle_target_eligibility_ram.c pc_port/src/battle_target_list_ram.c; do
        cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
            -c "$source" -o "$out/$opt.$(basename "$source").o"
    done
    clang -no-pie "${flags[@]}" "$out/$opt.battle_target_guest_frame_test.c.o" \
        "$out/$opt.battle_mips_adapter.c.o" "$out/$opt.battle_target_eligibility_ram.c.o" \
        "$out/$opt.battle_target_list_ram.c.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
for mutant in signed_rank actor_zero omit_restore nested_ra stale_owner nonwrapping_found; do
    case "$mutant" in
        signed_rank) expression='s/#define XBT_LIST_RANK(i) read_word/#define XBT_LIST_RANK(i) (int16_t)read_word/' ;;
        actor_zero) expression='s/list_body(\&r,cpu->gpr\[4\])/list_body(\&r,0)/' ;;
        omit_restore) expression='/for (unsigned reg=20;/,+1d' ;;
        nested_ra) expression='s/0x80084AB0/0x80084AB4/' ;;
        stale_owner) expression='/^        s->owner=read_word/d' ;;
        nonwrapping_found) expression='/static void after_selection_list.*{/a\    s->cpu->gpr[16] \&= 0x7FFFFFFFu;' ;;
    esac
    sed "$expression" pc_port/src/battle_target_list_ram.c > "$out/$mutant.c"
    if cmp -s pc_port/src/battle_target_list_ram.c "$out/$mutant.c"; then
        echo "PACKED LIST mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Ipc_port/src -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie "$out/O2.battle_target_guest_frame_test.c.o" \
        "$out/O2.battle_mips_adapter.c.o" "$out/O2.battle_target_eligibility_ram.c.o" \
        "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "PACKED LIST mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'PACKED LIST mismatch|Assertion.*failed' "$out/$mutant.log"
    echo "PACKED LIST mutant rejected: $mutant"
done
