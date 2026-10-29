#!/usr/bin/env bash
#
# Phase 1 link driver for the Xenogears native PC port.
#
# Strategy (boot-path-driven): compile every game translation unit that compiles
# in port mode, link them against PsyCross, discover the undefined references
# (everything not yet decompiled), auto-generate logging stubs for exactly those,
# and link the whole xeno-port executable. Running it then reveals the first
# missing function on the live execution path (the oracle).
#
# Run inside a Linux toolchain env (distrobox/container) with:
#   gcc g++ cmake make pkg-config binutils python3 libsdl2-dev libopenal-dev libgl1-mesa-dev libssl-dev
#
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PSX="pc_port/extern/PsyCross"
OUT="pc_port/build_native"
OBJ="$OUT/obj"

# Concurrency-validation build (the third regime, alongside objdiff + behavioral):
# XENO_TSAN=1 rebuilds PsyCross + game TUs + stubs + link with ThreadSanitizer
# into separate build dirs (pc_port/build_tsan, pc_port/build_native_tsan) so
# the normal artifacts stay untouched. Used to prove the sound tick gate
# (g_SoundTickMutex) holds: zero data races on sound shared state under
# main-thread-vs-240Hz-tick contention.
#
# XENO_ASAN=1 is the same regime with AddressSanitizer instead, for memory
# errors (heap overruns in port-side host allocations) rather than races. It
# uses its own build dirs (pc_port/build_asan, pc_port/build_native_asan) and
# is mutually exclusive with XENO_TSAN.
TSAN_FLAGS=""
PSYX_BUILD="pc_port/build"
COV_OBJ=""
if [ "${XENO_COVERAGE:-0}" = "1" ] && { [ "${XENO_TSAN:-0}" = "1" ] || [ "${XENO_ASAN:-0}" = "1" ]; }; then
    echo "ERROR: XENO_COVERAGE is mutually exclusive with XENO_TSAN / XENO_ASAN."
    exit 1
fi
if [ "${XENO_TSAN:-0}" = "1" ] && [ "${XENO_ASAN:-0}" = "1" ]; then
    echo "ERROR: XENO_TSAN and XENO_ASAN are mutually exclusive."
    exit 1
fi
if [ "${XENO_TSAN:-0}" = "1" ]; then
    TSAN_FLAGS="-fsanitize=thread"
    PSYX_BUILD="pc_port/build_tsan"
    OUT="pc_port/build_native_tsan"
    OBJ="$OUT/obj"
    echo "==> XENO_TSAN=1: ThreadSanitizer build -> $OUT"
elif [ "${XENO_COVERAGE:-0}" = "1" ]; then
    # Function coverage for the preservation report: every game, port and
    # PsyCross function entry is recorded (pc_port/tools/coverage_recorder.c,
    # tools/analysis/port_preservation.py).  Own build dirs.
    TSAN_FLAGS="-finstrument-functions"
    PSYX_BUILD="pc_port/build_cov"
    OUT="pc_port/build_native_cov"
    OBJ="$OUT/obj"
    COV_OBJ="$OBJ/coverage_recorder.o"
    echo "==> XENO_COVERAGE=1: function-coverage build -> $OUT"
elif [ "${XENO_ASAN:-0}" = "1" ]; then
    # -fno-omit-frame-pointer keeps ASan's reports readable; the sanitizer is
    # otherwise configured entirely through ASAN_OPTIONS at run time.
    # -static-libasan is required, not cosmetic: the port runs against the
    # prebuilt SDL2/OpenAL in xenogears-assets/lib via LD_LIBRARY_PATH, which
    # get loaded ahead of a dynamic libasan and trip ASan's "runtime does not
    # come first in initial library list" bail-out (LD_PRELOAD does not fix
    # it here). Linking the runtime in statically sidesteps the ordering.
    TSAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer -static-libasan"
    PSYX_BUILD="pc_port/build_asan"
    OUT="pc_port/build_native_asan"
    OBJ="$OUT/obj"
    echo "==> XENO_ASAN=1: AddressSanitizer build -> $OUT"
fi
# Native-only development target: keep the playable hybrid build, but omit
# the CPU instruction engine from this executable. Missing native bodies
# stop at their call boundary. This is a conversion target, not evidence that
# every game path is implemented. Rendering/audio and game-script VMs remain.
if [ "${XENO_NATIVE_ONLY:-0}" = "1" ]; then
    OUT="${OUT}_only"
    OBJ="$OUT/obj"
    echo "==> XENO_NATIVE_ONLY=1: CPU interpreter excluded -> $OUT"
fi
mkdir -p "$OBJ"
if [ -n "$TSAN_FLAGS" ] && [ -f pc_port/build_native/stubs.c ] && [ ! -f "$OUT/stubs.c" ]; then
    # The sanitizer variants mirror the normal build: reuse its validated stub
    # manifest (without matching ELFs the generator cannot create one, and
    # the undef set is identical by construction).
    cp pc_port/build_native/stubs.c "$OUT/stubs.c"
fi

# This must run in an environment with the toolchain + libs (the distrobox on
# Bazzite, NOT the immutable host). Fail fast with guidance if it's the wrong one.
for tool in cmake gcc pkg-config python3 ar nm objcopy; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "ERROR: '$tool' not found in this shell."
        echo "       Run inside the dev container:  distrobox enter xenogears-dev"
        echo "       (the host base system is immutable and lacks the toolchain.)"
        exit 1
    }
done
pkg-config --exists sdl2 2>/dev/null || {
    echo "ERROR: SDL2 development files not found."
    echo "       You are probably on the host. Run:  distrobox enter xenogears-dev"
    exit 1
}

pkg-config --atleast-version=3.0 libcrypto 2>/dev/null || {
    echo "ERROR: OpenSSL 3 development files required for verified KROM loading (Ubuntu: libssl-dev)."
    exit 1
}
INC="-Ipc_port/include_shim -Ipc_port/include -Iinclude -I$OUT -I$PSX/include -I$PSX/include/psx $(pkg-config --cflags libcrypto)"
# -std=gnu17: the game predates C23; gcc >= 15 defaults to C23 and rejects it.
# -fpermissive: gcc >= 14 promotes old-C constructs (implicit decls, int/pointer
#   conversions) to hard errors that -w can't silence; -fpermissive demotes them.
# -DUSE_EXTENDED_PRIM_POINTERS=0: use PsyCross's simple (non-PGXP) primitive
#   pipeline. The extended/PGXP path (default 1) routes 2D prims through a
#   perspective/offscreen path that doesn't render here, and its 24-byte SPRT
#   layout mismatches the game's hardcoded 0x10 strides (font.c). MUST match the
#   value PsyCross's lib is built with (see pc_port/CMakeLists.txt).
# XENO_FIELD_OBJECT_OVERLAY: activate func_800A1364's retail object-register
# body. The native owner for the field-local 0x6B9 overlay is compiled from
# pc_port/src/field_object_overlay.c; 0x801E742C/738C/7D14/8030/8330 are real
# entry points in that retail archive, not member-change-menu mid-functions.
# Strip the build machine's absolute paths from __FILE__/assert strings and
# debug info, so no personal infrastructure path ships in the binary
# (tools/analysis/psyq_port_guard.py --binary "path" check).
PREFIX_MAP="-ffile-prefix-map=$ROOT=."
GFLAGS="-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -w -O0 -g -m64 -fno-builtin $PREFIX_MAP"
# Optional compile-time diagnostics, for example:
# XENO_DIAG_DEFINES=-DXENO_DIAG_OPCODE_SWEEP ./scratchpad/run_build_port.sh
# Leave GFLAGS byte-for-byte unchanged when unset.
if [ -n "${XENO_DIAG_DEFINES:-}" ]; then
    GFLAGS="$GFLAGS $XENO_DIAG_DEFINES"
fi
# Sanitizer instrumentation for game TUs (see XENO_TSAN / XENO_ASAN above).
# GFLAGS stays byte-for-byte unchanged when unset.
if [ -n "$TSAN_FLAGS" ]; then
    GFLAGS="$GFLAGS $TSAN_FLAGS"
fi
if [ "${XENO_NATIVE_ONLY:-0}" = "1" ]; then
    GFLAGS="$GFLAGS -DXENO_NATIVE_ONLY"
fi

# Build-integrity policy:
#
# Every game TU is expected to compile and link. Explicit exclusions are
# deliberately neither compiled nor linked. REFERENCE_ONLY_GAME_TUS have a
# permanent port-side runtime owner while their retail source remains in the
# matching build. The remaining exclusions are temporary HOLDs whose source
# now compiles but cannot safely enter the port link until routing/decomp work
# is resolved. In contrast, KNOWN_BROKEN_GAME_TUS are compile failures tolerated
# temporarily. Do not add a TU to any list merely to make a build pass; each
# entry needs a reviewed ownership decision or issue.
INTENTIONALLY_EXCLUDED_GAME_TU_PATTERNS=(
    "*/psyq/*"
)
INTENTIONALLY_EXCLUDED_GAME_TUS=(
    "src/battling/main.c"
    "src/slus_006.64/system/archive.c"
)
REFERENCE_ONLY_GAME_TUS=(
    "src/slus_006.64/system/work_list.c"
    "src/battle/main.c"
    "src/battle/main2.c"
    "src/battle/main3.c"
    "src/battle/main4.c"
    "src/battle/main5.c"
    "src/battle/main6.c"
    "src/battle/main7.c"
    "src/battle/main8.c"
    "src/battle/main9.c"
    "src/battle/main10.c"
    "src/battle/main11.c"
    "src/battle/main12.c"
    "src/battle/main13.c"
    "src/battle/main14.c"
    "src/battle/mainc15.c"
    "src/battle/main16.c"
    "src/battle/main17.c"
    "src/battle/mainc18_q1.c"
    "src/battle/mainc18_q2.c"
    "src/battle/main19.c"
    "src/battle/main20.c"
    "src/battle/main21.c"
    "src/battle/main22.c"
    "src/battle/main23.c"
    "src/battle/main24.c"
    "src/battle/mainc25.c"
    "src/battle/main26.c"
    "src/battle/main27.c"
    "src/battle/main28.c"
    "src/battle/mainc29.c"
    "src/battle/main30.c"
    "src/battle/main31.c"
    "src/battle/main32.c"
    "src/battle/main33.c"
    "src/battle/main34.c"
    "src/battle/main35.c"
    "src/battle/main35_p1.c"
    "src/battle/main35_p2.c"
    "src/battle/main35_p4.c"
    "src/battle/main36.c"
    "src/battle/main36_p2.c"
    "src/battle/main37.c"
    "src/battle/main38.c"
    "src/battle/main38_p2.c"
    "src/battle/main39.c"
    "src/battle/main40.c"
    "src/battle/main40_p2.c"
    "src/battle/main41.c"
    "src/battle/main42.c"
    "src/battle/main43.c"
    "src/battle/main44.c"
    "src/battle/main45.c"
    "src/battle/main46.c"
    "src/battle/main47.c"
    "src/battle/main48.c"
    "src/battle/main49.c"
    "src/battle/main50.c"
    "src/battle/main51.c"
    "src/battle/main52.c"
    "src/battle/main53.c"
    "src/battle/main54.c"
    "src/battle/main55.c"
    "src/battle/main56.c"
    "src/battle/main57.c"
    "src/battle/main58.c"
    "src/battle/main59.c"
    "src/battle/main60.c"
    "src/battle/main61.c"
    "src/battle/main62.c"
    "src/battle/main63.c"
    "src/battle/main64.c"
    "src/battle/main65.c"
    "src/battle/main66.c"
    "src/battle/main67.c"
    "src/battle/main68.c"
    "src/battle/main69.c"
    "src/battle/main70.c"
    "src/battle/main71.c"
    "src/battle/main72.c"
    "src/battle/main72_p1.c"
    "src/battle/main72_p2.c"
    "src/battle/main72_p3.c"
    "src/battle/main73.c"
    "src/battle/main73_p1.c"
    "src/battle/main73_p2.c"
    "src/battle/main74.c"
    "src/battle/main75.c"
    "src/battle/mainc76.c"
    "src/battle/main77.c"
    "src/battle/mainc78.c"
    "src/battle/mainc79.c"
    "src/battle/mainc80.c"
    "src/battle/mainl81.c"
    "src/battle/mainc82.c"
    "src/battle/mainc83.c"
    "src/battle/mainc84.c"
    "src/battle/mainc85.c"
    "src/battle/mainc86.c"
    "src/battle/main87.c"
    "src/battle/mainc88.c"
    "src/battle/mainc89.c"
    "src/battle/main90.c"
    "src/battle/main91.c"
    "src/battle/mainl92.c"
    "src/battle/main93.c"
    "src/battle/mainl94.c"
    "src/battle/mainc95.c"
    "src/battle/main96.c"
    "src/battle/mainc97.c"
    "src/battle/mainl98.c"
    "src/battle/mainc99.c"
    "src/battle/mainc100.c"
    "src/battle/main101.c"
    "src/battle/mainc102.c"
    "src/battle/main103.c"
    "src/battle/mainc104.c"
    "src/battle/main105.c"
    "src/battle/main106.c"
    "src/battle/mainc107.c"
    "src/battle/mainc108.c"
    "src/battle/mainl109.c"
    "src/battle/mainl110.c"
    "src/battle/main111.c"
    "src/battle/mainc112.c"
    "src/battle/mainc113.c"
    "src/battle/mainc114.c"
    "src/battle/mainc114_p1.c"
    "src/battle/mainc114_p2.c"
    "src/battle/mainc115.c"
    "src/battle/mainl115.c"
    "src/battle/mainc116.c"
    "src/battle/mainc117.c"
    "src/battle/mainc118.c"
    "src/battle/main119.c"
    "src/battle/mainc120.c"
    "src/battle/main121.c"
    "src/battle/mainc122.c"
    "src/battle/mainc123.c"
    "src/battle/mainc124.c"
    "src/battle/main125.c"
    "src/battle/mainc126.c"
    "src/battle/mainc127.c"
    "src/battle/main128.c"
    "src/battle/mainc129.c"
    "src/battle/mainc130.c"
    "src/battle/mainc131.c"
    "src/battle/mainc132.c"
    "src/battle/mainl133.c"
    "src/battle/mainc134.c"
    "src/battle/main135.c"
    "src/battle/main125_q1.c"
    "src/battle/main125_q2.c"
    "src/battle/main17_q1.c"
    "src/battle/main17_q2.c"
    "src/battle/main19_q1.c"
    "src/battle/main19_q2.c"
    "src/battle/main22_q1.c"
    "src/battle/main22_q2.c"
    "src/battle/main34_q1.c"
    "src/battle/main34_q2.c"
    "src/battle/main35_p3_q1.c"
    "src/battle/main35_p3_q2.c"
    "src/battle/main37_q1.c"
    "src/battle/main37_q2.c"
    "src/battle/mainc25_q1.c"
    "src/battle/mainc25_q2.c"
    "src/battle/mainc29_q1.c"
    "src/battle/mainc29_q2.c"
)
KNOWN_BROKEN_GAME_TUS=(
)

