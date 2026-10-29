#!/usr/bin/env bash
# World-map retail differential: the retail MIPS out of the pinned
# disc/world_map.bin (overlay @0x8006FAF0) on the port's MIPS interpreter
# (TEST ORACLE ONLY) vs the linked port C bodies, on identical guest RAM.
# Compared: every guest RAM byte (minus the retail stack window), the boundary
# call order + declared-width arguments, and the return value.
# See pc_port/tests/world_map_differential_test.c.
#
# Regimes O0 / O2 / UBSan (-fsanitize=undefined -fno-sanitize-recover=all,
# shift-base ON; a UB report is a FAIL). Then one deliberately-wrong mutant per
# gate body (tools/scripts/wm_mutant.py: one integer literal +1) that must FAIL;
# the first rejected candidate is printed, a body whose candidates all survive
# fails the gate.
#   WMD_ONLY=wm_80097CB8,wm_... restricts the cases; WMD_CASES=N per function;
#   WMD_ALL=1 diagnostic sweep of every exported body (no gate, no mutants);
#   WMD_REGIMES="O0 O2 UBSan"; WMD_SKIP_CONTROL=1 skips the mutants.
set -euo pipefail
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=${WORLD_MAP_DIFFERENTIAL_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-wmd.XXXXXXXX")}
mkdir -p "$OUT"
IMAGE=${WMD_IMAGE:-disc/world_map.bin}
export WMD_IMAGE="$IMAGE"
sha=$(sha256sum "$IMAGE" | awk '{print $1}')
[ "$sha" = 4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70 ] \
  || { echo "ERROR: $IMAGE SHA-256 mismatch: $sha" >&2; exit 1; }
echo "WORLD MAP ORACLE $IMAGE sha256=$sha"
REGIMES=${WMD_REGIMES:-O0 O2 UBSan}
GATE=0
if [ -z "${WMD_ONLY:-}" ] && [ -z "${WMD_ALL:-}" ]; then
    WMD_ONLY=$(grep -v '^#' pc_port/tests/world_map_differential_gate.txt | awk 'NF{print $1}' | paste -sd, -)
    export WMD_ONLY WMD_REQUIRE_PROVEN=1
    GATE=1
fi
[ -n "${WMD_REQUIRE_PROVEN:-}" ] && GATE=1
[ -n "${WMD_ALL:-}" ] && GATE=0
P=pc_port/extern/PsyCross
# include/common.h's offsetof idiom trips UBSan; scoped wrapper (the only
# allowed workaround) swaps in the builtin, first on the include path.
mkdir -p "$OUT/inc"
printf '#include_next "common.h"\n#undef offsetof\n#define offsetof(s, m) __builtin_offsetof(s, m)\n' > "$OUT/inc/common.h"
rc=0
for regime in $REGIMES; do
    d="$OUT/$regime"
    mkdir -p "$d"
    opt="-$regime" hflag="" lflag="" extra=""
    if [ "$regime" = UBSan ]; then
        opt="-O1 -fsanitize=undefined -fno-sanitize-recover=all"
        hflag=-DWMD_UBSAN lflag=-fsanitize=undefined extra="-I$OUT/inc"
    fi
    python3 tools/scripts/wm_diff_gen.py --out "$d" --opt="$opt" ${extra:+--extra="$extra"}
    gcc -std=gnu17 -O0 -g -w $hflag -DXENO_PC_PORT -Ipc_port/src -I"$d" \
        -c pc_port/tests/world_map_differential_test.c -o "$d/harness.o"
    gcc -std=gnu17 -O0 -g -w -Ipc_port/src -c pc_port/src/battle_mips_adapter.c -o "$d/adapter.o"
    gcc -std=gnu17 -O0 -g -w -c "$d/wmd_stubs.c" -o "$d/stubs.o"
    g++ -c -O0 -w -I$P/include -I$P/include/psx -I$P/src $P/src/gte/PsyX_GTE.cpp -o "$d/gte.o"
    g++ -x c++ -c -O0 -w -I$P/include -I$P/include/psx -I$P/src $P/src/psx/INLINE_C.C -o "$d/inline.o"
    g++ -c -O0 -w -I$P/include -I$P/include/psx -I$P/src $P/src/gte/half_float.cpp -o "$d/half.o"
    gcc -std=gnu17 -c -O0 -w -I$P/include -I$P/include/psx pc_port/tests/world_map_differential_gte.c -o "$d/gtesize.o"
    g++ -no-pie $lflag -o "$d/wmd" "$d/harness.o" "$d/adapter.o" "$d/stubs.o" "$d/gte.o" "$d/half.o" "$d/inline.o" \
        "$d/gtesize.o" $(cat "$d/objs.txt") -lm
    echo "== regime $regime"
    set +e
    UBSAN_OPTIONS="abort_on_error=1:print_stacktrace=1:log_path=$d/ubsan" "$d/wmd" | tee "$d/result.txt"
    r=${PIPESTATUS[0]}
    set -e
    [ "$r" -eq 0 ] || rc=1
done
# Negative controls (gate mode): one wrong body per gate entry must go red.
if [ "$GATE" = 1 ] && [ -z "${WMD_SKIP_CONTROL:-}" ]; then
    base="$OUT/${REGIMES%% *}"
    m="$OUT/mutants"
    mkdir -p "$m"
    killed=0 survived=0
    for name in ${WMD_ONLY//,/ }; do
        python3 tools/scripts/wm_mutant.py "$name" "$m" --max "${WMD_MUTANT_MAX:-10}" >/dev/null
        hit=""
        while read -r k tu line old _ new; do
            obj=$base/obj/$(basename "$tu" .c).o
            python3 tools/scripts/wm_diff_gen.py --out "$m" --opt=-O0 --compile-one "$m/$name.$k.c" "$m/$name.$k.o" \
                2>/dev/null || continue
            g++ -no-pie -o "$m/wmd.$name" "$base/harness.o" "$base/adapter.o" "$base/stubs.o" "$base/gte.o" \
                "$base/half.o" "$base/inline.o" "$base/gtesize.o" \
                $(sed "s#^$obj\$#$m/$name.$k.o#" "$base/objs.txt") -lm 2>/dev/null || continue
            set +e
            WMD_ONLY=$name "$m/wmd.$name" > "$m/$name.$k.log" 2>&1
            set -e
            if grep -q "^FAIL $name " "$m/$name.$k.log"; then
                hit="$k $tu:$line $old->$new"
                break
            fi
        done < "$m/$name.list"
        rm -f "$m/wmd.$name" "$m"/"$name".*.o
        if [ -n "$hit" ]; then
            echo "MUTANT REJECTED $name (candidate $hit)"
            killed=$((killed + 1))
        else
            echo "MUTANT SURVIVED $name ($(wc -l < "$m/$name.list") candidates) -- harness blind to this body" >&2
            survived=$((survived + 1)); rc=1
        fi
    done
    echo "WORLD MAP MUTANTS rejected=$killed survived=$survived"
fi
ctl=""; [ "$GATE" = 1 ] && [ -z "${WMD_SKIP_CONTROL:-}" ] && ctl="; every per-body mutant rejected"
[ $rc -eq 0 ] && echo "WORLD MAP DIFFERENTIAL PASS ($REGIMES$ctl)"
exit $rc
