#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-command-sound.XXXXXX)
echo "Evidence: $out"
read -r digest _ < <(dd if=disc/battle.bin bs=1 skip=$((0x1af50)) count=$((0x60)) status=none | sha256sum)
test "$digest" = 23a387c4a275dd2a77a95f7c3c6e0a9bc4439280b9e8c1d690201fca00f03f72
body() {
    cc -std=gnu17 "${flags[@]}" -fno-pie -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DXENO_BATTLE_OVERLAY_HOST_BODIES -DSKIP_ASM -Iinclude -Ipc_port/include_shim -Ipc_port/src -c "$1" -o "$2"
}
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    body src/battle/main43.c "$out/$mode.body.o"
    cc -std=gnu17 "${flags[@]}" -fno-pie -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_command_sound_test.c -o "$out/$mode.test.o"
    cc -std=gnu17 "${flags[@]}" -fno-pie -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$mode.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$mode."*.o -o "$out/$mode.test"
    "$out/$mode.test" | tee "$out/$mode.log"
done
flags=(-O2)
for mutant in gate packed_id guest_pointer; do
    case "$mutant" in
        gate) expression='s/D_800D366C != 0/D_800D366C == 0/' ;;
        packed_id) expression='s/v << 16/v << 8/' ;;
        guest_pointer) expression='s/PSX_ADDR(bank)/(void*)(uintptr_t)bank/' ;;
    esac
    sed "$expression" src/battle/main43.c > "$out/$mutant.c"
    if cmp -s src/battle/main43.c "$out/$mutant.c"; then exit 1; fi
    body "$out/$mutant.c" "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/$mutant.o" "$out/O2.test.o" "$out/O2.cpu.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "COMMAND SOUND mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'COMMAND SOUND FAIL native differs from retail' "$out/$mutant.log"
done
echo 'COMMAND SOUND negative controls PASS gate/packed-id/guest-pointer'