known_broken_tu_reason() {
    case "$1" in
    esac
}

is_known_broken_game_tu() {
    local candidate="$1"
    local broken
    for broken in "${KNOWN_BROKEN_GAME_TUS[@]}"; do
        [ "$candidate" = "$broken" ] && return 0
    done
    return 1
}

is_intentionally_excluded_game_tu() {
    local candidate="$1"
    local excluded
    local pattern

    for excluded in "${INTENTIONALLY_EXCLUDED_GAME_TUS[@]}"; do
        [ "$candidate" = "$excluded" ] && return 0
    done
    for pattern in "${INTENTIONALLY_EXCLUDED_GAME_TU_PATTERNS[@]}"; do
        [[ "$candidate" == $pattern ]] && return 0
    done
    return 1
}

is_reference_only_game_tu() {
    local candidate="$1"
    local reference
    for reference in "${REFERENCE_ONLY_GAME_TUS[@]}"; do
        [ "$candidate" = "$reference" ] && return 0
    done
    return 1
}

reference_only_game_tu_reason() {
    case "$1" in
        src/battle/main.c)
            echo "retail MIPS overlay scaffold; runtime executes disc/battle.bin through pc_port/src/battle_mips_runtime.c" ;;
        src/battle/main*.c)
            echo "retail MIPS overlay scaffold (asm stubs for the un-decompiled battle runs); runtime executes disc/battle.bin" ;;
        src/slus_006.64/system/work_list.c)
            echo "runtime replaced by pc_port/src/work_list_port.c (packed 0x1C entry layout)" ;;
    esac
}

excluded_game_tu_reason() {
    case "$1" in
        "*/psyq/*")
            echo "PsyQ originals are replaced at runtime by PsyCross" ;;
        src/battling/main.c)
            echo "retail state-4 MIPS overlay scaffold; not a native host translation unit" ;;
        src/slus_006.64/system/archive.c)
            echo "replaced by pc_port/src/archive_port.c" ;;
    esac
}

print_reference_only_game_tus() {
    echo "    Reference-only game TUs (matching source; replaced in the port link):"
    local reference
    for reference in "${REFERENCE_ONLY_GAME_TUS[@]}"; do
        echo "      $reference — $(reference_only_game_tu_reason "$reference")"
    done
}

print_intentionally_excluded_game_tus() {
    echo "    Intentionally excluded game TUs (not compiled or linked):"
    local excluded
    local pattern
    for excluded in "${INTENTIONALLY_EXCLUDED_GAME_TUS[@]}"; do
        echo "      $excluded — $(excluded_game_tu_reason "$excluded")"
    done
    for pattern in "${INTENTIONALLY_EXCLUDED_GAME_TU_PATTERNS[@]}"; do
        echo "      $pattern — $(excluded_game_tu_reason "$pattern")"
    done
}

print_known_broken_game_tus() {
    if [ "${#KNOWN_BROKEN_GAME_TUS[@]}" -eq 0 ]; then
        echo "    Allowlisted broken game TUs (compile failures tolerated): none"
        return
    fi
    echo "    WARNING: building with explicitly allowlisted broken game TUs:"
    local broken
    for broken in "${KNOWN_BROKEN_GAME_TUS[@]}"; do
        echo "      $broken — $(known_broken_tu_reason "$broken")"
    done
    echo "    WARNING: these are temporary exceptions; any other game-TU compile failure aborts."
}


# PsyCross pin (pc_port/psycross.lock, license in pc_port/THIRD_PARTY.md).
# Clone at the pinned commit when absent; refuse a tree not based on the pin.
ensure_psycross_pinned() {
    local lock="$ROOT/pc_port/psycross.lock"
    local url pin git_bin
    url="$(sed -n 's/^url=//p' "$lock")"
    pin="$(sed -n 's/^commit=//p' "$lock")"
    if [ -z "$url" ] || [ -z "$pin" ]; then
        echo "ERROR: $lock must define url= and commit=" >&2
        exit 1
    fi
    git_bin="$(command -v git || true)"
    if [ -z "$git_bin" ] && [ -x /run/host/usr/bin/git ]; then
        git_bin=/run/host/usr/bin/git
    fi
    if [ -z "$git_bin" ]; then
        echo "ERROR: git is required to fetch/verify PsyCross" >&2
        exit 1
    fi
    if [ ! -e "$PSX" ]; then
        echo "    cloning PsyCross ${pin:0:12} from ${XENO_PSYCROSS_URL:-$url}"
        mkdir -p "$(dirname "$PSX")"
        "$git_bin" clone -q "${XENO_PSYCROSS_URL:-$url}" "$PSX" &&
            "$git_bin" -C "$PSX" -c advice.detachedHead=false checkout -q "$pin" || {
            echo "ERROR: could not clone PsyCross at $pin" >&2
            rm -rf "$PSX"
            exit 1
        }
    fi
    [ "${XENO_PSYCROSS_ALLOW_UNPINNED:-0}" = "1" ] && {
        echo "    WARNING: XENO_PSYCROSS_ALLOW_UNPINNED=1, PsyCross pin not verified"
        return 0
    }
    local psx_abs toplevel
    psx_abs="$(cd "$PSX" && pwd)"
    toplevel="$("$git_bin" -C "$PSX" rev-parse --show-toplevel 2>/dev/null || true)"
    if [ "$toplevel" != "$psx_abs" ] || \
       ! "$git_bin" -C "$PSX" merge-base --is-ancestor "$pin" HEAD 2>/dev/null; then
        echo "ERROR: $PSX is not a git checkout based on the pinned PsyCross $pin" >&2
        echo "       Move it aside and rerun to clone the pin (pc_port/THIRD_PARTY.md)." >&2
        exit 1
    fi
    local extra
    extra="$("$git_bin" -C "$PSX" log --format=%s "$pin..HEAD" | grep -vx 'vendor baseline for patch apply' || true)"
    if [ -n "$extra" ]; then
        echo "ERROR: $PSX has upstream/unknown commits beyond the pin $pin:" >&2
        echo "$extra" | sed 's/^/       | /' >&2
        exit 1
    fi
    echo "    PsyCross pinned at ${pin:0:12} (MIT)"
}
ensure_psycross_pinned

# PsyCross fidelity fixes kept as tracked patches because the vendored tree is
# gitignored. Apply in dependency order: the ABR patch was generated after the
# raw-texture dither correction.
#
# PsyCross is an untracked vendor tree. `git -C $PSX apply` only works correctly
# when $PSX is its own git worktree. Without a local .git, git walks up to the
# monorepo, reverse --check can false-positive, and optional F22/F24/F26/F28
# patches are silently skipped. Ensure a local vendor repo before applying.
ensure_psycross_git_worktree() {
    local git_bin="$1"
    local psx_abs
    psx_abs="$(cd "$PSX" && pwd)"
    local toplevel
    toplevel="$("$git_bin" -C "$PSX" rev-parse --show-toplevel 2>/dev/null || true)"
    if [ -n "$toplevel" ] && [ "$toplevel" = "$psx_abs" ]; then
        return 0
    fi
    echo "    initializing local PsyCross git worktree for patch apply"
    "$git_bin" -C "$PSX" init -q
    "$git_bin" -C "$PSX" -c user.email=xeno@local -c user.name=xeno add -A
    "$git_bin" -C "$PSX" -c user.email=xeno@local -c user.name=xeno \
        commit -q -m "vendor baseline for patch apply" || true
    toplevel="$("$git_bin" -C "$PSX" rev-parse --show-toplevel 2>/dev/null || true)"
    if [ "$toplevel" != "$psx_abs" ]; then
        echo "ERROR: PsyCross git toplevel is '$toplevel', expected '$psx_abs'" >&2
        exit 1
    fi
}

apply_psycross_patch() {
    local patch="$1"
    local marker="$2"
    local patch_mode="${3:-}"
    local apply_args=()
    if [ "$patch_mode" = "unidiff-zero" ]; then
        apply_args+=(--unidiff-zero)
    fi
    if grep -Rqs "$marker" "$PSX"; then
        return
    fi
    local git_bin="$(command -v git || true)"
    if [ -z "$git_bin" ] && [ -x /run/host/usr/bin/git ]; then
        git_bin=/run/host/usr/bin/git
    fi
    if [ -z "$git_bin" ]; then
        echo "ERROR: git is required to apply PsyCross source patches" >&2
        exit 1
    fi
    ensure_psycross_git_worktree "$git_bin"
    if "$git_bin" -C "$PSX" apply "${apply_args[@]}" --reverse --check "$patch" >/dev/null 2>&1; then
        return
    fi
    "$git_bin" -C "$PSX" apply "${apply_args[@]}" --check "$patch" || {
        echo "ERROR: PsyCross patch does not apply: $patch" >&2
        exit 1
    }
    "$git_bin" -C "$PSX" apply "${apply_args[@]}" "$patch"
}

