#!/usr/bin/env bash
# Differential prover for the adopted battle overlay host bodies.
#
# Runs every function in pc_port/src/battle_overlay_host_leaves.inc twice on the
# same guest RAM -- once as the retail MIPS bytes out of disc/battle.bin, once
# as the linked host C body -- and compares the result and the RAM that changed.
# See pc_port/tests/battle_overlay_host_differential_test.c for the contract and
# for what the comparison deliberately does not cover.
#
# Requires the port to have been built once (pc_port/build_native/stubs.c): the
# host bodies' callees and data come from the generated stub manifest, so this
# links the same definitions the shipped binary does.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

STUBS=pc_port/build_native/stubs.c
if [ ! -f "$STUBS" ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing $STUBS" >&2
    echo "  Build the port first: ./pc_port/build_port.sh" >&2
    exit 1
fi

OUT=${BATTLE_OVERLAY_DIFFERENTIAL_OUT:-$(mktemp -d /tmp/xeno-overlay-diff.XXXXXXXX)}
mkdir -p "$OUT"
printf 'BATTLE OVERLAY DIFFERENTIAL artifacts=%s\n' "$OUT"

# The allowlist is the single source of truth for both the names and the TUs.
LEAVES=$(python3 tools/scripts/battle_overlay_host_tus.py --leaves)
TUS=$(python3 tools/scripts/battle_overlay_host_tus.py --verify)
python3 tools/scripts/battle_overlay_host_tus.py --leaf-types \
    > "$OUT/overlay_leaf_types.inc"
# Pointer-typed overlay globals: the sweep seeds the guest word with a live RAM
# address for these, or a body that dereferences one faults on the retail side
# and no case can ever prove it.
python3 tools/scripts/battle_overlay_host_tus.py --pointer-globals \
    > "$OUT/overlay_pointer_globals.inc"

# A stale manifest that still stubs an adopted leaf would link two definitions
# of it; say so instead of failing with a wall of duplicate-symbol errors.
for leaf in $LEAVES; do
    if grep -q "\"$leaf\"" "$STUBS"; then
        echo "BATTLE OVERLAY DIFFERENTIAL FAIL $leaf is a generated stub in $STUBS" >&2
        echo "  The manifest is stale; re-run ./pc_port/build_port.sh" >&2
        exit 1
    fi
done

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then
    bridge_elf+=(--elf build/out/slus_006.64.elf)
fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt \
    --out "$OUT/battle_bridge_map.inc"

COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
# Must match pc_port/build_port.sh's GFLAGS: the body under test has to be the
# body the port ships.
HOST_FLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
    -DXENO_BATTLE_OVERLAY_HOST_BODIES -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -w -O0 -g -m64 -fno-builtin)
