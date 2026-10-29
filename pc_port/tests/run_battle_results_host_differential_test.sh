#!/usr/bin/env bash
# Differential prover for the adopted battle RESULTS module routines.
#
# Extracts the retail results module (archive 0x10/file 4) from ./disc into the
# artifact directory, then runs every routine in
# pc_port/src/battle_results_host_leaves.inc twice from identical state -- the
# retail MIPS bytes through the interpreter (force_interpret) and the native
# body through the production identity gate and bridge -- and compares v0,
# guest RAM, the native g_GameState object and the scratchpad.  See
# pc_port/tests/battle_results_host_differential_test.c for the contract, the
# gate controls and what is not covered.
#
# Negative controls (each must be REJECTED by the prover):
#   wrong-body     func_801E24B0 loop bound 150 -> 149;
#   raw-mapping    the accessors replaced by raw PSX_ADDR-style guest RAM, the
#                  draft bug the shared resolver exists to prevent;
#   wrong-identity the runtime's pinned module digest altered, so the gate must
#                  refuse every native dispatch.
#
# Requires a prior ./pc_port/build_port.sh (stubs.c and the production
# data_game_state.c object, so the runtime binds the shipped g_GameState).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

STUBS=pc_port/build_native/stubs.c
GS_OBJ=pc_port/build_native/obj/data_game_state.c.o
for need in "$STUBS" "$GS_OBJ"; do
    if [ ! -f "$need" ]; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL missing $need" >&2
        echo "  Build the port first: ./pc_port/build_port.sh" >&2
        exit 1
    fi
done

OUT=${BATTLE_RESULTS_DIFFERENTIAL_OUT:-$(mktemp -d /tmp/xeno-results-diff.XXXXXXXX)}
# The mutant controls #include sources by path from other directories, so a
# relative artifacts directory must be made absolute.
OUT=$(realpath -m "$OUT")
mkdir -p "$OUT"
printf 'BATTLE RESULTS DIFFERENTIAL artifacts=%s\n' "$OUT"

# The allowlist is the single source of truth; refuse a malformed one.
LEAVES=$(python3 tools/scripts/battle_results_host_leaves.py --verify)
SYMBOLS=$(python3 tools/scripts/battle_results_host_leaves.py --symbols)
printf 'BATTLE RESULTS DIFFERENTIAL leaves=%s\n' "$(echo $LEAVES)"
for sym in $SYMBOLS; do
    if grep -q "\"$sym\"" "$STUBS"; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL $sym is a generated stub in $STUBS" >&2
        exit 1
    fi
done

# Retail bytes come from the disc at test time only (identity-checked by the
# extractor); nothing retail is in the tree.
python3 tools/scripts/extract_battle_results_module.py \
    --output "$OUT/results_module.bin" > "$OUT/results_module.json"
printf 'BATTLE RESULTS DIFFERENTIAL module=%s\n' "$OUT/results_module.bin"

# Size of the production g_GameState object: the prover compares all of it.
GS_BYTES=$(nm -S --defined-only "$GS_OBJ" | awk '$4 == "g_GameState" {print "0x" $2; exit}')
if [ -z "$GS_BYTES" ]; then
    echo "BATTLE RESULTS DIFFERENTIAL FAIL no g_GameState in $GS_OBJ" >&2
    exit 1
fi

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
# pc_port/build_port.sh's GFLAGS: the body under test is the body the port
# ships.  Its -O0 is overridden per mode below.
HOST_FLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
    -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
    -w -O0 -g -m64 -fno-builtin)

gcc -c "$STUBS" -O0 -g -o "$OUT/stubs.o"

# Mutant sources for the negative controls.
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])

def mutate(src, old, new, dest):
    text = Path(src).read_text()
    assert text.count(old) == 1, (src, old, text.count(old))
    dest = out / dest
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(text.replace(old, new, 1))