apply_psycross_patch "$ROOT/pc_port/patches/psycross_port_prelude.patch" "_xeno_drmove_snapshot_decl"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_image_open.patch" "_xeno_cd_image_open"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_cue_checked.patch" "_xeno_cd_cue_checked"
# PsyCross carries a non-retail, unused DRAWENV.drt byte.  Removing it restores
# PsyQ's 0x5C DRAWENV ABI so the game's literal +0x5C/+0xB8 DISPENV accesses
# address the same data as typed host code.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_retail_drawenv_layout.patch" "_xeno_retail_drawenv_layout"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_pad_include.patch" "_xeno_pad_stdlib"
# Preserve SDL key-down edges until the next pad sample.  Render/present paths
# may poll and drain both halves of a quick tap before PsyX_UpdateInput samples
# SDL_GetKeyboardState; without this latch normal keyboard taps vanish.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_keyboard_tap_latch.patch" "_xeno_keyboard_tap_latch"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_raw_texture_dither.patch" "_xeno_raw_texture_dither"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_abr_clut_bit15.patch" "_xeno_clut_bit15_abr"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_compmatrix_alias.patch" "_xeno_compmatrix_alias"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_compmatrix_word_store.patch" "_xeno_compmatrix_word_store"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_compmatrix_translation_word.patch" "_xeno_compmatrix_translation_word"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_compmatrix_translation_overflow.patch" "_xeno_compmatrix_translation_overflow"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sqrt_lz_registers.patch" "_xeno_sqrt_lz_registers"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sqrt_retail_memory.patch" "_xeno_sqrt_retail_memory"
# The port ships no game data: LIBGTE.C's retail rcossin/ratan tables are
# loaded from the user's disc/SLUS_006.64 (retail_data.h); SQRT is dropped
# (SquareRoot0 reads guest memory since the previous patch).
apply_psycross_patch "$ROOT/pc_port/patches/psycross_gte_tables_from_disc.patch" "_xeno_gte_tables_from_disc"
# Replace only the PushMatrix/PopMatrix failure blocks with host diagnostics.
# Structural rewriting keeps the original text out of tracked patch payloads
# and also repairs checkouts that received the old malformed insertion patch.
python3 "$ROOT/pc_port/tools/fix_psycross_gte_stack_messages.py" "$PSX/src/psx/LIBGTE.C" || exit 1
apply_psycross_patch "$ROOT/pc_port/patches/psycross_gte_mvmva_signed_translation.patch" "_xeno_gte_mvmva_signed_translation"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_gte_projection_signed_translation.patch" "_xeno_gte_projection_signed_translation"
# Optional F22 logical-origin correction. This is intentionally applied only
# by the on-screen non-PGXP branch in GR_SetOffscreenState; unset/0 retains the
# original GR_Ortho2D call byte-for-byte, while offscreen VRAM and PGXP paths
# remain untouched.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_halfpixel_origin.patch" "_xeno_half_pixel_origin"
# Optional F24 texel-center correction. This is independent of the F22
# coverage-origin correction and is enabled only for on-screen non-PGXP
# textured draws; offscreen VRAM and PGXP paths force the shader uniform off.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_texel_center.patch" "_xeno_texel_center" "unidiff-zero"
# Optional F26 exact fixed-point UV interpolation. Polygon producers mark only
# FT3/FT4/GT3/GT4 vertices; the renderer then uses the PCSX-Redux edge/span
# arithmetic for on-screen non-PGXP genuine-PS1 textured triangles. Its
# internal +0x8000 replaces F24's shader offset while active. Sprites, tiles,
# glyphs, placeholders, PGXP, offscreen VRAM, and the disabled path stay on
# their existing routes.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_fixed_uv.patch" "_xeno_fixed_uv_option" "unidiff-zero"
# Optional F28 exact raw-texture modulation identity. A dedicated vertex
# marker selects raw FT3/FT4 polygons only; the uniform is restricted to
# on-screen non-PGXP PS1-textured draws and is disabled by default.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_raw_texture_identity.patch" "_xeno_raw_identity_option" "unidiff-zero"
# SPRT/SPRT_8/SPRT_16 rectangles share MakeTexcoordRect. Pair their upper
# screen edge with the base V so PSX top-down VRAM rows stay upright in the
# presented window; polygon UV paths remain independent.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sprite_v_orientation.patch" "_xeno_sprite_v_orientation" "unidiff-zero"
# World-map acceptance captures are requested by the game loop but fulfilled
# at PsyX_EndScene's sole effective pre-swap presentation boundary.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_world_capture_request.patch" "_xeno_world_capture_request" "unidiff-zero"
# OpenGL framebuffer readback starts at the lower-left. Flip complete RGBA
# rows before SDL serializes screenshots so capture files match presentation.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_capture_readback_orientation.patch" "_xeno_capture_readback_orientation" "unidiff-zero"
# Recording extends the direct-display presentation seam, and the toolbar
# then extends both.  Keep that dependency order reproducible from the vendor
# baseline instead of relying on an already-patched working tree.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_display_present.patch" "_xeno_display_present"
# Host-only F9 video capture. The recorder reads the completed backbuffer at
# both presentation boundaries before swap and streams raw frames to ffmpeg.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_video_recording.patch" "_xeno_video_recording"
# Clickable host controls occupy pixels reserved above the PSX framebuffer;
# capture readback remains limited to the retail game region below the bar.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_host_toolbar.patch" "_xeno_host_toolbar"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_port_dev_menu_input.patch" "_xeno_dev_menu_input"
# Texture-cache format key (F10): GR_SetTexture's cache early-returned on
# texture ID alone (PsyX_render.cpp GR_SetTexture), and the return fires
# BEFORE the per-shader sampler uniforms (u_tex=0/u_lut=1) are initialized.
# A same-ID 4-bit -> 8-bit transition (observed on texture ID 5 in Lahan)
# left the 8-bit shader's LUT sampler at its GL default 0, so it decoded
# VRAM as its own palette (yellow/black model corruption). The cache key is
# now (texture ID, texture format); both cached fields reset together in
# GR_BeginScene and update only AFTER the sampler uniforms are set, so a
# failure between bind and uniform-set cannot poison the cache.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_texcache_format_key.patch" "_xeno_texcache_format_key"
# Sound SDK Phase 0+1: SPU backend accessor + the RCnt2 (counter-2) event pump
# (OpenEvent/EnableEvent/DisableEvent registry + 240Hz dispatch on the interrupt
# thread). See OPEN_ISSUES.md "Sound cold-init" and pc_port/src/port_main.c.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_pump.patch" "_xeno_sound_pump"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_present_vram_full_frame.patch" "_xeno_present_vram_full_frame"
# Sound SDK Phase 2 (init-proof): wire the 5 init-reached SDK primitives against
# the awake backend -- SpuSetReverbModeType/Depth + SpuSetReverbModeParam/Get
# (shared reverb state + EFX preset table -> OpenAL reverb), SpuSetCommonAttr
# (master volume -> listener gain), SpuSetIRQ/SpuSetIRQCallback (state only; no
# SPU IRQ source in the port). Behavioral, not objdiff-matched. See OPEN_ISSUES.md.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_prims.patch" "_xeno_sound_prims"
# Sound tick gate (tick-leg step 1, gate-first): g_SoundTickMutex, a recursive
# mutex mapping retail's DisableEvent/EnableEvent(g_unk_SoundEvent) bracket.
# DisableEvent on the counter-2 tick event acquires-and-holds, EnableEvent
# releases, the 240Hz pump TRY-locks around dispatch (a held bracket drops the
# tick, matching retail's disabled-event semantics; the pump can never stall
# or deadlock). Prerequisite for every real tick-body decomp. See OPEN_ISSUES.md.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_gate.patch" "_xeno_sound_gate"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_critical_declarations.patch" "_xeno_critical_declarations"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sw_critical_mask.patch" "_xeno_sw_critical_mask"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_event_delivery.patch" "_xeno_event_delivery"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_wait_event.patch" "_xeno_wait_event"
# Reservation primitive only; no production sound continuation uses it yet.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_reservation.patch" "_xeno_sound_reservation" "unidiff-zero"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_reservation_identity.patch" "_xeno_sound_reservation_identity" "unidiff-zero"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_reservation_cleanup.patch" "_xeno_sound_reservation_cleanup" "unidiff-zero"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_reservation_epoch.patch" "_xeno_sound_reservation_epoch" "unidiff-zero"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_reservation_owned.patch" "_xeno_sound_reservation_owned" "unidiff-zero"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_reservation_pristine.patch" "_xeno_sound_reservation_pristine"
# ADSR fidelity phase 1: per-voice hardware ADSR envelope (psx-spx 44100Hz
# counter machine) in the SPU backend -- KON=attack-from-zero, KOFF=release
# tail (source kept playing to level 0), composed single-writer AL_GAIN
# (base volume x envelope), live ENVX readback (SpuGetVoiceEnvelopeAttr, real
# -- backs seq cmd 0xFF's free-on-decay), ALC_REFRESH=240 render-granularity
# hint. Generated on top of the sound patches above (apply order matters).
# See OPEN_ISSUES.md "Audio fidelity: hardware-ADSR".
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_adsr.patch" "_xeno_sound_adsr"
# ADPCM/Gaussian fidelity (the streaming mix-stage): SPU-faithful per-voice
# synthesis via AL_SOFT_callback_buffer -- integer-exact ADPCM decode,
# hardware loop/End+Mute semantics, 4-tap Gaussian resampling at the live
# pitch counter, AL_PITCH pinned 1.0.  Mixer-thread state handoff under
# s_StreamMutex (never held across AL calls); legacy cubic path kept
# (XENO_SOUND_LEGACY_RESAMPLER=1 / emscripten / no-extension fallback).
# Generated on top of the sound patches above (apply order matters).
apply_psycross_patch "$ROOT/pc_port/patches/psycross_sound_adpcm.patch" "_xeno_sound_adpcm"
# STR movie streaming (movie player module, retail cdstream): CdControl
# CdlSetmode/CdlReadS(NULL) start the spooler from the CdControlB(CdlSetloc)
# position, CdControlB CdlSetfilter/CdlPause report success (retail spins on
# a zero return), CdlPause joins the spooler thread, and the spooler paces
# sector delivery at the drive rate (75/s, 150/s with CdlModeSpeed) so the
# 32-slot STR ring is not overrun.  Generated against the tree with the
# patches above applied (the cdsync hunk sits in the same function).
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_stream_movie.patch" "_xeno_cd_stream_movie"
# Host shutdown may race an active paced CdlReadS worker. Quiesce and join that
# worker before SDL_Quit invalidates SDL_Delay/event machinery.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_shutdown_cd_join.patch" "_xeno_cd_shutdown_join"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_image_ready.patch" "_xeno_cd_image_ready"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_complete_sector.patch" "_xeno_cd_complete_sector"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_pause_boundary.patch" "_xeno_cd_pause_boundary"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_bounded_seek.patch" "_xeno_cd_bounded_seek"
# Mirror completed framebuffer captures into CPU VRAM at the copy boundary.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_fb_mirror_readpixels.patch" "_xeno_fb_mirror_readpixels"
# The same FBO also reads VRAM for MoveImage presentation. Restore its staging
# attachment before the next capture; otherwise CPU VRAM receives stale pixels.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_framebuffer_staging_attachment.patch" "_xeno_fb_staging_attachment"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_framebuffer_materialize_rgb555.patch" "_xeno_fb_materialize_rgb555"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_framebuffer_materialize_rect.patch" "_xeno_fb_materialize_rect"
# Intro DR_MOVE can pass RECT.x < 0. Mask like GR_CopyVRAM instead of aborting.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_vram_copy_wrap.patch" "_xeno_vram_copy_wrap"
# GR_CopyVRAM masked the destination origin but never the extent, so a
# malformed MOVE_IMAGE (map 96: w=6432 h=1956 against 1024x512 VRAM) memcpy'd
# ~4MB past vram[]. Bound the row count to the end of vram[], keeping every
# in-bounds copy byte-identical, and log the first clamp.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_vram_copy_extent.patch" "_xeno_vram_copy_extent"

