#!/usr/bin/env bash
# Self-contained runner for the packet-callback host tests
# (battle_triangle_{max,min}_host_test.c, battle_quad_{max,min}_host_test.c).
# Each test compares the original SLUS_006.64 bytes, interpreted, with the
# production host on the real packet caller 8002C700.  Every support object
# is built from this tree's sources (no pinned artefacts):
#   battle_mips_adapter.c (CPU), controller_vblank_service.c, PsyCross's
#   INLINE_C.C / PsyX_GTE.cpp / LIBGTE.C, psyq_compat.c and
#   retail_leaf_adapters.c;
#   func_80089C08 is a stub that aborts (no packet case may reach it).
# PsyCross's doCOP2 is compiled as color_fade_backend_command, as in the
# pinned objects: the test's own doCOP2 records each GTE command and forwards.
# Symbols the runtime source reaches but no packet case uses get
# placeholders: zeroed data, and functions that abort when called.
# The bridge map is this tree's pc_port/build_native/battle_bridge_map.inc, so
# build the port first.
#
# Instrumentation, as the pinned runner did: the production bus definitions
# are renamed (runtime_read / runtime_write -> model_test_raw_*) in a copy of
# battle_mips_runtime.c, so the test can observe and fault every access; no
# body changes.  A mutant run puts a mutated host .inc, under its production
# name, next to that copy (BATTLE_PACKET_HOST_TEST_MUTANT=<file>): the
# copy's quoted include finds it before pc_port/src.
#
# Caller hosts are opt-in in the hybrid build (27facced), so the tests run
# with XENO_BATTLE_NATIVE_HOSTS=1.
#
#   run_battle_packet_host_test.sh TEST.c [test arguments]
# BATTLE_PACKET_HOST_TEST_OUT (required, scratch), BATTLE_PACKET_HOST_TEST_MODES
# (default "O0 O2 UBSan"), BATTLE_PACKET_HOST_TEST_PASS (grep -E pattern the
# log must contain; default the suite's "ALL PASS" line).
set -euo pipefail
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST=${1:?usage: run_battle_packet_host_test.sh TEST.c [args]}; shift
OUT=${BATTLE_PACKET_HOST_TEST_OUT:?set BATTLE_PACKET_HOST_TEST_OUT to a scratch directory}
PASS=${BATTLE_PACKET_HOST_TEST_PASS:-ALL PASS}
BUILD=pc_port/build_native
PSX=pc_port/extern/PsyCross
for f in "$TEST" "$BUILD/battle_bridge_map.inc" disc/SLUS_006.64 "$PSX/src/psx/INLINE_C.C"; do
    [ -f "$f" ] || { echo "PACKET HOST TEST FAIL missing $f (build the port first)" >&2; exit 1; }
done
mkdir -p "$OUT"
export TMPDIR="$OUT/tmp"; mkdir -p "$TMPDIR"

python3 - "$OUT/observed-runtime.c" <<'PY'
import sys
from pathlib import Path
text = Path('pc_port/src/battle_mips_runtime.c').read_text()
for name in ('read', 'write'):
    old = 'static int runtime_' + name + '('
    assert text.count(old) == 1, old
    text = text.replace(old, 'static int model_test_raw_' + name + '(')
Path(sys.argv[1]).write_text(text)
PY
rm -f "$OUT"/battle_*_host.inc
if [ -n "${BATTLE_PACKET_HOST_TEST_MUTANT:-}" ]; then
    cp "$BATTLE_PACKET_HOST_TEST_MUTANT" "$OUT/$(basename "$BATTLE_PACKET_HOST_TEST_MUTANT")"
fi
printf '#include <stdio.h>\n#include <stdlib.h>\nvoid func_80089C08(void);\nvoid func_80089C08(void){fputs("func_80089C08 stub reached\\n",stderr);abort();}\n' > "$OUT/mask.c"

common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -include assert.h -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude
    -Ipc_port/src -I"$BUILD" -I"$PSX/include" -I"$PSX/include/psx")