host_c = "pc_port/src/battle_results_host.c"
host_h = "pc_port/src/battle_results_host.h"
# wrong-body: drops the last table entry.
mutate(host_c, "for (i = 0; i < 150; i++)", "for (i = 0; i < 149; i++)",
       "mutant-wrong-body/battle_results_host.c")
(out / "mutant-wrong-body/battle_results_host.h").write_text(Path(host_h).read_text())
# raw-mapping: the accessors read/write raw guest RAM, never the native copies.
(out / "mutant-raw-mapping").mkdir(exist_ok=True)
(out / "mutant-raw-mapping/battle_results_host.c").write_text(Path(host_c).read_text())
mutate(host_h,
       "uint8_t *PcPort_BattleMipsResolve(uint32_t address, unsigned width, int write);",
       "extern uint8_t g_PsxRam[];\n"
       "static inline uint8_t *raw_guest(uint32_t address, unsigned width, int write)\n"
       "{ (void)width; (void)write; return g_PsxRam + (address & 0x1FFFFFu); }\n"
       "#define PcPort_BattleMipsResolve raw_guest",
       "mutant-raw-mapping/battle_results_host.h")
# wrong-identity: the runtime (and its includes, found next to it first)
# with one digit of the pinned digest changed.
gate = Path("pc_port/src/battle_results_gate.inc").read_text()
digest = "80c31ea1289a0ce11e2440ee9759494503347c5a27cbb325d33a4492e4d74f67"
assert gate.count(digest) == 1
(out / "mutant-wrong-identity").mkdir(exist_ok=True)
(out / "mutant-wrong-identity/battle_results_gate.inc").write_text(
    gate.replace(digest, "0" + digest[1:], 1))
(out / "mutant-wrong-identity/battle_mips_runtime.c").write_text(
    Path("pc_port/src/battle_mips_runtime.c").read_text())
PY

mode_flags() {
    case "$1" in
        O0)    echo "-O0" ;;
        O2)    echo "-O2" ;;
        UBSan) echo "-O1 -fsanitize=undefined -fno-sanitize-recover=all -fno-sanitize=function" ;;
    esac
}