HOST_INC=(-Ipc_port/include_shim -Iinclude -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

gcc -c "$STUBS" -O0 -g -o "$OUT/stubs.o"

# Main-executable callees the adopted bodies reach, linked from the objects the
# port binary itself links, so both sides run the shipped code instead of a
# placeholder: HeapFree (memory.c), func_8001BD40 (temp3.c), the sprite-list
# builder func_8002675C (temp1.c), GetStringEntry/SystemRenderStringEntry
# (system.c), rsin/rcos (PsyCross LIBGTE.C, the
# definitions the runtime's dlsym binds 0x8003F8B0 / 0x8003F8CC to) and the
# libgpu primitive helpers SetPolyFT4/GetTPage/GetClut/SetSemiTrans/...
# (PsyCross LIBGPU.C). Their own unresolved references still get counted
# placeholders below; LIBGPU's display/VRAM entry points (GR_*, PsyX_EndScene)
# therefore mark any case that reaches them inconclusive. rand() is the C library's on both sides here, reseeded
# with srand(1) before each run, so the two sides draw identical sequences.
PORT_REAL_OBJS=()
for obj in pc_port/build_native/obj/src_slus_006.64_system_memory.c.o \
           pc_port/build_native/obj/src_slus_006.64_system_system.c.o \
           pc_port/build_native/obj/src_slus_006.64_system_temp1.c.o \
           pc_port/build_native/obj/src_slus_006.64_system_temp3.c.o; do
    if [ ! -f "$obj" ]; then
        echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing $obj (build the port first)" >&2
        exit 1
    fi
    PORT_REAL_OBJS+=("$obj")
done
PSYLIB=$(find pc_port/build -name libpsycross.a 2>/dev/null | head -1)
if [ -z "$PSYLIB" ]; then
    echo "BATTLE OVERLAY DIFFERENTIAL FAIL missing libpsycross.a (build the port first)" >&2
    exit 1
fi
mkdir -p "$OUT/psycross"
PSYLIB_ABS="$PWD/$PSYLIB"
(cd "$OUT/psycross" && ar x "$PSYLIB_ABS" LIBGTE.C.o LIBGPU.C.o)
PORT_REAL_OBJS+=("$OUT/psycross/LIBGTE.C.o" "$OUT/psycross/LIBGPU.C.o")

# main12.c is the mutation target for the negative control: it owns
# func_80079934, whose whole body is `*pValue += 4`.
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
src = Path("src/battle/main12.c").read_text()
old = "*pValue += 4;"
assert src.count(old) == 1, src.count(old)
(out / "mutant_main12.c").write_text(src.replace(old, "*pValue += 5;", 1))
PY

mode_flags() {
    case "$1" in
        O0)    echo "-O0" ;;
        O2)    echo "-O2" ;;
        UBSan) echo "-O1 -fsanitize=undefined -fno-sanitize-recover=all -fno-sanitize=function" ;;
    esac
}

