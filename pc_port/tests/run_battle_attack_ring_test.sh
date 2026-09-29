#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-attack-ring.XXXXXX)
echo "Evidence: $out"
check_slice() {
    local offset=$1 size=$2 expected=$3 actual
    read -r actual _ < <(dd if=disc/battle.bin bs=1 skip="$offset" count="$size" status=none | sha256sum)
    test "$actual" = "$expected"
}
check_slice $((0x1166c)) $((0x1bc)) c3639f06d1cb7097d5551490f63c8eea7431404779ce59cbcd108ae38238678f
check_slice $((0x520)) 32 c9be78ab2802b5bc30ad1feede1534b2b4f85e9d4181dc1db277a4adfad9238a
build_body() {
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$1" -o "$2"
    # This harness traces 84A7C; its real body has a separate differential gate.
    objcopy --weaken-symbol=func_80084A7C "$2"
}
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_attack_ring_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    build_body src/battle/main35.c "$out/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" "$out/$opt.cpu.o" \
        "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" | tee "$out/$opt.log"
done
flags=(-O2)
cc -std=gnu17 -O2 -E -P -DXENO_PC_PORT -DSKIP_ASM \
    -Iinclude -Ipc_port/include_shim src/battle/main35.c \
    -o "$out/mutation-baseline.c"
build_body "$out/mutation-baseline.c" "$out/mutation-baseline.o"
clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
    "$out/mutation-baseline.o" -o "$out/mutation-baseline.test"
"$out/mutation-baseline.test" | tee "$out/mutation-baseline.log"
for mutant in cancel stride width repeat stale_accept stale_repeat; do
    case "$mutant" in
        cancel) expression='s/case 4:/case 5:/' ;;
        stride) expression='s/D_800C3EAC->rows\[actor\].target/((BattleCommandActor *)((u8 *)D_800C3EAC + actor * 60))->target/' ;;
        width) expression='s/\.right)/.right \& 0xFF)/; s/\.down)/.down \& 0xFF)/' ;;
        repeat) expression='s/\[0x2F6\] = 0;/[0x2F6] = 1;/' ;;
        stale_accept) expression='/void func_8008115C(u32 arg0) {/a\    u8 *saved_owner = (u8 *)D_800C3EAC;
s/((u8 \*)D_800C3EAC)\[0x2DD\] = 5;/saved_owner[0x2DD] = 5;/' ;;
        stale_repeat) expression='/void func_8008115C(u32 arg0) {/a\    u8 *saved_owner = (u8 *)D_800C3EAC;
s/((u8 \*)D_800C3EAC)\[0x2F6\] = 0;/saved_owner[0x2F6] = 0;/' ;;
    esac
    sed "$expression" "$out/mutation-baseline.c" > "$out/$mutant.c"
    if cmp -s "$out/mutation-baseline.c" "$out/$mutant.c"; then
        echo "ATTACK RING mutation made no change: $mutant" >&2; exit 1
    fi
    build_body "$out/$mutant.c" "$out/$mutant.o"
    cc -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "ATTACK RING mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'ATTACK RING FAIL native (differs from retail|context reload differs from retail)' "$out/$mutant.log"
done
echo 'ATTACK RING negative controls PASS cancel/stride/width/repeat/stale-accept/stale-repeat'
