#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-setup-runtime.XXXXXX)
echo "Evidence: $out"
sha256sum pc_port/src/battle_mips_runtime.c pc_port/src/battle_mips_adapter.c \
    pc_port/src/battle_target_setup.c pc_port/src/battle_target_setup.h \
    pc_port/tests/battle_target_setup_runtime_test.c \
    pc_port/tests/run_battle_target_setup_runtime_test.sh \
    tools/scripts/gen_battle_bridge_map.py config/symbol_addrs.slus_006.64.txt \
    linker/undefined_funcs_auto.battle.txt linker/undefined_syms_auto.battle.txt > "$out/source-pins.sha256"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
python3 tools/scripts/gen_battle_bridge_map.py \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt --out "$out/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -I"$out" -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
    -Wall -Wextra -Werror)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc "${common[@]}" "${flags[@]}" -c pc_port/tests/battle_target_setup_runtime_test.c -o "$out/$opt.test.o"
    cc "${common[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    cc "${common[@]}" "${flags[@]}" -c pc_port/src/battle_target_setup.c -o "$out/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections -Wl,--export-dynamic \
        "$out/$opt.test.o" "$out/$opt.cpu.o" "$out/$opt.body.o" -ldl -o "$out/$opt.test"
    "$out/$opt.test" >"$out/$opt.log" 2>"$out/$opt.stderr"
    rg -q '^TARGET SETUP runtime composition PASS' "$out/$opt.log"
    test "$(wc -l < "$out/$opt.stderr")" -eq 1
    rg -q '^\[xeno-port\]\[battle-mips\] retail adapter ready:' "$out/$opt.stderr"
    tail -1 "$out/$opt.log"
done
for mutant in MASK_ARGUMENT RAW_RAM; do
    cc "${common[@]}" -O2 -DXBT_SETUP_"$mutant" \
        -c pc_port/tests/battle_target_setup_runtime_test.c -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections -Wl,--export-dynamic "$out/$mutant.o" \
        "$out/O2.cpu.o" "$out/O2.body.o" -ldl -o "$out/$mutant.test"
    if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
        echo "SETUP runtime mutant survived: $mutant" >&2; exit 1
    fi
    case "$mutant" in
        MASK_ARGUMENT) pattern='Assertion.*==marker.*failed' ;;
        RAW_RAM) pattern='Assertion.*memcmp\(expected_ram.*failed' ;;
    esac
    rg -q "$pattern" "$out/$mutant.log"
    echo "SETUP runtime mutant rejected: $mutant"
done
sha256sum -c "$out/source-pins.sha256" > "$out/source-pins-check.log"