# Host testing speed: pace vblank, sound and streamed disc reads together,
# without incrementing game clocks on queries or bypassing frame waits.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_host_speed.patch" "_xeno_host_speed"
# The BIOS memory-card driver entry points (InitCARD/_card_info/...) are owned
# by pc_port/src/memcard_port.c; drop PsyCross's silent `return 0` copies.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_card_native_owner.patch" "_xeno_card_native_owner"
# Game controllers (pc_port/src/psycross_pad_select.inl): drop the handheld /
# InputPlumber entries (28de:1205/1206, 17ef:*) from Steam's inherited
# SDL_GAMECONTROLLER_IGNORE_DEVICES, open every pad but feed port 1 from one
# (Steam's virtual pad only as a last resort), reconcile hotplug by instance
# id, and let the left stick drive the digital pad's D-pad.  Generated on top
# of every PsyX_pad.cpp patch above (pad include, tap latch, dev menu input).
apply_psycross_patch "$ROOT/pc_port/patches/psycross_pad_select.patch" "_xeno_pad_select"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_dev_menu_values.patch" "_xeno_dev_menu_values"
# Host keys (owner spec): Backspace hold / F6 toggle fast-forward, F9 record,
# F10 screenshot, F11 quick save, F12 quick load, F7 speed cycle only with the
# dev-menu opt-in; no other host key actions.  Always-on, rate-limited input
# trace (pc_port/src/psycross_input_trace.inl) and background joystick events.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_host_hotkeys.patch" "_xeno_host_hotkeys"
# Diagnostic only: XENO_GPU_TRACE=a-b logs display/VRAM calls and each present
# (scene vs VRAM path, DISPENV, mask) for presented-frame indices a..b. Off and
# silent unless the variable is set.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_gpu_trace.patch" "_xeno_gpu_trace"
# activeDispEnv names the buffer PsyCross renders into; PutDispEnv guesses it
# from the double-buffer flip.  When VRAM-path presents already left the
# current DISPENV on the buffer the game re-puts, a display-sized draw clip can
# end up wholly outside activeDispEnv and the frame is offset/scissored off
# screen (field zoom-fade first frame: one black frame at the title).  Rebase
# only in that no-overlap case.  Generated on top of psycross_gpu_trace.patch.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_active_disp_draw_buffer.patch" "_xeno_active_disp_draw_buffer"
# ClearImage must not mark CPU VRAM older than the last framebuffer capture:
# the capture was mirrored when taken, so a later overlapping MoveImage would
# splat stale pixels over newer CPU writes (the attract-return crossfade frame).
apply_psycross_patch "$ROOT/pc_port/patches/psycross_clear_keeps_vram_synced.patch" "_xeno_clear_keeps_vram_synced"
# Track which VRAM clips the GL backbuffer holds since the last present, so the
# field's port-only backdrop snapshot (func_800A476C) can tell when the
# backbuffer is a different buffer than the (0,0) rect it is about to save.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_rendered_clip_track.patch" "_xeno_rendered_clip_track"
# Without new geometry, the CPU mirror already owns the last completed frame;
# do not recapture retained presentation effects or host overlays.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_rendered_clip_pending_capture.patch" "_xeno_rendered_clip_pending_capture"
# A scene present shows the drawn buffer one vblank early; on the first scene
# after VRAM-only presents that buffer may never be displayed on the PSX (the
# attract-return fade-start frame).  Show the DISPENV area there instead.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_present_displayed_after_vram.patch" "_xeno_present_displayed_after_vram"
# Every speed / fast-forward change is logged with its source (F6, Backspace,
# F7, L3, R3, dev-menu, focus-lost); applied last, on top of the host keys.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_speed_source.patch" "_xeno_speed_source"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_log_append.patch" "_xeno_log_append"
# libetc VSync(n) for n >= 2 returned immediately, so the world map's
# VSync(2)-paced frame driver (retail 30 world frames/s) ran at the host's
# present rate.  Wait as retail does (SLUS Vsync 0x8004B54C).
apply_psycross_patch "$ROOT/pc_port/patches/psycross_vsync_retail_n.patch" "_xeno_vsync_retail_n"
# Reproduce existing vendor fixes absent from the pinned-source patch stack.
# These markers intentionally do not appear in source: reverse-apply checks
# verify each whole patch before skipping an already-fixed vendor tree.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_initialization_fail_fast.patch" "_xeno_init_fail_fast_reversecheck"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_cd_standby.patch" "_xeno_cd_standby_reversecheck"
apply_psycross_patch "$ROOT/pc_port/patches/psycross_gte_retail_word_stores.patch" "_xeno_gte_retail_word_stores_reversecheck"
# HD-2D enhancement lane hook points (pc_port/src/hd2d): HOOK 1 post-process
# at both present sites, HOOK 3 textured-primitive override, and the
# XENO_PRESENT_HASH_LOG per-present CRC. Weak symbols: with no hd2d
# definition linked, retail rendering is unchanged. Keep this after every
# other PsyCross patch; generated against the tree they produce.
apply_psycross_patch "$ROOT/pc_port/patches/psycross_hd2d_hooks.patch" "_xeno_hd2d_hooks"
# HD-2D lane PsyCross patches: every pc_port/patches/hd2d/*.patch, in
# byte-sorted order, after all patches above. A patch is skipped when its
# own sentinel -- the literal comment `/* _hd2d_<basename> */` (basename
# without .patch) in one of its added lines -- is already in the tree, or
# when it reverse-applies (already applied). Make each with diff -u against
# the tree the earlier patches produce.
if [ -d "$ROOT/pc_port/patches/hd2d" ]; then
    while IFS= read -r hd2d_patch; do
        hd2d_base="$(basename "$hd2d_patch" .patch)"
        if grep -RqsF --exclude-dir=.git -- "/* _hd2d_${hd2d_base} */" "$PSX"; then
            continue
        fi
        # The literal sentinel was checked above; this marker never matches,
        # so apply_psycross_patch falls through to its reverse-check/apply.
        apply_psycross_patch "$hd2d_patch" "_hd2d_sentinel_checked_above_$$_${RANDOM}"
    done < <(find "$ROOT/pc_port/patches/hd2d" -maxdepth 1 -type f -name '*.patch' | LC_ALL=C sort)
fi

echo "==> [1/5] Building PsyCross (libpsycross.a) via CMake"
# Drop a stale CMake cache generated under a different absolute path (e.g. from a
# different container mount) so it reconfigures cleanly in the current env.
if [ -f "$PSYX_BUILD/CMakeCache.txt" ] && \
   ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=$ROOT/pc_port" "$PSYX_BUILD/CMakeCache.txt"; then
    echo "    (removing stale CMake cache)"; rm -rf "$PSYX_BUILD"
fi
if [ -n "$TSAN_FLAGS" ]; then
    cmake -S pc_port -B "$PSYX_BUILD" -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_FLAGS="$TSAN_FLAGS $PREFIX_MAP" -DCMAKE_CXX_FLAGS="$TSAN_FLAGS $PREFIX_MAP" >/dev/null
else
    cmake -S pc_port -B "$PSYX_BUILD" -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_FLAGS="$PREFIX_MAP" -DCMAKE_CXX_FLAGS="$PREFIX_MAP" >/dev/null
fi
if ! cmake --build "$PSYX_BUILD" --target psycross -j"$(nproc)" >/dev/null; then
    echo "ERROR: PsyCross build failed; refusing to link a stale library." >&2
    exit 1
fi
PSYLIB="$(find "$PSYX_BUILD" -name 'libpsycross.a' 2>/dev/null | head -1)"
if [ -z "$PSYLIB" ] || [ ! -s "$PSYLIB" ]; then
    echo "ERROR: libpsycross.a was not built (the CMake step failed above)."
    echo "       Make sure you're in the distrobox with SDL2/OpenAL/OpenGL dev installed."
    exit 1
fi
echo "    libpsycross.a: $PSYLIB"

echo "==> [2/5] Compiling game translation units in port mode"
print_intentionally_excluded_game_tus
print_reference_only_game_tus
print_known_broken_game_tus

compiled=0; skipped=0; SKIPPED=""
GAME_OBJS=()
GAME_TU_OBJS=()
PORT_OVERRIDE_MANIFEST="pc_port/port_owned_overrides.txt"
if [ ! -f "$PORT_OVERRIDE_MANIFEST" ]; then
    echo "ERROR: missing port ownership manifest: $PORT_OVERRIDE_MANIFEST"
    exit 1
