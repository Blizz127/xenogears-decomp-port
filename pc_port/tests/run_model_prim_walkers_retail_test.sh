#!/usr/bin/env bash
#
# Table-driven retail differential for the D_8004FE50 model-prim walkers.
#
# Links the REAL production table owner pc_port/src/game_overrides.c plus the
# other body TUs it dispatches (temp2.c's func_8002E688/F2E0/F4B4/FAE8 and
# model_prim_ed20.c's func_8002ED20) and drives every distinct retail walker
# entry through the live port table, comparing the native body against the
# retail SLUS bytes executed on the project MIPS interpreter:
# packet window + guards, full OT, walker globals/cursors, emitted counter,
# input windows and the complete GTE register file, per fixture case.
#
# Then proves the test detects damage: one mutation per walker body (a store
# offset, an OT shift, a tag length, a gate -- see the table below), each
# rebuilt into its own TU and relinked, MUST make the binary fail.
#
# Retail bytes never enter the tree: the binary reads disc/SLUS_006.64 at
# runtime; this script only pins sha256 slices so moved/altered retail code
# is caught before a misleading comparison.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/model_prim_walkers_retail_test
mkdir -p "$OUT"
check_slice() {
    local address="$1" size="$2" expected="$3" actual
    read -r actual _ < <(dd if=disc/SLUS_006.64 bs=1 \
        skip=$((address - 0x8000f800)) count=$((size)) status=none | sha256sum)
    if [ "$actual" != "$expected" ]; then
        echo "MODEL PRIM WALKERS RETAIL SLICE MISMATCH: $address" >&2
        exit 1
    fi
}
if [ ! -f disc/SLUS_006.64 ]; then
    echo 'MODEL PRIM WALKERS SKIP: disc/SLUS_006.64 not present' >&2
    exit 0
fi
# The whole walker code span (alternate entries E010/E024/E038/E04C through
# func_8003101C's tail) and the retail dispatch table itself.
check_slice 0x8002e010 0x3160 e89822c1f429f192dbf4b9019ce3d4a5e4b6092d6bf229edc94e55e6dc33e010
check_slice 0x8004fe50 0x2e0 7ecbb9ab152ccc320a8fe321e56da89328d1689e87b96bdd5161d95f002fb79c

COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0
    -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native)
STRICT=(-Wall -Wextra -Werror
    -include common.h -include psyq/libgte.h -include psx/inline_c.h
    -include psx_memory.h -include pc_port/src/model_prim_link.h)

