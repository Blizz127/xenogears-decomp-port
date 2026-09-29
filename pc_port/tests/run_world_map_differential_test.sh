#!/usr/bin/env bash
# World-map differential prover: retail MIPS out of disc/world_map.bin vs the
# linked port C bodies. See pc_port/tests/world_map_differential_test.c.
#   WMD_ONLY=wm_80097CB8,wm_... restricts the cases; WMD_CASES=N per function.
set -euo pipefail
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=${WORLD_MAP_DIFFERENTIAL_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-wmd.XXXXXXXX")}
mkdir -p "$OUT"
IMAGE=${WMD_IMAGE:-disc/world_map.bin}
export WMD_IMAGE="$IMAGE"
REGIMES=${WMD_REGIMES:-O0 O2}
# Default: the gate list (bodies proven against retail; each must stay PROVEN).
# WMD_ALL=1 sweeps every exported world-map body (diagnostic; many known FAILs).
if [ -z "${WMD_ONLY:-}" ] && [ -z "${WMD_ALL:-}" ]; then
    WMD_ONLY=$(grep -v '^#' pc_port/tests/world_map_differential_gate.txt | awk 'NF{print $1}' | paste -sd, -)
    export WMD_ONLY WMD_REQUIRE_PROVEN=1
fi
rc=0
for regime in $REGIMES; do
    d="$OUT/$regime"
    mkdir -p "$d"
    python3 tools/scripts/wm_diff_gen.py --out "$d" --opt="-${regime}"
    gcc -std=gnu17 -"$regime" -g -w -DXENO_PC_PORT -Ipc_port/src -I"$d" \
        -c pc_port/tests/world_map_differential_test.c -o "$d/harness.o"
    gcc -std=gnu17 -O0 -g -w -Ipc_port/src -c pc_port/src/battle_mips_adapter.c -o "$d/adapter.o"
    gcc -std=gnu17 -O0 -g -w -c "$d/wmd_stubs.c" -o "$d/stubs.o"
    P=pc_port/extern/PsyCross
    g++ -c -O0 -w -I$P/include -I$P/include/psx -I$P/src $P/src/gte/PsyX_GTE.cpp -o "$d/gte.o"
    g++ -x c++ -c -O0 -w -I$P/include -I$P/include/psx -I$P/src $P/src/psx/INLINE_C.C -o "$d/inline.o"
    g++ -c -O0 -w -I$P/include -I$P/include/psx -I$P/src $P/src/gte/half_float.cpp -o "$d/half.o"
    gcc -std=gnu17 -c -O0 -w -I$P/include -I$P/include/psx pc_port/tests/world_map_differential_gte.c -o "$d/gtesize.o"
    g++ -no-pie -o "$d/wmd" "$d/harness.o" "$d/adapter.o" "$d/stubs.o" "$d/gte.o" "$d/half.o" "$d/inline.o" "$d/gtesize.o" \
        $(cat "$d/objs.txt") -lm
    echo "== regime $regime"
    set +e
    "$d/wmd" | tee "$d/result.txt"
    r=${PIPESTATUS[0]}
    set -e
    [ "$r" -eq 0 ] || rc=1
done
# Negative control: the same harness must reject a known-wrong body.
# WM_97DC0_MUTANT_M3 indexes the 9x9 window as row+col instead of row*9+col.
if [ -z "${WMD_SKIP_CONTROL:-}" ]; then
    d="$OUT/control"
    mkdir -p "$d"
    python3 tools/scripts/wm_diff_gen.py --out "$d" --opt=-O0 --extra=-DWM_97DC0_MUTANT_M3 >/dev/null
    gcc -std=gnu17 -O0 -g -w -DXENO_PC_PORT -Ipc_port/src -I"$d" \
        -c pc_port/tests/world_map_differential_test.c -o "$d/harness.o"
    gcc -std=gnu17 -O0 -g -w -c "$d/wmd_stubs.c" -o "$d/stubs.o"
    g++ -no-pie -o "$d/wmd" "$d/harness.o" "$OUT/${REGIMES%% *}/adapter.o" "$d/stubs.o" \
        "$OUT/${REGIMES%% *}/gte.o" "$OUT/${REGIMES%% *}/half.o" "$OUT/${REGIMES%% *}/inline.o" \
        "$OUT/${REGIMES%% *}/gtesize.o" $(cat "$d/objs.txt") -lm
    set +e
    WMD_ONLY=wm_80097DC0 "$d/wmd" > "$d/result.txt"
    set -e
    if grep -q '^FAIL wm_80097DC0 ' "$d/result.txt"; then
        echo "negative control: WM_97DC0_MUTANT_M3 REJECTED"
    else
        echo "negative control NOT rejected -- harness is blind" >&2
        cat "$d/result.txt" >&2
        rc=1
    fi
fi
[ $rc -eq 0 ] && echo "WORLD MAP DIFFERENTIAL PASS ($REGIMES; wrong-body control rejected)"
exit $rc