# build_link <mode> <host-object-dir> <main12 source> <output suffix>
build_link() {
    local mode="$1" hostdir="$2" main12="$3" suffix="${4:-}"
    local flags; flags=$(mode_flags "$mode")
    local target="$OUT/$mode$suffix"
    local err="$OUT/$mode$suffix-bridge.err"
    local stub_c="$OUT/$mode$suffix-bridge.c"

    rm -rf "$hostdir"; mkdir -p "$hostdir"
    for tu in $TUS; do
        local src="$tu"
        [ "$tu" = "src/battle/main12.c" ] && src="$main12"
        gcc -c "$src" "${HOST_FLAGS[@]}" "${HOST_INC[@]}" \
            -o "$hostdir/$(basename "$tu" .c).o"
    done
    # shellcheck disable=SC2086
    clang "${COMMON[@]}" $flags -Wall -Wextra -Werror \
        -DBATTLE_RUNTIME_SOURCE="\"$PWD/pc_port/src/battle_mips_runtime.c\"" \
        -c pc_port/tests/battle_overlay_host_differential_test.c \
        -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" $flags -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"

    # Some callees of an adopted body are resolved by the port from PsyCross or
    # from game translation units this harness does not link. Rather than let the
    # link die, give each one a placeholder that fails loudly: a leaf that
    # reaches one is not provable here, and the run must say so instead of
    # reporting a match. The set comes from the linker itself, so nothing the C
    # library already provides gets a colliding second definition. The probe
    # link reports mangled names so C++ callees (PsyCross) get placeholders too.
    local extra=()
    if ! clang -no-pie -rdynamic $flags -Wl,--gc-sections -Wl,--no-demangle \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$hostdir"/*.o "${PORT_REAL_OBJS[@]}" "$OUT/stubs.o" \
        -ldl -o "$target" 2>"$err"; then
        local missing
        missing=$(grep -oE "undefined reference to \`[A-Za-z0-9_]+'" "$err" \
            | sed "s/.*\`//; s/'//" | sort -u)
        if [ -z "$missing" ]; then
            cat "$err" >&2
            echo "BATTLE OVERLAY DIFFERENTIAL FAIL $mode link failed (no undefined refs)" >&2
            exit 1
        fi
        {
            echo '/* Generated by run_battle_overlay_host_differential_test.sh.'
            echo ' * Placeholders for callees only PsyCross or a non-linked game TU'
            echo ' * provides. The interpreted side resolves the same name through the'
            echo ' * bridge map and dlsym, so it reaches these placeholders too and both'
            echo ' * sides would compare an undefined return register. Every call is'
            echo ' * therefore counted in g_prover_placeholder_hits, and the prover'
            echo ' * treats any case that reached one as inconclusive, never as a match;'
            echo ' * a leaf with no proven case is reported UNPROVEN and fails the run. */'
            echo '#include <stdio.h>'
            echo 'extern unsigned g_prover_placeholder_hits;'
            echo 'static void unresolved(const char *name) {'
            echo '    static const char *seen[256];'
            echo '    static int count;'
            echo '    int i;'
            echo '    g_prover_placeholder_hits++;'
            echo '    for (i = 0; i < count; i++) if (seen[i] == name) return;'
            echo '    if (count < 256) seen[count++] = name;'
            echo '    fprintf(stderr, "[prover] unavailable callee %s (case inconclusive)\n", name);'
            echo '}'
            for sym in $missing; do
                case "$sym" in
                    D_*|g_*|jtbl_*|jpt_*|str_*|aD_*|aG_*|gteRegs|activeDispEnv|activeDrawEnv|currentDispEnv)
                        # 64 KiB, not 4 KiB: the runtime binds a shared
                        # global by its retail size (g_GameState is widened
                        # to 0x2358 in initialize_runtime), so a smaller
                        # placeholder let an interpreted read of
                        # g_GameState+0x22B6 run off the end of BSS.
                        echo "unsigned char $sym[0x10000];" ;;
                    *)
                        echo "void $sym(void) { unresolved(\"$sym\"); }" ;;
                esac
            done
        } > "$stub_c"
        clang -c "$stub_c" -O0 -o "$OUT/$mode$suffix-bridge.o"
        extra=("$OUT/$mode$suffix-bridge.o")
        printf 'BATTLE OVERLAY DIFFERENTIAL bridge-placeholders=%s\n' \
            "$(echo "$missing" | wc -l)"
    fi
    # shellcheck disable=SC2086
    clang -no-pie -rdynamic $flags -Wl,--gc-sections \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$hostdir"/*.o "${PORT_REAL_OBJS[@]}" "$OUT/stubs.o" \
        "${extra[@]}" -ldl -o "$target"
}

# build_link <mode> <host-object-dir> <main12 source>
for mode in ${BATTLE_OVERLAY_DIFFERENTIAL_MODES:-O0 O2 UBSan}; do
    build_link "$mode" "$OUT/host-$mode" "src/battle/main12.c" ""
    status=0
    "$OUT/$mode" >"$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    if [ "$status" -ne 0 ] || ! grep -q '^BATTLE OVERLAY DIFFERENTIAL PASS ' "$OUT/$mode.log"; then
        echo "BATTLE OVERLAY DIFFERENTIAL FAIL mode=$mode status=$status" >&2
        exit 1
    fi
done

# Negative control: a body that returns the wrong value must be rejected.
build_link O2 "$OUT/host-mutant" "$OUT/mutant_main12.c" "-mutant"
status=0
"$OUT/O2-mutant" >"$OUT/mutant.log" 2>&1 || status=$?
if [ "$status" -eq 0 ] || ! grep -q '^BATTLE OVERLAY DIFFERENTIAL FAIL ' "$OUT/mutant.log"; then
    cat "$OUT/mutant.log" >&2
    echo "BATTLE OVERLAY DIFFERENTIAL CONTROL FAILURE wrong-result body accepted" >&2
    exit 1
fi
grep -m1 '^BATTLE OVERLAY DIFFERENTIAL FAIL ' "$OUT/mutant.log" >&2

echo "BATTLE OVERLAY DIFFERENTIAL PASS (O0/O2/UBSan + wrong-result control rejected)"