fi
PORT_OVERRIDE_SYMBOLS=()
declare -A PORT_OVERRIDE_REASON=()
declare -A PORT_OVERRIDE_EXIT=()
while IFS='|' read -r sym reason exit_condition; do
    if [[ ! "$sym" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
        echo "ERROR: invalid symbol in $PORT_OVERRIDE_MANIFEST: $sym"
        exit 1
    fi
    if [ -z "$reason" ] || [ -z "$exit_condition" ]; then
        echo "ERROR: manifest row needs reason and exit condition: $sym"
        exit 1
    fi
    PORT_OVERRIDE_SYMBOLS+=("$sym")
    PORT_OVERRIDE_REASON[$sym]="$reason"
    PORT_OVERRIDE_EXIT[$sym]="$exit_condition"
done < <(sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*$/d' "$PORT_OVERRIDE_MANIFEST")
if [ "${#PORT_OVERRIDE_SYMBOLS[@]}" -eq 0 ]; then
    echo "ERROR: empty port ownership manifest: $PORT_OVERRIDE_MANIFEST"
    exit 1
fi
declare -A PORT_OVERRIDE_SEEN=()
for sym in "${PORT_OVERRIDE_SYMBOLS[@]}"; do
    if [ "${PORT_OVERRIDE_SEEN[$sym]+set}" = set ]; then
        echo "ERROR: duplicate symbol in $PORT_OVERRIDE_MANIFEST: $sym"
        exit 1
    fi
    PORT_OVERRIDE_SEEN[$sym]=1
done
while IFS= read -r f; do
    # Reference-only TUs remain in the matching build but have a deliberate,
    # complete port-side runtime owner and never enter the native port link.
    if is_reference_only_game_tu "$f"; then
        # Do not leave a formerly-compiled object available for accidental
        # reuse after a TU changes ownership classification.
        rm -f "$OBJ/$(echo "$f" | tr '/' '_').o" \
              "$OUT/$(echo "$f" | tr '/' '_').err"
        continue
    fi

    # Explicit exclusions are deliberately absent from both port compilation
    # and link while their routing decision remains held.
    if is_intentionally_excluded_game_tu "$f"; then
        continue
    fi

    o="$OBJ/$(echo "$f" | tr '/' '_').o"
    compile_err="$OUT/$(echo "$f" | tr '/' '_').err"
    rm -f "$o" "$compile_err"
    if gcc -c "$f" $GFLAGS $INC -o "$o" 2>"$compile_err"; then
        if is_known_broken_game_tu "$f"; then
            echo "ERROR: allowlisted broken game TU now compiles: $f"
            echo "       It may not silently enter the link; classify it as intentionally excluded"
            echo "       or remove it from the broken list after a reviewed routing decision."
            exit 1
        fi
        GAME_OBJS+=("$o")
        GAME_TU_OBJS+=("$o")
        compiled=$((compiled+1))
    elif is_known_broken_game_tu "$f"; then
        skipped=$((skipped+1)); SKIPPED="$SKIPPED $f"
        echo "    WARNING: allowlisted broken game TU failed: $f"
        echo "      $(known_broken_tu_reason "$f")"
        sed 's/^/      | /' "$compile_err"
    else
        echo "ERROR: game TU compilation failed and is not explicitly allowlisted."
        echo "       TU: $f"
        sed 's/^/       | /' "$compile_err"
        echo "ERROR: aborting; fix the TU or add a reviewed temporary allowlist entry."
        exit 1
    fi
done < <(find src -name '*.c' | sort)
echo "    compiled=$compiled  skipped=$skipped"
if [ -n "$SKIPPED" ]; then
    echo "    WARNING: skipped only by the explicit temporary allowlist:$SKIPPED"
    print_known_broken_game_tus
fi

echo "==> [2b/5] Compiling port-only sources (PSX RAM emu, overrides/dispatch table)"
BATTLE_BRIDGE_ELFS=()
if [ -f build/out/slus_006.64.elf ]; then
    BATTLE_BRIDGE_ELFS+=(--elf build/out/slus_006.64.elf)
fi
# The port binary from the previous build is evidence for classifying the retail
# library range (heap/CD/SPU/GTE): a name it defines as FUNC is bindable, so a
# retail jal to that address is a function call even though the matching ELF has
# no symbol for that (un-decompiled) code. Absent on the first clean build.
BATTLE_BRIDGE_HOST_ELFS=()
if [ -f "$OUT/xeno-port" ]; then
    BATTLE_BRIDGE_HOST_ELFS+=(--host-elf "$OUT/xeno-port")
fi
python3 tools/scripts/gen_battle_bridge_map.py \
    "${BATTLE_BRIDGE_ELFS[@]}" \
    "${BATTLE_BRIDGE_HOST_ELFS[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --out "$OUT/battle_bridge_map.inc"
PORT_SOURCES=(
    pc_port/src/krom_rom.c
    pc_port/src/disc_check.c
    pc_port/src/glibc_compat.c
    pc_port/src/plat/xg_plat_psycross.c
    pc_port/src/plat/xg_plat_config.c
    pc_port/src/plat/xg_plat_mods.c
    pc_port/src/mod_events.c
    pc_port/src/cheat_console.c
    pc_port/src/port_log.c
    pc_port/src/port_soft_reset.c
    pc_port/src/game_error_report.c
    pc_port/src/port_dev_menu.c
    pc_port/src/port_dev_menu_actions.c
    pc_port/src/port_dev_menu_warps.c
    pc_port/src/port_pad_confirm.c
    pc_port/src/port_dev_menu_steps.c
    pc_port/src/krom_mapping.c
    pc_port/src/psx_memory.c
    pc_port/src/battle_mips_adapter.c
    pc_port/src/battle_mips_runtime.c
    pc_port/src/battle_results_host.c
    pc_port/src/battle_guest_trampolines.c
    pc_port/src/battle_state_hash.c # TEST TOOLING: opt-in battle-frame input/hash recorder
    pc_port/src/battle_results_trace.c # TEST TOOLING: opt-in entry/render/return snapshots
    pc_port/src/guest_prim_link.c
    pc_port/src/model_prim_link.c
    pc_port/src/model_prim_ed20.c
    pc_port/src/test_input.c
    pc_port/src/quick_checkpoint_file.c
    pc_port/src/quick_checkpoint.c
    pc_port/src/fei_hd2d.c
    pc_port/src/god_mode.c
    pc_port/src/file_menu_notice.c
    pc_port/src/battling_unavailable.c # TEMPORARY STOPGAP: unported state 4 returns to field
    pc_port/src/memcard_port.c
    pc_port/src/route_recorder.c  # TEST TOOLING: XENO_ROUTE_LOG progression recorder
    pc_port/src/field_pos_diag.c  # TEST TOOLING: XENO_FIELD_POS_DIAG walk telemetry
    pc_port/src/field_warp_diag.c # TEST TOOLING: XENO_FIELD_WARP one-shot debug teleport
    pc_port/src/debug_battle_warp.c # TEST TOOLING: XENO_BATTLE_WARP_FILE battle launcher
    pc_port/src/walkmesh_dump.c   # TEST TOOLING: XENO_WALKMESH_DUMP per-map walkmesh dump
    pc_port/src/game_overrides.c
    pc_port/src/retail_leaf_adapters.c
    pc_port/src/boot_menu.c
    pc_port/src/boot_str.c
    pc_port/src/boot_assets.c
    pc_port/src/boot_sound_banks.c
    pc_port/src/boot_sound_commit.c
    pc_port/src/movie_player.c
    pc_port/src/pc_file_io.c
    pc_port/src/controller_vblank_service.c
    pc_port/src/field_object_overlay.c
    pc_port/src/field_party_gear.c
    pc_port/src/field_timed_commands.c
    pc_port/src/field_clip_prelude.c
    pc_port/src/field_clip_control.c
    pc_port/src/field_clip_data.c
    pc_port/src/sprite_constructor.c
    pc_port/src/world_map_init.c
    pc_port/src/world_map_frame_driver.c
    pc_port/src/world_map_image_transfer_25044.c
    pc_port/src/world_map_upload_pump_74f2c.c
    pc_port/src/world_map_upload_pump_75104.c
    pc_port/src/world_map_frame_tail_71984.c
    pc_port/src/world_map_ot_adapter.c
    pc_port/src/world_map_frame_driver_712d0.c
    pc_port/src/world_map_pause.c
    pc_port/src/world_map_main_loop_71034.c
    pc_port/src/world_map_terminal_default_71264.c
    pc_port/src/world_map_terminal_one_711b0.c
    pc_port/src/world_map_terminal_zero_710e4.c
    pc_port/src/world_map_teardown_7299c.c
    pc_port/src/world_map_mode811_lifecycle.c
    pc_port/src/world_map_mode9_lifecycle.c
    pc_port/src/world_map_mode10_lifecycle.c
    pc_port/src/world_map_mode12_lifecycle.c
    pc_port/src/world_map_mode13_lifecycle.c
    pc_port/src/world_map_mode14_lifecycle.c
    pc_port/src/world_map_mode15_lifecycle.c
    pc_port/src/world_map_mode16_lifecycle.c
    pc_port/src/world_map_mode17_lifecycle.c
    pc_port/src/world_map_mode18_lifecycle.c
    pc_port/src/world_map_capture.c
    pc_port/src/world_map_gamestate_alias.c
    pc_port/src/world_map_selector.c
    pc_port/src/psyq_cd_mix.c
    pc_port/src/psyq_compat.c
    pc_port/src/archive_port.c
    pc_port/src/work_list_port.c
    pc_port/src/sound_transfer_callbacks.c
    pc_port/src/data_published_logo.c
    pc_port/src/data_font.c
    pc_port/src/data_battle_popup_frame.c
    pc_port/src/data_kernel_menu.c
    pc_port/src/data_field.c
    pc_port/src/data_member_change_menu.c
    pc_port/src/data_shop_menu.c
    pc_port/src/data_name_entry_menu.c
    pc_port/src/data_main_menu.c
    pc_port/src/data_game_state.c
    pc_port/src/data_controller.c
    pc_port/src/data_heap.c
    pc_port/src/data_boot_globals.c
    pc_port/src/data_slus_sdata.c
    pc_port/src/data_slus_rodata.c
    pc_port/src/world_map_convergence.c
    pc_port/src/world_map_framebuffer_init.c
    pc_port/src/world_map_terrain_init.c
    pc_port/src/world_map_terrain_cell.c
    pc_port/src/world_map_plane_solver.c
    pc_port/src/world_map_terrain_normal.c
    pc_port/src/world_map_terrain_sampler.c
    pc_port/src/world_map_common_tail.c
    pc_port/src/world_map_scheduler.c
    pc_port/src/world_map_callback_923a8.c
    pc_port/src/world_map_callback_925a0.c
    pc_port/src/world_map_resume_initializers.c
    pc_port/src/world_map_callback_8a2c8.c
    pc_port/src/world_map_callback_8b2bc.c
    pc_port/src/world_map_callback_8bb40.c
    pc_port/src/world_map_callback_8c530.c
    pc_port/src/world_map_callback_8d3f0.c
    pc_port/src/world_map_callback_8dd6c.c
    pc_port/src/world_map_callback_8e190.c
    pc_port/src/world_map_callback_906e0.c
    pc_port/src/world_map_callback_91b54.c
    pc_port/src/world_map_callback_92234.c
    pc_port/src/world_map_state_vertical_8f690.c
    pc_port/src/world_map_callback_92be4.c
    pc_port/src/world_map_callback_92df8.c
    pc_port/src/world_map_callback_87734.c
    pc_port/src/world_map_callback_7756c.c
    pc_port/src/world_map_callback_76a14.c
    pc_port/src/world_map_callback_78948.c
    pc_port/src/world_map_callback_77dc8.c
    pc_port/src/world_map_callback_78e2c.c
    pc_port/src/world_map_callback_794d8.c
    pc_port/src/world_map_callback_795e4.c
    pc_port/src/world_map_callback_7a144.c
    pc_port/src/world_map_callback_7a9b4.c
    pc_port/src/world_map_callback_7ad34.c
    pc_port/src/world_map_callback_7b200.c
    pc_port/src/world_map_callback_7b604.c
    pc_port/src/world_map_callback_7ba08.c
    pc_port/src/world_map_callback_7bb60.c
    pc_port/src/world_map_callback_7c36c.c
    pc_port/src/world_map_callback_7c724.c
    pc_port/src/world_map_callback_7de14.c
    pc_port/src/world_map_callback_7e450.c
    pc_port/src/world_map_callback_7eca4.c
    pc_port/src/world_map_callback_7f8ac.c
    pc_port/src/world_map_callback_7fc8c.c
    pc_port/src/world_map_callback_8032c.c
    pc_port/src/world_map_callback_80578.c
    pc_port/src/world_map_callback_80a28.c
    pc_port/src/world_map_callback_81174.c
    pc_port/src/world_map_callback_813e8.c
    pc_port/src/world_map_callback_817a0.c
    pc_port/src/world_map_callback_819c8.c
    pc_port/src/world_map_callback_81c3c.c
    pc_port/src/world_map_callback_81fb4.c
    pc_port/src/world_map_callback_827c8.c
    pc_port/src/world_map_callback_827ec.c
    pc_port/src/world_map_callback_83214.c
    pc_port/src/world_map_callback_834d0.c
    pc_port/src/world_map_callback_88720.c
    pc_port/src/world_map_callback_838e8.c
    pc_port/src/world_map_callback_83a00.c
    pc_port/src/world_map_callback_84068.c
    pc_port/src/world_map_cold_defaults.c
    pc_port/src/world_map_mode_selector_73300.c
    pc_port/src/psyq_compat_normal_light_col.c
    pc_port/src/psyq_compat_transpose_matrix.c
    pc_port/src/psyq_compat_spu_noise_clock.c
    pc_port/src/world_map_callback_7cc6c.c
    pc_port/src/world_map_callback_7ce84.c
    pc_port/src/world_map_callback_7d078.c
    pc_port/src/world_map_callback_7d228.c
    pc_port/src/world_map_callback_7d414.c
    pc_port/src/world_map_callback_7d774.c
    pc_port/src/world_map_callback_8a72c.c
    pc_port/src/world_map_callback_8b644.c
    pc_port/src/world_map_callback_8c844.c
    pc_port/src/world_map_callback_8d678.c
    pc_port/src/world_map_callback_8e76c.c
    pc_port/src/world_map_state01_8eb30.c
    pc_port/src/world_map_state2_8eb64.c
    pc_port/src/world_map_vehicle_tail_90620.c
    pc_port/src/world_map_callback_914d0.c
    pc_port/src/world_map_callback_91c18.c
    pc_port/src/world_map_callback_922ac.c
    pc_port/src/world_map_callback_92c70.c
    pc_port/src/world_map_callback_92fd8.c
    pc_port/src/world_map_r4world_71a58.c
    pc_port/src/world_map_helper_8bfd4.c
    pc_port/src/world_map_helper_8e078.c
    pc_port/src/world_map_helper_8e0f0.c
    pc_port/src/world_map_helper_90c68.c
    pc_port/src/world_map_helper_90e14.c
    pc_port/src/world_map_helper_907f4.c
    pc_port/src/world_map_helper_90fb4.c
    pc_port/src/world_map_helper_91ff8.c
    pc_port/src/world_map_helper_3101c.c
    pc_port/src/world_map_helper_93484.c
    pc_port/src/world_map_helper_96f18.c
    pc_port/src/world_map_helper_93a5c.c
    pc_port/src/world_map_helper_95cd4.c
    pc_port/src/world_map_helper_9623c.c
    pc_port/src/world_map_helper_983a0.c
    pc_port/src/world_map_helper_987ac.c
    pc_port/src/world_map_helper_9980c.c
    pc_port/src/world_map_helper_96328.c
    pc_port/src/world_map_helper_97dc0.c
    pc_port/src/world_map_helper_980d4.c
    pc_port/src/world_map_helper_73398.c
    pc_port/src/world_map_helper_73448.c
    pc_port/src/world_map_helper_72db4.c
    pc_port/src/world_map_helper_76858.c
    pc_port/src/world_map_helper_76954.c
    pc_port/src/world_map_helper_97070.c
    pc_port/src/world_map_helper_94154.c
    pc_port/src/world_map_helper_7565c.c
    pc_port/src/world_map_helper_75d4c.c
    pc_port/src/world_map_helper_75e7c.c
    pc_port/src/world_map_helper_762fc.c
    pc_port/src/world_map_helper_86124.c
    pc_port/src/world_map_helper_71fec.c
    pc_port/src/world_map_menu_lifecycle.c
    pc_port/src/world_map_session_setup_72238.c
    pc_port/src/world_map_helper_96130.c
    pc_port/src/world_map_helper_966cc.c
    pc_port/src/world_map_helper_737ec.c
    pc_port/src/world_map_helper_747dc.c
    pc_port/src/world_map_helper_740b8.c
    pc_port/src/world_map_helper_97244.c
    pc_port/src/world_map_helper_965a4.c
    pc_port/src/world_map_helper_89580.c
    pc_port/src/world_map_helper_89748.c
    pc_port/src/world_map_helper_98cc0.c
    pc_port/src/world_map_helper_9932c.c
    pc_port/src/world_map_helper_99bfc.c
    pc_port/src/world_map_helper_99708.c
    pc_port/src/world_map_helper_981c8.c
    pc_port/src/world_map_helper_848b4.c
    pc_port/src/world_map_helper_848f4.c
    pc_port/src/world_map_helper_85cdc.c
    pc_port/src/world_map_helper_89c78.c
    pc_port/src/world_map_helper_8615c.c
    pc_port/src/world_map_helper_86798.c
    pc_port/src/world_map_helper_c28c.c
    pc_port/src/world_map_helper_c364.c
    pc_port/src/world_map_helper_8dff4.c
    pc_port/src/world_map_helper_7528c.c
    pc_port/src/world_map_helper_8e034.c
    pc_port/src/world_map_helper_73b04.c
    pc_port/src/world_map_helper_90a84.c
    pc_port/src/world_map_helper_74794.c
    pc_port/src/world_map_helper_85760.c
    pc_port/src/world_map_helper_85418.c
    pc_port/src/world_map_helper_93354.c
    pc_port/src/world_map_helper_952b0.c
    pc_port/src/world_map_helper_95324.c
    pc_port/src/world_map_helper_93fe4.c
    pc_port/src/world_map_helper_94088.c
    pc_port/src/world_map_helper_951a8.c
    pc_port/src/world_map_helper_85158.c
    pc_port/src/world_map_helper_84db8.c
    pc_port/src/world_map_helper_84d00.c
    pc_port/src/world_map_helper_97770.c
    pc_port/src/world_map_helper_941c4.c
    pc_port/src/world_map_helper_94238.c
    pc_port/src/world_map_helper_94364.c
    pc_port/src/world_map_helper_8bec8.c
    pc_port/src/world_map_helper_8c1dc.c
    pc_port/src/world_map_helper_93534.c
    pc_port/src/world_map_helper_93e8c.c
    pc_port/src/world_map_helper_93f18.c
    pc_port/src/world_map_helper_94060.c
    pc_port/src/world_map_helper_8c040.c
    pc_port/src/world_map_private_collision.c
    pc_port/src/world_map_func_94a5c.c
    pc_port/src/world_map_func_95414.c
)
# HD-2D enhancement lane: every pc_port/src/hd2d/*.c is a port source by
# glob (sorted), so that lane adds files without editing this list. Its
# objects are named hd2d__<basename>.o so they cannot collide with the
# flat pc_port/src basenames. pc_port/assets/hd2d/ is reserved for its assets.
while IFS= read -r hd2d_src; do
    PORT_SOURCES+=("$hd2d_src")
done < <(find pc_port/src/hd2d -maxdepth 1 -type f -name '*.c' 2>/dev/null | LC_ALL=C sort)

# Port-side sources that are deliberately NOT part of the runtime link: each is
# a verification fixture that its own pc_port/tests/run_*.sh compiles directly,
# and no runtime port source includes its header.  Verified with
#   grep -rln '#include "battle_target' pc_port --include=*.c --include=*.h
# which lists only these files and pc_port/tests/*.c.  They are registered here
# rather than added to PORT_SOURCES so the link keeps exactly the symbols it
# had before they were written; dropping them from the tree instead would take
# the differential tests with them.
TEST_ONLY_PORT_SOURCES=(
    pc_port/src/battle_target_bounds.c
    pc_port/src/battle_target_camera.c
    pc_port/src/battle_target_eligibility_ram.c
    pc_port/src/battle_target_list_ram.c
    pc_port/src/battle_target_setup.c
)

test_only_port_source_reason() {
    case "$1" in
        pc_port/src/battle_target_bounds.c)
            echo "target-bounds oracle for run_battle_target_{bounds,radius,camera}_test.sh" ;;
        pc_port/src/battle_target_camera.c)
            echo "camera-projection oracle for run_battle_target_camera_test.sh" ;;
        pc_port/src/battle_target_eligibility_ram.c)
            echo "guest-RAM eligibility oracle for run_battle_target_{eligibility_ram,guest_frame}_test.sh" ;;
        pc_port/src/battle_target_list_ram.c)
            echo "guest-RAM target-list oracle for run_battle_target_guest_frame_test.sh" ;;
        pc_port/src/battle_target_setup.c)
            echo "setup/cleanup contract oracle for run_battle_target_setup_*.sh and run_battle_target_cleanup_retail_test.sh" ;;
    esac
}

