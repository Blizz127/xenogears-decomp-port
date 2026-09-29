#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
SOURCE=${BATTLE_OVERLAY_LEAF_SOURCE:-"$PWD/pc_port/src/battle_mips_runtime.c"}
SOURCE=$(realpath "$SOURCE")
OUT=${BATTLE_OVERLAY_LEAF_OUT:-$(mktemp -d /tmp/xeno-overlay-leaf.XXXXXXXX)}
mkdir -p "$OUT"
printf 'BATTLE OVERLAY LEAF source=%s artifacts=%s\n' "$SOURCE" "$OUT"
bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in ${BATTLE_OVERLAY_LEAF_MODES:-O0 O2 UBSan}; do
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all -fno-sanitize=function); fi
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$SOURCE\"" \
        -c pc_port/tests/battle_overlay_leaf_bridge_test.c -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    # battle_mips_runtime.c calls PcPort_GodModeBeforeGuest() on every guest
    # entry, so god_mode.c has to be in the link or the fixture fails to build.
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/god_mode.c -o "$OUT/$mode.godmode.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.godmode.o" \
        -ldl -o "$OUT/$mode"
    status=0
    "$OUT/$mode" >"$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    if [ "$status" -ne 0 ] || ! grep -q '^BATTLE OVERLAY LEAF PASS ' "$OUT/$mode.log"; then
        exit 1
    fi
done
# Negative control: restore the old "always interpret overlay" early return.
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import sys
src = Path(sys.argv[1]).read_text()
out = Path(sys.argv[2])
old = """    if (target == 0x801e6ce8u) {
        int adopted = file1_try_controller(runtime, cpu);
        if (adopted != 0) return adopted;
    }

    resolved = NULL;
"""
new = """    if (target == 0x801e6ce8u) {
        int adopted = file1_try_controller(runtime, cpu);
        if (adopted != 0) return adopted;
    }
    if (target_is_guest_code(target))
        return 0;

    resolved = NULL;
"""
assert src.count(old) == 1, src.count(old)
(out / 'always-interpret.c').write_text(src.replace(old, new, 1))
PY
clang "${COMMON[@]}" -O2 -Wall -Wextra -Werror \
    "-DBATTLE_RUNTIME_SOURCE=\"$OUT/always-interpret.c\"" \
    -c pc_port/tests/battle_overlay_leaf_bridge_test.c -o "$OUT/mutant.test.o"
clang -no-pie -O2 -Wl,--gc-sections \
    "$OUT/mutant.test.o" "$OUT/O2.cpu.o" "$OUT/O2.godmode.o" -ldl -o "$OUT/mutant"
status=0
"$OUT/mutant" >"$OUT/mutant.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q '^BATTLE OVERLAY LEAF FAIL ' "$OUT/mutant.log"; then
    cat "$OUT/mutant.log" >&2
    echo "BATTLE OVERLAY LEAF CONTROL FAILURE always-interpret status=$status" >&2
    exit 1
fi
echo "BATTLE OVERLAY LEAF PASS (O0/O2/UBSan + always-interpret control)"