exports=()
for name in D_80050104 D_800595C0 D_80059308 D_8005930C D_800500F8 D_800500FC D_80050100 D_80059578; do
    exports+=(-Wl,--export-dynamic-symbol="$name")
done
psylib=$(find pc_port/build -name libpsycross.a | head -1)
[ -n "$psylib" ] || { echo "PACKET HOST TEST FAIL missing libpsycross.a" >&2; exit 1; }
(mkdir -p "$OUT/psycross" && cd "$OUT/psycross" && ar x "$OLDPWD/$psylib" LIBGTE.C.o)
name=$(basename "$TEST" .c)
status=0
for mode in ${BATTLE_PACKET_HOST_TEST_MODES:-O0 O2 UBSan}; do
    if [ "$mode" = UBSan ]; then cc=clang; flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    else cc=gcc; flags=(-"$mode"); fi
    "$cc" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -DBATTLE_RUNTIME_SOURCE="\"$OUT/observed-runtime.c\"" -c "$TEST" -o "$OUT/$mode.test.o"
    objs=("$OUT/$mode.test.o")
    for src in pc_port/src/battle_mips_adapter.c pc_port/src/controller_vblank_service.c \
               pc_port/src/psyq_compat.c pc_port/src/retail_leaf_adapters.c "$OUT/mask.c"; do
        o="$OUT/$mode.$(basename "$src").o"
        "$cc" "${common[@]}" "${flags[@]}" -w -c "$src" -o "$o"; objs+=("$o")
    done
    for src in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        o="$OUT/$mode.$(basename "$src").o"; rename=()
        [ "$src" = psx/INLINE_C.C ] && rename=(-DdoCOP2=color_fade_backend_command)
        g++ -std=c++17 -fno-pie "${flags[@]}" -fpermissive -w -DUSE_EXTENDED_PRIM_POINTERS=0 \
            "${rename[@]}" -I"$PSX/include" -I"$PSX/include/psx" -include pc_port/src/port_compat.h \
            -c "$PSX/src/$src" -o "$o"; objs+=("$o")
    done
    objs+=("$OUT/psycross/LIBGTE.C.o")
    if ! clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "${exports[@]}" "${objs[@]}" -ldl -lcrypto \
            -o "$OUT/$mode.$name" 2> "$OUT/$mode.link.err"; then
        missing=$(grep -oE "undefined reference to \`[A-Za-z0-9_]+'" "$OUT/$mode.link.err" \
            | sed "s/.*\`//; s/'//" | sort -u)
        [ -n "$missing" ] || { cat "$OUT/$mode.link.err" >&2; exit 1; }
        {
            echo '#include <stdio.h>'
            echo '#include <stdlib.h>'
            for sym in $missing; do
                case "$sym" in
                    D_*|g_*|jtbl_*|jpt_*|s_*) echo "unsigned char $sym[0x10000];" ;;
                    *) echo "void $sym(void);"
                       echo "void $sym(void) { fputs(\"placeholder $sym reached\\n\", stderr); abort(); }" ;;
                esac
            done
        } > "$OUT/$mode.placeholders.c"
        gcc -w -c "$OUT/$mode.placeholders.c" -o "$OUT/$mode.placeholders.o"
        echo "     $mode: $(echo "$missing" | wc -l) placeholders ($(echo $missing | tr '\n' ' '))"
        clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "${exports[@]}" "${objs[@]}" \
            "$OUT/$mode.placeholders.o" -ldl -lcrypto -o "$OUT/$mode.$name"
    fi
    rc=0
    XENO_BATTLE_NATIVE_HOSTS=1 UBSAN_OPTIONS=symbolize=0 "$OUT/$mode.$name" "$@" > "$OUT/$mode.log" 2>&1 || rc=$?
    tail -n 3 "$OUT/$mode.log"
    if [ "$rc" -ne 0 ] || ! grep -Eq "$PASS" "$OUT/$mode.log"; then
        echo "PACKET HOST TEST FAIL $name $mode rc=$rc" >&2; status=1
    else
        echo "PACKET HOST TEST PASS $name $mode"
    fi
done
exit $status