is_test_only_port_source() {
    local candidate="$1"
    local entry
    for entry in "${TEST_ONLY_PORT_SOURCES[@]}"; do
        [ "$candidate" = "$entry" ] && return 0
    done
    return 1
}

print_test_only_port_sources() {
    echo "    Test-only port sources (compiled by their own tests, never linked):"
    local entry
    for entry in "${TEST_ONLY_PORT_SOURCES[@]}"; do
        echo "      $entry — $(test_only_port_source_reason "$entry")"
    done
}

# The lists above are the port link's explicit ownership registry. Refuse to
# silently ignore a new port-side TU: every C source under pc_port/src must be
# the separately-built entry point, present in PORT_SOURCES, or registered as
# test-only above.
while IFS= read -r pf; do
    [ "$pf" = "pc_port/src/port_main.c" ] && continue
    if is_test_only_port_source "$pf"; then
        continue
    fi
    listed=0
    for listed_pf in "${PORT_SOURCES[@]}"; do
        if [ "$pf" = "$listed_pf" ]; then
            listed=1
            break
        fi
    done
    if [ "$listed" -eq 0 ]; then
        echo "ERROR: unclassified port source is not in PORT_SOURCES: $pf"
        echo "ERROR: if it is a verification fixture, register it in"
        echo "ERROR: TEST_ONLY_PORT_SOURCES with a reason instead."
        echo "ERROR: aborting before trial link and stub generation."
        exit 1
    fi
done < <(find pc_port/src -type f -name '*.c' | sort)

# A test-only source must not also be linked, and must actually exist.
for pf in "${TEST_ONLY_PORT_SOURCES[@]}"; do
    if [ ! -f "$pf" ]; then
        echo "ERROR: test-only port source does not exist: $pf"
        exit 1
    fi
    for listed_pf in "${PORT_SOURCES[@]}"; do
        if [ "$pf" = "$listed_pf" ]; then
            echo "ERROR: $pf is registered both test-only and in PORT_SOURCES."
            exit 1
        fi
    done
done

print_test_only_port_sources