FAILED_REGIMES=0
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${COMMON[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" \
            -o "$OUT/$opt.$name.o"
    done
    # Production body TUs compiled with their own includes and flags (the
    # port build's per-TU recipe; see run_prim_table_retail_test.sh and
    # run_model_prim_f2e0_retail_test.sh).
    gcc "${COMMON[@]}" "${GAME[@]}" -DXENO_FIELD_OBJECT_OVERLAY "${flags[@]}" \
        -fpermissive -w -c pc_port/src/game_overrides.c -o "$OUT/$opt.overrides.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -fpermissive -w \
        -c src/slus_006.64/system/temp2.c -o "$OUT/$opt.temp2.o"
    # The port owns several temp2.c symbols outright (port_owned_overrides.txt;
    # game_overrides.c carries the strong body).  The port build weakens the
    # duplicate game-TU definition with objcopy --weaken-symbol -- replicate
    # that here so linking both TUs matches production ownership.
    for sym in $(cut -d'|' -f1 pc_port/port_owned_overrides.txt | grep -v '^#'); do
        # Capture nm's output first: piping straight into `grep -q` lets grep
        # exit early, nm dies of SIGPIPE, and with pipefail the match reads
        # as a failure and the weaken is silently skipped.
        syms=$(nm "$OUT/$opt.temp2.o")
        if grep -q " T $sym\$" <<<"$syms"; then
            objcopy --weaken-symbol="$sym" "$OUT/$opt.temp2.o"
        fi
    done
    # Test driver and support TUs: strict.
    for pair in "test:pc_port/tests/model_prim_walkers_retail_test.c" \
                "cpu:pc_port/src/battle_mips_adapter.c" \
                "ed20:pc_port/src/model_prim_ed20.c" \
                "model_link:pc_port/src/model_prim_link.c" \
                "guest_link:pc_port/src/guest_prim_link.c" \
                "leaf:pc_port/src/retail_leaf_adapters.c" \
                "normlight:pc_port/src/psyq_compat_normal_light_col.c" \
                "testinput:pc_port/src/test_input.c"; do
        name=${pair%%:*}; source=${pair#*:}
        if [ "$name" = cpu ]; then
            # The interpreter is host code with system headers (<signal.h>,
            # <unistd.h>); the PsyQ sys/types.h pre-include redefines
            # uid_t/gid_t, so it is compiled without the game pre-includes.
            gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
                -c "$source" -o "$OUT/$opt.$name.o"
            continue
        fi
        gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" "${STRICT[@]}" \
            -c "$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/psyq_compat.c -o "$OUT/$opt.compat.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -ldl \
        -o "$OUT/$opt.test"
    echo "MODEL PRIM WALKERS full production table $opt"
    # Run every regime to completion even while bodies diverge, so the
    # per-walker report and the mutant controls below stay observable;
    # the final verdict still fails the script.
    rc=0
    timeout 60s "$OUT/$opt.test" || rc=$?
    if [ "$rc" -ne 0 ]; then
        echo "MODEL PRIM WALKERS REGIME DIVERGES $opt (rc=$rc)"
        FAILED_REGIMES=$((FAILED_REGIMES + 1))
    fi
done

# ---------------------------------------------------------------------------
# Negative controls: one mutation per walker body, each MUST fail the run.
# The mutation is applied INSIDE the body's span only (python), so a regex
# that also matches elsewhere in the TU cannot corrupt another body.
# ---------------------------------------------------------------------------
MUT_GAME_OVERRIDES=(
    "ModelPrimTriSharedSpine:out \+ 0x08\) = xy0:out + 0x0A) = xy0"
    "ModelPrimTriMaxSZSharedSpine:D_80050100 \+ 2:D_80050100 + 3"
    "ModelPrimQuadSharedSpine:out \+ 0x08\) = xy0:out + 0x0A) = xy0"
    "ModelPrimQuadSZSelectSpine:D_80050100 \+ 2:D_80050100 + 3"
    "ModelPrimQuadF4LitVariant1:out \+ 0x14\) = xy3:out + 0x16) = xy3"
    "ModelPrimTriMediumLitVariant1:out \+ 0x0C\) = MFC2\(21\):out + 0x0E) = MFC2(21)"
    "ModelPrimTriDepthCueVariant4:out\[0x7\] << 24:out[0x6] << 24"
    "ModelPrimTriDepthCueMaxSZVariant5:out\[0x7\] << 24:out[0x6] << 24"
    "ModelPrimQuadFT4DepthCueVariant4:out \+ 0x10\) = xy1:out + 0x12) = xy1"
    "ModelPrimQuadFT4DepthCueMaxSZVariant5:D_80050100 \+ 2\) & 31u:D_80050100 + 3) & 31u"
)
mutate_span() {
    # $1 source TU, $2 function name (start-of-line "static s32 NAME(" or
    # "s32 NAME("), $3 find regex, $4 replacement, $5 output path.
    python3 - "$1" "$2" "$3" "$4" "$5" <<'PY'
import re, sys
src, fn, find, repl, dst = sys.argv[1:6]
src_text = open(src).read()
# [^;{]* lets the signature span lines but never skip past a ';' (the
# forward declaration) to find a brace elsewhere.
m = re.search(r'^(static )?s32 %s\([^;{]*\)\s*\{' % re.escape(fn), src_text, re.M)
if not m:
    sys.exit("no body start for " + fn)
start = m.start()
end = src_text.index('\n}\n', start) + 2  # body end at column-0 '}'
span = src_text[start:end]
n = len(re.findall(find, span))
if n != 1:
    sys.exit("mutation target hit %d times (want 1) in %s" % (n, fn))
open(dst, "w").write(src_text[:start] + re.sub(find, repl, span, count=1) + src_text[end:])
PY
}

# Object lists for mutant links: the mutated TU REPLACES its production
# object, so never glob the whole $OUT (that would link the original body
# alongside the mutant and fail on duplicate definitions).
LINK_WITHOUT_OVERRIDES=(test cpu temp2 ed20 model_link guest_link leaf
    normlight testinput compat LIBGTE.C INLINE_C.C PsyX_GTE.cpp)
mut_objects() {
    # $1... object base names to link (O2.<name>.o)
    local objs=()
    for name in "$@"; do objs+=("$OUT/O2.$name.o"); done
    echo "${objs[@]}"
}

MUTANTS_REJECTED=0
for entry in "${MUT_GAME_OVERRIDES[@]}"; do
    fn=${entry%%:*}; rest=${entry#*:}; find=${rest%%:*}; repl=${rest#*:}
    mutate_span pc_port/src/game_overrides.c "$fn" "$find" "$repl" "$OUT/$fn.c"
    gcc "${COMMON[@]}" "${GAME[@]}" -DXENO_FIELD_OBJECT_OVERLAY -O2 -fpermissive -w \
        -c "$OUT/$fn.c" -o "$OUT/$fn.o"
    # shellcheck disable=SC2046
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$fn.o" \
        $(mut_objects "${LINK_WITHOUT_OVERRIDES[@]}") -ldl -o "$OUT/$fn.test"
    if timeout 60s "$OUT/$fn.test" > "$OUT/$fn.log" 2>&1; then
        echo "MODEL PRIM WALKERS MUTANT SURVIVED: $fn" >&2
        exit 1
    fi
    if ! grep -q 'MODEL PRIM WALKERS FAIL' "$OUT/$fn.log"; then
        echo "MODEL PRIM WALKERS MUTANT BROKEN (no FAIL line): $fn" >&2
        exit 1
    fi
    echo "  mutant rejected: $fn"
    MUTANTS_REJECTED=$((MUTANTS_REJECTED + 1))
done

# temp2.c bodies: same span mutation, temp2 object replaced, port-owned
# override weakening replicated as for the production temp2 object.
MUT_TEMP2=(
    "func_8002E688:out \+ 0x10\) = \(u32\)xy1:out + 0x12) = (u32)xy1"
    "func_80030750:out\[0x0D\] = :out[0x0E] = "
)
for entry in "${MUT_TEMP2[@]}"; do
    fn=${entry%%:*}; rest=${entry#*:}; find=${rest%%:*}; repl=${rest#*:}
    mutate_span src/slus_006.64/system/temp2.c "$fn" "$find" "$repl" "$OUT/$fn.c"
    gcc "${COMMON[@]}" "${GAME[@]}" -O2 -fpermissive -w \
        -c "$OUT/$fn.c" -o "$OUT/$fn.o"
    for sym in $(cut -d'|' -f1 pc_port/port_owned_overrides.txt | grep -v '^#'); do
        syms=$(nm "$OUT/$fn.o")
        if grep -q " T $sym\$" <<<"$syms"; then
            objcopy --weaken-symbol="$sym" "$OUT/$fn.o"
        fi
    done
    # (object list on one line: this bash build mis-parses multi-line $( ))
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$fn.o" \
        $(mut_objects overrides test cpu ed20 model_link guest_link leaf normlight testinput compat LIBGTE.C INLINE_C.C PsyX_GTE.cpp) -ldl -o "$OUT/$fn.test"
    if timeout 60s "$OUT/$fn.test" > "$OUT/$fn.log" 2>&1; then
        echo "MODEL PRIM WALKERS MUTANT SURVIVED: $fn" >&2
        exit 1
    fi
    if ! grep -q 'MODEL PRIM WALKERS FAIL' "$OUT/$fn.log"; then
        echo "MODEL PRIM WALKERS MUTANT BROKEN (no FAIL line): $fn" >&2
        exit 1
    fi
    echo "  mutant rejected: $fn"
    MUTANTS_REJECTED=$((MUTANTS_REJECTED + 1))
done

mutate_span pc_port/src/model_prim_ed20.c func_8002ED20 \
    'otz >> shift' 'otz >> (shift + 1)' "$OUT/func_8002ED20.c"
gcc "${COMMON[@]}" "${GAME[@]}" -O2 "${STRICT[@]}" \
    -c "$OUT/func_8002ED20.c" -o "$OUT/func_8002ED20.o"
clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/func_8002ED20.o" \
    $(mut_objects overrides test cpu temp2 model_link guest_link leaf normlight testinput compat LIBGTE.C INLINE_C.C PsyX_GTE.cpp) -ldl -o "$OUT/func_8002ED20.test"
if timeout 60s "$OUT/func_8002ED20.test" > "$OUT/func_8002ED20.log" 2>&1; then
    echo "MODEL PRIM WALKERS MUTANT SURVIVED: func_8002ED20" >&2
    exit 1
fi
if ! grep -q 'MODEL PRIM WALKERS FAIL' "$OUT/func_8002ED20.log"; then
    echo "MODEL PRIM WALKERS MUTANT BROKEN (no FAIL line): func_8002ED20" >&2
    exit 1
fi
echo "  mutant rejected: func_8002ED20"
MUTANTS_REJECTED=$((MUTANTS_REJECTED + 1))

echo "MODEL PRIM WALKERS MUTANTS PASS $MUTANTS_REJECTED bodies (store-offset/ot-shift/tag/gate)"

# Caveat while bodies are uncertified: a mutant of a DIVERGING body fails the
# run for the pre-existing divergence, not (necessarily) for its own damage,
# so the controls above only prove per-body sensitivity once every body
# passes the regimes.  FAILED_REGIMES > 0 keeps the gate closed until then.
if [ "$FAILED_REGIMES" -ne 0 ]; then
    echo "MODEL PRIM WALKERS RESULT FAIL regimes-diverging=$FAILED_REGIMES" >&2
    exit 1
fi
echo "MODEL PRIM WALKERS RESULT PASS"