# build_link <mode> <host source dir> <runtime source> <output suffix>
build_link() {
    local mode="$1" hostdir="$2" runtime="$3" suffix="${4:-}"
    local flags; flags=$(mode_flags "$mode")
    local target="$OUT/$mode$suffix"
    local err="$OUT/$mode$suffix-link.err"
    local stub_c="$OUT/$mode$suffix-placeholders.c"
    local objs=("$OUT/$mode$suffix.test.o" "$OUT/$mode$suffix.cpu.o"
                "$OUT/$mode$suffix.host.o" "$GS_OBJ" "$OUT/stubs.o")

    if [ "$mode" = UBSan ]; then
        # gcc cannot link libubsan on these hosts; clang, warnings fatal.
        # shellcheck disable=SC2086
        clang "${COMMON[@]}" $flags -Wall -Wextra -Werror \
            -c "$hostdir/battle_results_host.c" -o "$OUT/$mode$suffix.host.o"
    else
        # shellcheck disable=SC2086
        gcc -c "$hostdir/battle_results_host.c" "${HOST_FLAGS[@]}" $flags \
            -Ipc_port/src -o "$OUT/$mode$suffix.host.o"
    fi
    # shellcheck disable=SC2086
    clang "${COMMON[@]}" $flags -Wall -Wextra -Werror \
        -DRESULTS_GS_BYTES="$GS_BYTES" \
        -DBATTLE_RUNTIME_SOURCE="\"$runtime\"" \
        -c pc_port/tests/battle_results_host_differential_test.c \
        -o "$OUT/$mode$suffix.test.o"
    # shellcheck disable=SC2086
    clang "${COMMON[@]}" $flags -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode$suffix.cpu.o"

    # As in run_battle_overlay_host_differential_test.sh: anything the
    # runtime or the stub manifest references that nothing here defines gets
    # a counted placeholder; the prover fails any case that reaches one.
    local link=(clang -no-pie -rdynamic $flags -Wl,--gc-sections
                -Wl,--wrap=PcPort_BattleMipsResolve)
    local extra=()
    if ! "${link[@]}" -Wl,--no-demangle "${objs[@]}" -ldl -o "$target" 2>"$err"; then
        local missing
        missing=$(grep -oE "undefined reference to \`[A-Za-z0-9_]+'" "$err" \
            | sed "s/.*\`//; s/'//" | sort -u | grep -v '^__real_' || true)
        if [ -z "$missing" ]; then
            cat "$err" >&2
            echo "BATTLE RESULTS DIFFERENTIAL FAIL $mode$suffix link failed" >&2
            exit 1
        fi
        {
            echo '/* Generated by run_battle_results_host_differential_test.sh. */'
            echo '#include <stdio.h>'
            echo 'extern unsigned g_prover_placeholder_hits;'
            echo 'static void unresolved(const char *name) {'
            echo '    g_prover_placeholder_hits++;'
            echo '    fprintf(stderr, "[prover] unavailable callee %s\n", name);'
            echo '}'
            for sym in $missing; do
                case "$sym" in
                    D_*|g_*|jtbl_*|jpt_*|str_*|aD_*|aG_*|gteRegs|activeDispEnv|activeDrawEnv|currentDispEnv)
                        echo "unsigned char $sym[0x10000];" ;;
                    *)
                        echo "void $sym(void) { unresolved(\"$sym\"); }" ;;
                esac
            done
        } > "$stub_c"
        clang -c "$stub_c" -O0 -o "$OUT/$mode$suffix-placeholders.o"
        extra=("$OUT/$mode$suffix-placeholders.o")
        printf 'BATTLE RESULTS DIFFERENTIAL placeholders=%s\n' \
            "$(echo "$missing" | wc -l)"
    fi
    "${link[@]}" "${objs[@]}" "${extra[@]}" -ldl -o "$target"
}

