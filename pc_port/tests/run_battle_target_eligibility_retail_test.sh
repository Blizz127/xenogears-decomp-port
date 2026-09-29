#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-eligibility.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_eligibility_retail_test.c -o "$out/$opt.test.o"
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
# Expand includes before mutation, and verify the expanded baseline itself.
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
for mutant in status_mask status_path matrix_offset matrix_stride reject_all; do
    case "$mutant" in
        status_mask) expression='s/0xC001/0xC000/g' ;;
        status_path) expression='s/D_800CCD64\[target\]/D_800CCE08[target]/' ;;
        matrix_offset) expression='s/0x140/0x141/' ;;
        matrix_stride) expression='s/\* 64/\* 32/' ;;
        reject_all) expression='s/return result;/return 0;/' ;;
    esac
    sed "/^u32 func_80083FF4(u32 arg0, u32 arg1) {/,/^}/ { $expression
}" "$out/mutation-baseline.c" > "$out/$mutant.c"
    if cmp -s "$out/mutation-baseline.c" "$out/$mutant.c"; then
        echo "ELIGIBILITY mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
        "$out/$mutant.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "ELIGIBILITY mutant survived: $mutant" >&2; exit 1
    fi
    rg -q '^ELIGIBILITY FAIL native differs from retail' "$out/$mutant.log"
    echo "ELIGIBILITY mutant rejected: $mutant"
done