for pf in "${PORT_SOURCES[@]}"; do
    if [ ! -f "$pf" ]; then
        echo "ERROR: listed port source does not exist: $pf"
        echo "ERROR: aborting before trial link and stub generation."
        exit 1
    fi
    o="$OBJ/$(basename "$pf").o"
    case "$pf" in pc_port/src/hd2d/*) o="$OBJ/hd2d__$(basename "$pf").o" ;; esac
    compile_err="$OUT/$(echo "$pf" | tr '/' '_').err"
    rm -f "$o" "$compile_err"
    if ! gcc -c "$pf" $GFLAGS -Ipc_port/src $INC -o "$o" 2>"$compile_err"; then
        echo "ERROR: port source compilation failed."
        echo "       TU: $pf"
        sed 's/^/       | /' "$compile_err"
        echo "ERROR: aborting before trial link and stub generation; port sources may not be stubbed."
        exit 1
    fi
    GAME_OBJS+=("$o")
    echo "    $(basename "$pf") ok"
done

echo "==> [2c/5] Compiling adopted battle overlay host bodies"
# src/battle TUs stay reference-only for the interpreter's benefit, but the
# functions listed in pc_port/src/battle_overlay_host_leaves.inc are allowed to
# run as native C instead of interpreted retail bytes. That allowlist is only
# real if the bodies are in the link: runtime_bridge_call() resolves overlay
# targets with dlsym(RTLD_DEFAULT, "func_XXXXXXXX"), so an uncompiled allowlist
# entry silently falls through to the interpreter and the port claims native
# coverage it does not have. Derive the TU set from the allowlist itself so the
# two can never drift, and fail the build rather than ship an inert allowlist.
BATTLE_HOST_TUS="$(python3 tools/scripts/battle_overlay_host_tus.py --verify)" || {
    echo "ERROR: could not derive the battle host TU list from the allowlist."
    exit 1
}
# Typed bridge thunks (pc_port/src/battle_guest_abi_thunks.inc): every
# guest-ABI leaf must declare its argument/return kinds, and each declaration
# must agree with the C prototype, or the bridge would hand it legacy
# heuristically translated arguments.
python3 tools/scripts/battle_overlay_host_tus.py --check-abi-thunks || {
    echo "ERROR: typed bridge thunks disagree with the battle guest-ABI leaves."
    exit 1
}
BATTLE_HOST_OBJS=()
# TUs on the battle guest ABI (include/battle_guest_abi.h) drop the blanket
# `-w -fpermissive` and compile with strict int/pointer errors instead.
BATTLE_ABI_TUS=" $(python3 tools/scripts/battle_overlay_host_tus.py --abi-tus | tr '\n' ' ') "
BATTLE_ABI_FLAGS="$(python3 tools/scripts/battle_overlay_host_tus.py --abi-strict-flags)"
BATTLE_ABI_GFLAGS="$(echo " $GFLAGS " | sed 's/ -w / /; s/ -fpermissive / /')"
for btu in $BATTLE_HOST_TUS; do
    bo="$OBJ/battle_host_$(basename "$btu").o"
    berr="$OUT/$(echo "$btu" | tr '/' '_').host.err"
    rm -f "$bo" "$berr"
    btu_flags="$GFLAGS"
    case "$BATTLE_ABI_TUS" in
        *" $btu "*) btu_flags="$BATTLE_ABI_GFLAGS $BATTLE_ABI_FLAGS" ;;
    esac
    # XENO_BATTLE_OVERLAY_HOST_BODIES is what activates the guest-RAM D_*
    # aliases (see include/common.h); it must not be set for any other TU.
    if ! gcc -c "$btu" $btu_flags -DXENO_BATTLE_OVERLAY_HOST_BODIES $INC \
        -o "$bo" 2>"$berr"; then
        echo "ERROR: adopted battle overlay host TU failed to compile: $btu"
        sed 's/^/       | /' "$berr"
        echo "ERROR: an adopted leaf may not enter the link unbuilt."
        exit 1
    fi
    GAME_OBJS+=("$bo")
    BATTLE_HOST_OBJS+=("$bo")
    echo "    $(basename "$btu") ok"
done
if [ "${#BATTLE_HOST_OBJS[@]}" -eq 0 ]; then
    echo "ERROR: the overlay host allowlist produced no translation units."
    exit 1
fi
for leaf in $(python3 tools/scripts/battle_overlay_host_tus.py --leaves); do
    if ! nm -g --defined-only "${BATTLE_HOST_OBJS[@]}" 2>/dev/null \
        | awk -v s="$leaf" '$3 == s {found=1} END {exit !found}'; then
        echo "ERROR: adopted leaf $leaf is defined by no battle host object."
        echo "ERROR: the allowlist would be inert for it; fix the TU mapping."
        exit 1
    fi
done
echo "    ${#BATTLE_HOST_OBJS[@]} battle host units define the full allowlist"

# The battle RESULTS module (retail overlay at 0x801DE000) has its own
# allowlist, pc_port/src/battle_results_host_leaves.inc, and its own host TU,
# pc_port/src/battle_results_host.c (in PORT_SOURCES above).  The runtime binds
# each entry with dlsym("battle_results_func_XXXXXXXX") behind the module
# identity gate (battle_results_gate.inc); an entry with no compiled body would
# silently stay interpreted while the list claims native coverage.  An empty
# list is valid: then every results routine is interpreted, as before.
RESULTS_HOST_OBJECT="$OBJ/battle_results_host.c.o"
if ! python3 tools/scripts/battle_results_host_leaves.py --verify >/dev/null; then
    echo "ERROR: pc_port/src/battle_results_host_leaves.inc is malformed."
    exit 1
fi
BATTLE_RESULTS_SYMBOLS="$(python3 tools/scripts/battle_results_host_leaves.py --symbols)"
results_adopted=0
for sym in $BATTLE_RESULTS_SYMBOLS; do
    if ! nm -g --defined-only "$RESULTS_HOST_OBJECT" 2>/dev/null \
        | awk -v s="$sym" '$2 == "T" && $3 == s {found=1} END {exit !found}'; then
        echo "ERROR: adopted results routine $sym is not defined by $RESULTS_HOST_OBJECT."
        echo "ERROR: the results allowlist would be inert for it."
        exit 1
    fi
    results_adopted=$((results_adopted+1))
done
echo "    ${results_adopted} adopted battle results bodies defined (rest of the results module interpreted)"

# Every overlay loads at the same vram base, so a battle function and a field
# function can share an address -- and therefore a splat symbol name. src/battle
# /main2.c defines func_800764B4 and so does src/field/main/misc2.c. Two strong
# definitions cannot both be linked.
#
# A non-adopted body in a compiled TU is unreachable: the interpreter only ever
# enters an overlay target that is on the allowlist, so weakening one of those
# changes nothing that runs. Anything an adopted leaf can reach is a different
# matter -- redirecting that would silently change what the leaf calls -- so a
# collision on those names is fatal and has to be resolved in the source.
for o in "${GAME_OBJS[@]}"; do
    case " ${BATTLE_HOST_OBJS[*]} " in *" $o "*) continue ;; esac
    nm -g --defined-only "$o" 2>/dev/null | awk '{print $3}'
done | sort -u > "$OUT/port_defined.txt"
# Weakening the battle copy was wrong whenever that battle function is itself
# an adopted leaf (battle_overlay_host_leaves.inc): the bridge's
# dlsym("func_<addr>") then returned the OTHER overlay's strong body and ran
# e.g. world-map code inside a battle (func_800879A8: SIGSEGV on the first
# on-foot Attack).  Give each colliding battle definition a battle_ prefix in
# EVERY battle host object instead, so battle-internal native calls bind to the
# battle copy, other overlays keep theirs, and battle_mips_runtime.c looks up
# battle_func_<addr> first for overlay targets.
collisions=()
for bo in "${BATTLE_HOST_OBJS[@]}"; do
    while read -r sym; do
        [ -z "$sym" ] && continue
        grep -qx "$sym" "$OUT/port_defined.txt" || continue
        collisions+=("$sym")
    done < <(nm -g --defined-only "$bo" 2>/dev/null | awk '$2 ~ /^[TDBW]$/ {print $3}')
done
renamed=0
if [ "${#collisions[@]}" -gt 0 ]; then
    redefs=()
    for sym in $(printf '%s\n' "${collisions[@]}" | sort -u); do
        redefs+=(--redefine-sym "$sym=battle_$sym")
        renamed=$((renamed+1))
        echo "    overlay name collision: battle copy of $sym linked as battle_$sym"
    done
    for bo in "${BATTLE_HOST_OBJS[@]}"; do
        objcopy "${redefs[@]}" "$bo"
    done
fi
echo "    ${renamed} overlay name collision(s) resolved by battle_ renaming"
# A battle host object can also CALL a battle-range function that no battle
# host object defines.  If another overlay's game TU defines that name, the
# call would silently bind to the other overlay's body (battle callers of
# func_800877E0 ran the world-map function).  Rename those references to
# battle_<name>: pc_port/src/battle_guest_trampolines.c defines them, and a
# missing one fails the link instead of running foreign code.
misbinds="$(python3 - "$OUT" "${BATTLE_HOST_OBJS[@]}" -- "${GAME_OBJS[@]}" <<'PY'
import os, re, subprocess, sys
args = sys.argv[2:]
split = args.index("--")
battle, game = args[:split], args[split + 1:]
def syms(obj, undefined):
    flag = ["-u"] if undefined else ["-g", "--defined-only"]
    out = subprocess.run(["nm", *flag, obj], capture_output=True, text=True).stdout
    return {line.split()[-1] for line in out.splitlines() if line.strip()}
battle_defined = set().union(*(syms(o, False) for o in battle)) if battle else set()
battle_set = set(battle)
other_defined = set()
for o in game:
    base = os.path.basename(o)
    if o in battle_set or not base.startswith("src_") or base.startswith("src_battle_"):
        continue
    other_defined |= syms(o, False)
def overlay(name):
    m = re.fullmatch(r"func_([0-9A-F]{8})", name)
    return bool(m) and 0x8006FAF0 <= int(m.group(1), 16) < 0x800E0000
refs = set().union(*(syms(o, True) for o in battle)) if battle else set()
print(" ".join(sorted(n for n in refs if overlay(n) and n in other_defined and n not in battle_defined)))
PY
)" || { echo "ERROR: battle cross-overlay reference scan failed."; exit 1; }
if [ -n "$misbinds" ]; then
    redefs=()
    for sym in $misbinds; do
        redefs+=(--redefine-sym "$sym=battle_$sym")
        echo "    battle reference to $sym (defined only by another overlay) linked as battle_$sym"
    done
    for bo in "${BATTLE_HOST_OBJS[@]}"; do
        objcopy "${redefs[@]}" "$bo"
    done
fi

# Port fallbacks are deliberately strong. If a matching game TU later gains
# one of these retail definitions, weaken that duplicate in the game object
# before linking. Relocations retain the original symbol name, so every caller
# binds uniformly to the strong port owner; matching sources stay retail-shaped.
PORT_OVERRIDE_OBJECT="$OBJ/game_overrides.c.o"
if [ ! -f "$PORT_OVERRIDE_OBJECT" ]; then
    echo "ERROR: port ownership object was not built: $PORT_OVERRIDE_OBJECT"
    exit 1
fi
for sym in "${PORT_OVERRIDE_SYMBOLS[@]}"; do
    if ! nm -g --defined-only "$PORT_OVERRIDE_OBJECT" 2>/dev/null \
        | awk -v sym="$sym" '$3 == sym {found=1} END {exit !found}'; then
        echo "ERROR: port ownership manifest symbol is not defined by $PORT_OVERRIDE_OBJECT: $sym"
        exit 1
    fi
    matched_objects=0
    for o in "${GAME_TU_OBJS[@]}"; do
        if nm -g --defined-only "$o" 2>/dev/null \
            | awk -v sym="$sym" '$3 == sym {found=1} END {exit !found}'; then
            matched_objects=$((matched_objects+1))
            echo "    port-owned override: weakening matching definition $sym in $(basename "$o")"
            if ! objcopy --weaken-symbol="$sym" "$o"; then
                echo "ERROR: failed to weaken duplicate matching definition: $sym"
                exit 1
            fi
        fi
    done
    if [ "$matched_objects" -eq 0 ]; then
        symbol_sources="$(rg -l --glob '*.c' "\\b${sym}\\s*\\(" src 2>/dev/null || true)"
        excluded_sources=""
        broken_sources=""
        reference_sources=""
        while IFS= read -r candidate; do
            [ -z "$candidate" ] && continue
            if is_intentionally_excluded_game_tu "$candidate"; then
                excluded_sources="$excluded_sources $candidate"
            elif is_known_broken_game_tu "$candidate"; then
                broken_sources="$broken_sources $candidate"
            elif is_reference_only_game_tu "$candidate"; then
                reference_sources="$reference_sources $candidate"
            fi
        done <<< "$symbol_sources"
        if [ -n "$excluded_sources" ]; then
            echo "ERROR: port ownership row has no compiled definition because the matching TU is intentionally excluded: $sym"
            echo "       Sources:$excluded_sources"
        elif [ -n "$broken_sources" ]; then
            echo "ERROR: port ownership row has no compiled definition because the matching TU is allowlisted broken: $sym"
            echo "       Sources:$broken_sources"
        elif [ -n "$reference_sources" ]; then
            echo "ERROR: port ownership row has no compiled definition because the matching TU is reference-only: $sym"
            echo "       Sources:$reference_sources"
        else
            echo "ERROR: stale port ownership row has no matching game definition: $sym"
        fi
        echo "       Retire the row and its fallback together, or restore the matching TU."
        exit 1
    fi
    for o in "${GAME_TU_OBJS[@]}"; do
        if nm -g --defined-only "$o" 2>/dev/null \
            | awk -v sym="$sym" '$3 == sym && $2 !~ /^[Ww]$/ {found=1} END {exit !found}'; then
            echo "ERROR: matching object still has a strong retired override symbol: $sym"
            exit 1
        fi
    done
done
echo "    verified ${#PORT_OVERRIDE_SYMBOLS[@]} port-owned override symbols with retirement metadata"

echo "==> [3/5] Compiling port entry point"
PORT_MAIN_SOURCE="pc_port/src/port_main.c"
PORT_MAIN_OBJECT="$OBJ/port_main.o"
PORT_MAIN_ERR="$OUT/pc_port_src_port_main.c.err"
rm -f "$PORT_MAIN_OBJECT" "$PORT_MAIN_ERR"
if ! gcc -c "$PORT_MAIN_SOURCE" $GFLAGS -Ipc_port/src -Iinclude \
    -I"$PSX/include" -I"$PSX/include/psx" -o "$PORT_MAIN_OBJECT" \
    2>"$PORT_MAIN_ERR"; then
    echo "ERROR: port entry-point compilation failed."
    echo "       TU: $PORT_MAIN_SOURCE"
    sed 's/^/       | /' "$PORT_MAIN_ERR"
    echo "ERROR: aborting before trial link and stub generation; a stale port_main.o will not be reused."
    exit 1
fi

LIBS="$(pkg-config --libs sdl2 openal libcrypto 2>/dev/null) -lGL -lm -lpthread -ldl"
if [ -n "$TSAN_FLAGS" ]; then
    # Sanitizer EH instrumentation of the C++ PsyCross objects references the
    # C++ personality routine (__gxx_personality_v0); the normal build does
    # not need libstdc++ at all.
    LIBS="$LIBS -lstdc++"
fi
# -no-pie: link non-PIE so the executable loads at a fixed low base and ALL of
# its BSS (the emulated PSX RAM g_PsxRam[] plus every auto-stubbed data symbol)
# lives below 4 GiB. The decompiled game truncates its own pointers to 32 bits
# all over (e.g. the heap's `(u32)pHeapStart & -4`); keeping that memory in the
# low 32-bit address space makes every such truncation a lossless round-trip.
NOPIE="-no-pie -fno-pie -rdynamic"
# xg_plat routing: the game's PSX SDK calls listed here go through the
# xg_plat interfaces (src/plat/xg_plat_psycross.c: __wrap_* -> interface ->
# PsyCross __real_*); game C is unchanged.
XG_PLAT_WRAP_SYMS="LoadImage SpuWrite"
XG_PLAT_WRAP_SYMS+=" DrawOTag PutDrawEnv PutDispEnv ClearOTag ClearOTagR"   # renderer
XG_PLAT_WRAP_SYMS+=" CdRead CdControl CdControlB CdControlF"                # disc
XG_PLAT_WRAP_SYMS+=" PsyX_Pad_InitPad PsyX_UpdateInput"                     # input (pad buffers)
XG_PLAT_WRAP_SYMS+=" VSync SetRCnt GetRCnt StartRCnt"                        # timing
XG_PLAT_WRAP_SYMS+=" PsyX_Sys_DispatchCounter2"                              # sound tick clock
XG_PLAT_WRAP_SYMS+=" acosf asinf sqrtf"   # pc_port/src/glibc_compat.c: pre-2.43 libm versions
XG_PLAT_WRAPS=""
for _sym in $XG_PLAT_WRAP_SYMS; do XG_PLAT_WRAPS+=" -Wl,--wrap=$_sym"; done
if [ -n "$COV_OBJ" ]; then
    gcc -c pc_port/tools/coverage_recorder.c -O2 -g $PREFIX_MAP -o "$COV_OBJ"
fi
LINK=(gcc -m64 $NOPIE $TSAN_FLAGS "$PORT_MAIN_OBJECT" "${GAME_OBJS[@]}" ${COV_OBJ:+"$COV_OBJ"} "$PSYLIB" $LIBS $XG_PLAT_WRAPS -o "$OUT/xeno-port")

echo "==> [4/5] Trial link to discover undefined references"
"${LINK[@]}" 2> "$OUT/link1.err"
grep -oE "undefined reference to \`[A-Za-z0-9_]+'" "$OUT/link1.err" \
    | sed -E "s/.*\`([A-Za-z0-9_]+)'/\1/" | sort -u > "$OUT/undef.txt"
echo "    undefined symbols to stub: $(wc -l < "$OUT/undef.txt")"

if [ -s "$OUT/undef.txt" ]; then
    ELFS=()
    for e in build/out/slus_006.64.elf build/out/field.elf build/out/member_change_menu.elf \
             build/out/shop_menu.elf build/out/name_entry_menu.elf build/out/menu.elf build/out/battle.elf; do
        [ -f "$e" ] && ELFS+=(--elf "$e")
    done
    MAPS=()
    for m in build/out/slus_006.64.map build/out/field.map build/out/member_change_menu.map \
             build/out/shop_menu.map build/out/name_entry_menu.map build/out/menu.map build/out/battle.map; do
        [ -f "$m" ] && MAPS+=(--map "$m")
    done
    # symbol_addrs files carry the real struct sizes (size:) the ELF omits, so data
    # stubs (e.g. g_GameState = 0x2300) are reserved at full size instead of 16 bytes.
    # port_buffers.txt is port-only: it sizes retail regions the port READS INTO at
    # runtime (boot archive buffers), which the matching configs do not annotate.
    SYMS=()
    for s in config/symbol_addrs.slus_006.64.txt config/symbol_addrs.field.txt \
             config/symbol_addrs.member_change_menu.txt config/symbol_addrs.shop_menu.txt \
             config/symbol_addrs.name_entry_menu.txt config/symbol_addrs.menu.txt config/symbol_addrs.battle.txt \
             config/symbol_addrs.port_buffers.txt; do
        [ -f "$s" ] && SYMS+=(--symbol-addrs "$s")
    done
    if [ "${#ELFS[@]}" -gt 0 ] || [ "${#MAPS[@]}" -gt 0 ]; then
        if ! python3 tools/scripts/gen_port_stubs.py "${ELFS[@]}" "${MAPS[@]}" "${SYMS[@]}" --undefined "$OUT/undef.txt" --out "$OUT/stubs.c"; then
            echo "ERROR: stub classification failed; refusing to compile or link stale stubs."
            exit 1
        fi
    elif [ -f "$OUT/stubs.c" ]; then
        # Without matching ELFs the generator cannot safely classify a newly
        # undefined symbol as function vs data, nor size new data storage.
        # An existing stubs.c is usable only as an exact typed manifest for
        # this undef set; silently reusing it for a different set would make
        # the build link against stale or wrongly-shaped placeholders.
        echo "    matching ELFs missing; validating existing stubs.c against current undef.txt"
        if ! python3 - "$OUT/undef.txt" "$OUT/stubs.c" <<'PY'
import re
import sys

undef_path, stubs_path = sys.argv[1:]
with open(undef_path) as f:
    undefined = set(f.read().split())

# xeno_port_stub is defined by stubs.c itself and intentionally appears during
# the trial link before stubs.o is linked.
undefined.discard("xeno_port_stub")
with open(stubs_path) as f:
    source = f.read()

stubbed = set(re.findall(r"^unsigned char\s+([A-Za-z_]\w*)\[", source, re.M))
stubbed.update(re.findall(r"^long\s+([A-Za-z_]\w*)\(void\)", source, re.M))
missing = sorted(undefined - stubbed)
stale = sorted(stubbed - undefined)
if missing or stale:
    print("ERROR: existing stubs.c does not exactly match current undef.txt.", file=sys.stderr)
    if missing:
        print("  missing typed stubs: " + ", ".join(missing[:16]) +
              (" ..." if len(missing) > 16 else ""), file=sys.stderr)
    if stale:
        print("  stale stubs: " + ", ".join(stale[:16]) +
              (" ..." if len(stale) > 16 else ""), file=sys.stderr)
    print("  Matching ELFs are required to regenerate a safe typed stubs.c.", file=sys.stderr)
    sys.exit(1)
print(f"    existing typed stub manifest matches {len(undefined)} current undefined symbols")
PY
        then
            echo "ERROR: matching ELFs are absent and stubs.c is stale."
            echo "       Provide matching ELFs to regenerate typed stubs; refusing to link."
            exit 1
        fi
    else
        echo "ERROR: matching ELFs missing and no typed $OUT/stubs.c manifest exists."
        echo "       Provide matching ELFs to generate safe function/data stubs."
        exit 1
    fi
    nm -g --defined-only "$OBJ/port_main.o" "${GAME_OBJS[@]}" 2>/dev/null \
        | awk '{print $3}' | sort -u > "$OUT/defined.txt"
    python3 - "$OUT/stubs.c" "$OUT/defined.txt" <<'PY'
import re
import sys

stubs_path, defined_path = sys.argv[1], sys.argv[2]
with open(defined_path) as f:
    defined = set(f.read().split())
with open(stubs_path) as f:
    lines = f.readlines()

stub_re = re.compile(r"^long\s+([A-Za-z_]\w*)\(void\)\s+\{")
out = []
removed = []
for line in lines:
    m = stub_re.match(line)
    if m and m.group(1) in defined:
        removed.append(m.group(1))
        continue
    out.append(line)

func_count = sum(1 for line in out if stub_re.match(line))
for i, line in enumerate(out):
    if re.match(r"/\* ---- \d+ function symbols ---- \*/", line):
        out[i] = f"/* ---- {func_count} function symbols ---- */\n"
        break