RUNTIME="$PWD/pc_port/src/battle_mips_runtime.c"
# The retail meter battle lists 0x801E3A18 among its interpreted targets
# (any rank); once 0x801E3A18 itself is adopted the native battle must show
# no interpreted work at all and exactly one native results call.
caller_native=0
case " $(echo $LEAVES) " in *" func_801E3A18 "*) caller_native=1 ;; esac
for mode in ${BATTLE_RESULTS_DIFFERENTIAL_MODES:-O0 O2 UBSan}; do
    build_link "$mode" pc_port/src "$RUNTIME" ""
    status=0
    env -u XENO_CPU_FALLBACK_STATS "$OUT/$mode" "$OUT/results_module.bin" \
        >"$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    if [ "$status" -ne 0 ] || ! grep -q '^BATTLE RESULTS DIFFERENTIAL PASS ' "$OUT/$mode.log"; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL mode=$mode status=$status" >&2
        exit 1
    fi
    # The CPU-fallback meter is off by default: not one line of it.
    if grep -q 'cpu-fallback' "$OUT/$mode.log"; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL mode=$mode meter output while off" >&2
        exit 1
    fi
    # On: the meter self-check (counts against cpu.steps and the known call
    # sequence of 0x801E3A18) and its stderr report format.
    status=0
    XENO_CPU_FALLBACK_STATS=1 "$OUT/$mode" "$OUT/results_module.bin" --meter \
        >"$OUT/$mode-meter.log" 2>&1 || status=$?
    cat "$OUT/$mode-meter.log"
    summary='^\[xeno-port\]\[cpu-fallback\] battle=[12] end=returned interpreted_instructions=[0-9]+ interpreted_calls=[0-9]+ disabled_dispatches=0 interpreted_targets=[0-9]+ untracked_calls=0 native_main_exe_calls=0 native_overlay_calls=0 native_results_calls=[0-9]+ native_file1_calls=0 native_call_share=[0-9.]+%$'
    if [ "$status" -ne 0 ] ||
       ! grep -q '^BATTLE RESULTS DIFFERENTIAL METER PASS ' "$OUT/$mode-meter.log" ||
       [ "$(grep -cE "$summary" "$OUT/$mode-meter.log")" -ne 2 ] ||
       ! grep -qE '^\[xeno-port\]\[cpu-fallback\] +[0-9]+\. 0x801e3a18 calls=1 ' "$OUT/$mode-meter.log" ||
       { [ "$caller_native" = 1 ] &&
         ! grep -qE '^\[xeno-port\]\[cpu-fallback\] battle=2 end=returned interpreted_instructions=0 interpreted_calls=0 disabled_dispatches=0 interpreted_targets=0 .* native_results_calls=1 ' "$OUT/$mode-meter.log"; }; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL mode=$mode meter status=$status" >&2
        exit 1
    fi
    # 0x801E3A18 model composed with the adopted native helpers (not
    # dispatched; see compose_run), then a mutant that drops their exit
    # registers, which must fail.
    status=0
    env -u XENO_CPU_FALLBACK_STATS -u XENO_COMPOSE_3A18_MUTANT "$OUT/$mode" \
        "$OUT/results_module.bin" --compose-3a18 >"$OUT/$mode-compose.log" 2>&1 || status=$?
    cat "$OUT/$mode-compose.log"
    if [ "$status" -ne 0 ] || ! grep -q '^BATTLE RESULTS COMPOSE 3A18 PASS ' "$OUT/$mode-compose.log"; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL mode=$mode compose-3a18 status=$status" >&2
        exit 1
    fi
    status=0
    env -u XENO_CPU_FALLBACK_STATS XENO_COMPOSE_3A18_MUTANT=drop-exit-regs "$OUT/$mode" \
        "$OUT/results_module.bin" --compose-3a18 >"$OUT/$mode-compose-mutant.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! grep -q 'compose-3a18.*exit-registers' "$OUT/$mode-compose-mutant.log"; then
        echo "BATTLE RESULTS DIFFERENTIAL FAIL mode=$mode compose mutant drop-exit-regs survived" >&2
        exit 1
    fi
    echo "BATTLE RESULTS DIFFERENTIAL compose-3a18 negative control PASS mode=$mode"
done

# expect_rejected <suffix> <host dir> <runtime> <required FAIL pattern>
expect_rejected() {
    local suffix="$1" hostdir="$2" runtime="$3" pattern="$4"
    build_link O2 "$hostdir" "$runtime" "$suffix"
    local status=0
    "$OUT/O2$suffix" "$OUT/results_module.bin" >"$OUT/control$suffix.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! grep -qE "$pattern" "$OUT/control$suffix.log"; then
        cat "$OUT/control$suffix.log" >&2
        echo "BATTLE RESULTS DIFFERENTIAL CONTROL FAILURE$suffix accepted (wanted /$pattern/)" >&2
        exit 1
    fi
    grep -m1 -E "$pattern" "$OUT/control$suffix.log" >&2
}

expect_rejected -wrong-body "$OUT/mutant-wrong-body" "$RUNTIME" \
    '^BATTLE RESULTS DIFFERENTIAL FAIL func_801E24B0 native-copy '
expect_rejected -raw-mapping "$OUT/mutant-raw-mapping" "$RUNTIME" \
    '^BATTLE RESULTS DIFFERENTIAL FAIL func_801E24B0 (native-copy|ram) '
expect_rejected -wrong-identity pc_port/src "$OUT/mutant-wrong-identity/battle_mips_runtime.c" \
    '^BATTLE RESULTS DIFFERENTIAL FAIL loaded module does not match the pinned identity'

echo "BATTLE RESULTS DIFFERENTIAL PASS (O0/O2/UBSan + meter + wrong-body, raw-mapping, wrong-identity controls rejected)"
