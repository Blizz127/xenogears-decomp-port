#!/usr/bin/env bash
# Compile-only isolation test for the setjmp/-Wclobbered guard in
# battle_results_gate.inc (results_try_host).
#
# GCC >= 14 at -O2 reports `index` "might be clobbered by longjmp" even though
# it is never written after setjmp.  Before it was declared volatile, every
# test that includes battle_mips_runtime.c at -O2 -Wall -Wextra -Werror was red
# on those compilers.  This builds the real runtime TU alone, with the same
# flags those tests use, and then a mutant with the qualifier removed, which
# must fail on -Wclobbered.  No warning is disabled; nothing is linked or run.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/battle_runtime_clobber_test.XXXXXXXX)
echo "BATTLE RUNTIME CLOBBER artifacts: $OUT"
CC=${CC:-gcc}
python3 tools/scripts/gen_battle_bridge_map.py \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --out "$OUT/battle_bridge_map.inc" >/dev/null
flags=(-std=gnu17 -O2 -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT
       -DSKIP_ASM -D_LANGUAGE_C -include assert.h
       -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
       -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
       -Wall -Wextra -Werror)
guarded='    volatile size_t index;'
[ "$(grep -cxF "$guarded" pc_port/src/battle_results_gate.inc)" = 1 ]
printf '#include "%s/pc_port/src/battle_mips_runtime.c"\n' "$PWD" > "$OUT/real.c"
if ! "$CC" "${flags[@]}" -c "$OUT/real.c" -o "$OUT/real.o" 2> "$OUT/real.log"; then
    cat "$OUT/real.log" >&2
    echo "BATTLE RUNTIME CLOBBER FAIL: runtime TU red at -O2 -Werror ($CC)" >&2
    exit 1
fi
# Mutant: the same TU with the qualifier dropped.  Quoted includes resolve
# beside the including file first, so a shadow src/ with its own .inc copies
# wins; ../extern is linked so the runtime's relative PsyCross include holds.
mkdir -p "$OUT/src"
ln -s "$PWD/pc_port/extern" "$OUT/extern"
cp pc_port/src/battle_mips_runtime.c pc_port/src/*.inc "$OUT/src/"
sed -i "s/^$guarded\$/    size_t index;/" "$OUT/src/battle_results_gate.inc"
! grep -qxF "$guarded" "$OUT/src/battle_results_gate.inc"
major=0
if ! "$CC" --version | head -1 | grep -qi clang; then
    major=$("$CC" -dumpfullversion | cut -d. -f1)
fi
if [ "$major" -lt 14 ]; then
    echo "BATTLE RUNTIME CLOBBER PASS real TU ($CC); negative control NOT RUN: needs gcc >= 14"
    exit 0
fi
if "$CC" "${flags[@]}" -c "$OUT/src/battle_mips_runtime.c" \
        -o "$OUT/mutant.o" 2> "$OUT/mutant.log"; then
    echo "BATTLE RUNTIME CLOBBER FAIL mutant compiled; control does not detect the warning" >&2
    exit 1
fi
grep -q "index.*-Werror=clobbered" "$OUT/mutant.log"
echo "BATTLE RUNTIME CLOBBER PASS real TU ($CC $major, -O2 -Werror); negative control red on -Wclobbered"