if removed:
    with open(stubs_path, "w") as f:
        f.writelines(out)
    print(f"    pruned stale function stubs: {', '.join(removed[:8])}"
          f"{'...' if len(removed) > 8 else ''}")
PY
    gcc -c "$OUT/stubs.c" -O0 -g $TSAN_FLAGS $PREFIX_MAP -o "$OBJ/stubs.o"
fi

echo "==> [5/5] Final link"
LINK_MAP="$OUT/xeno-port.map"
# Record the final link inputs and the port-source compile flags so a test
# can relink this exact build with one extra pc_port/src/hd2d-style object
# (pc_port/tests/run_hd2d_off_identity_test.sh) instead of rebuilding.
printf '%s\n' -m64 $NOPIE $TSAN_FLAGS "$PORT_MAIN_OBJECT" "${GAME_OBJS[@]}" "$OBJ/stubs.o" \
    ${COV_OBJ:+"$COV_OBJ"} "$PSYLIB" $LIBS $XG_PLAT_WRAPS > "$OUT/xeno-port.link-args"
printf '%s\n' "$GFLAGS -Ipc_port/src $INC" > "$OUT/port-source.cflags"
final_link() {
    gcc -m64 $NOPIE $TSAN_FLAGS "$PORT_MAIN_OBJECT" "${GAME_OBJS[@]}" "$OBJ/stubs.o" ${COV_OBJ:+"$COV_OBJ"} "$PSYLIB" $LIBS \
        $XG_PLAT_WRAPS -Wl,-Map="$LINK_MAP" -o "$OUT/xeno-port" 2> "$OUT/link2.err"
}
final_link
# Size the battle bridge map's data bindings from the binary just linked
# (gen_battle_bridge_map.py --size-from-host: min(guest extent, host object),
# plus an entry for each host-defined D_ object inside another binding's
# extent).  Only battle_mips_runtime.c includes the map; the link's undefined
# set does not depend on it (dlsym names).  Recompile that TU and relink when
# the map changes; a second pass over the relinked binary must reproduce it.
if [ -f "$OUT/xeno-port" ] && [ ! -s "$OUT/link2.err" ]; then
    for bridge_pass in 1 2 3; do
        python3 tools/scripts/gen_battle_bridge_map.py \
            "${BATTLE_BRIDGE_ELFS[@]}" --host-elf "$OUT/xeno-port" \
            --size-from-host "$OUT/xeno-port" \
            --symbols config/symbol_addrs.slus_006.64.txt \
            --symbols linker/undefined_funcs_auto.battle.txt \
            --symbols linker/undefined_syms_auto.battle.txt \
            --out "$OUT/battle_bridge_map.inc.sized"
        if cmp -s "$OUT/battle_bridge_map.inc.sized" "$OUT/battle_bridge_map.inc"; then
            rm -f "$OUT/battle_bridge_map.inc.sized"
            break
        fi
        if [ "$bridge_pass" = 3 ]; then
            echo "ERROR: the sized battle bridge map does not converge"
            exit 1
        fi
        mv "$OUT/battle_bridge_map.inc.sized" "$OUT/battle_bridge_map.inc"
        echo "    battle bridge map sized from the linked binary (pass $bridge_pass); relinking"
        bridge_err="$OUT/pc_port_src_battle_mips_runtime.c.err"
        if ! gcc -c pc_port/src/battle_mips_runtime.c $GFLAGS -Ipc_port/src $INC \
                -o "$OBJ/battle_mips_runtime.c.o" 2>"$bridge_err"; then
            echo "ERROR: port source compilation failed (sized bridge map)."
            sed 's/^/       | /' "$bridge_err"
            exit 1
        fi
        final_link
        if [ ! -f "$OUT/xeno-port" ] || [ -s "$OUT/link2.err" ]; then
            break
        fi
    done
fi
if [ -f "$OUT/xeno-port" ] && [ ! -s "$OUT/link2.err" ]; then
    port_text_start=""
    port_text_size=""
    read -r port_text_start port_text_size < <(
        awk -v obj="$PORT_OVERRIDE_OBJECT" \
            '$1 == ".text" && $4 == obj {print $2, $3; exit}' "$LINK_MAP"
    )
    if [ -z "$port_text_start" ] || [ -z "$port_text_size" ]; then
        echo "ERROR: final link map has no .text range for $PORT_OVERRIDE_OBJECT"
        exit 1
    fi
    for sym in "${PORT_OVERRIDE_SYMBOLS[@]}"; do
        port_symbol_offset="$(nm -g --defined-only "$PORT_OVERRIDE_OBJECT" 2>/dev/null \
            | awk -v sym="$sym" '$3 == sym {print "0x" $1; exit}')"
        resolved_address="$(nm -g --defined-only "$OUT/xeno-port" 2>/dev/null \
            | awk -v sym="$sym" '$3 == sym {print "0x" $1; exit}')"
        if [ -z "$port_symbol_offset" ] || [ -z "$resolved_address" ]; then
            echo "ERROR: final link ownership check has no address for $sym"
            exit 1
        fi
        expected_address="$(printf '0x%016x' $((port_text_start + port_symbol_offset)))"
        if [ "$resolved_address" != "$expected_address" ]; then
            echo "ERROR: final link owner is not the port definition for $sym"
            echo "       resolved=$resolved_address expected_port=$expected_address"
            exit 1
        fi
    done
    echo "    LINK OK -> $OUT/xeno-port (port-owned addresses verified)"

    # Native field packets use compact 24-bit host links. Optional metadata
    # must not silently push their static backing buffer, or any game/port
    # object's data, out of that range.
    if ! python3 tools/analysis/field_render_address_guard.py "$OUT/xeno-port" --map "$LINK_MAP"; then
        echo "ERROR: native field packet storage exceeds compact GPU addresses."
        exit 1
    fi

    # The shipped binary is the artifact that matters. An adopted overlay leaf
    # must be a real definition here and must NOT be one of the generated
    # stubs: runtime_bridge_call() refuses stub-backed overlay targets, so a
    # leaf in both places would be reinterpreted while the allowlist claims it
    # runs natively.
    defeated=""
    for leaf in $(python3 tools/scripts/battle_overlay_host_tus.py --leaves); do
        if ! nm -g --defined-only "$OUT/xeno-port" 2>/dev/null \
            | awk -v s="$leaf" '$3 == s {found=1} END {exit !found}'; then
            echo "ERROR: adopted leaf is absent from the linked binary: $leaf"
            defeated="$defeated $leaf(missing)"
        elif rg -q "\"$leaf\"" "$OUT/stubs.c" 2>/dev/null; then
            echo "ERROR: adopted leaf is also a generated stub: $leaf"
            defeated="$defeated $leaf(stub)"
        fi
    done
    if [ -n "$defeated" ]; then
        echo "ERROR: the overlay host allowlist is not effective:$defeated"
        exit 1
    fi
    echo "    overlay host allowlist verified against the linked binary"

    # Same for the results allowlist: dlsym() needs each body exported by the
    # shipped binary (-rdynamic), and never as a generated stub.
    for sym in $BATTLE_RESULTS_SYMBOLS; do
        if ! nm -D --defined-only "$OUT/xeno-port" 2>/dev/null \
            | awk -v s="$sym" '$3 == s {found=1} END {exit !found}'; then
            echo "ERROR: adopted results body is not exported by the linked binary: $sym"
            exit 1
        fi
        if rg -q "\"$sym\"" "$OUT/stubs.c" 2>/dev/null; then
            echo "ERROR: adopted results body is also a generated stub: $sym"
            exit 1
        fi
    done
    echo "    results host allowlist verified against the linked binary"

    # Being in the binary is not enough: an adopted body that reaches a
    # generated stub calls a placeholder where retail ran real code, and it
    # would do so silently. Walk each adopted leaf's call graph through the
    # battle C bodies and refuse any path that lands on a stub. Address-taken
    # functions count, since a later JALR reaches the same placeholder.
    if ! python3 tools/scripts/battle_overlay_host_tus.py --check-stubs \
        "$OUT/stubs.c"; then
        echo "ERROR: adopted overlay leaves reach generated stubs."
        exit 1
    fi

    # No Sony Psy-Q code in the port (pc_port/THIRD_PARTY.md): refuse a binary
    # that links a matching-build Psy-Q TU (src/**/psyq/**, via DWARF compile
    # units) or defines func_XXXXXXXX at a Psy-Q address outside the compat
    # layer. Release packaging additionally requires the retail table scan.
    if ! python3 tools/analysis/psyq_port_guard.py -q --no-sources --no-disc \
        --strict --require-empty-allowlist --require-dwarf --binary "$OUT/xeno-port"; then
        echo "ERROR: Psy-Q build guard failed (tools/analysis/psyq_port_guard.py)."
        exit 1
    fi
    echo "    Psy-Q exclusion verified against the linked binary"
    if [ "${XENO_NATIVE_ONLY:-0}" = "1" ]; then
        if nm -a "$OUT/xeno-port" | awk '$NF == "execute_one" {found=1} END {exit !found}'; then
            echo "ERROR: native-only binary contains the CPU instruction engine."
            exit 1
        fi
        echo "    native-only target: CPU instruction engine excluded; incomplete paths still refuse"
    fi
else
    echo "    LINK incomplete; remaining errors:"
    grep -oE "undefined reference to \`[A-Za-z0-9_]+'|multiple definition of \`[A-Za-z0-9_]+'" "$OUT/link2.err" | sort | uniq -c | sort -rn | head -20
    exit 1
fi
