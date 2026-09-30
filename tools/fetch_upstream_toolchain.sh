#!/usr/bin/env bash
# Fetch the build pieces this repository does not redistribute.
#
# This repository leaves out the Sony Psy-Q SDK (headers and the decompiled
# SDK libraries), the GCC 2.x PSX compiler binaries and other prebuilt
# binaries.  The upstream decompilation (ladysilverberg/xenogears-decomp)
# hosts them publicly, and decompals/old-gcc publishes the one compiler
# upstream does not have.  This script downloads them into YOUR working tree
# at build time.  Nothing it fetches is committed (see .gitignore).
#
# What it puts in place:
#   tools/gcc-2.6.0-psx/, tools/gcc-2.7.2-psx/, tools/gcc-2.7.2-cdk-psx/
#                                 from upstream (pinned commit, sha256-checked)
#   tools/gcc-2.6.3-psx/          from decompals/old-gcc release 0.12
#   include/psyq/                 from upstream, plus this project's port-only
#                                 blocks (tools/scripts/adjust_upstream_psyq_headers.py,
#                                 sha256-checked: byte-identical to ours)
#   src/slus_006.64/psyq/         from upstream.  Upstream's copy is older than
#                                 this project's (not published), but the matching
#                                 build of SLUS_006.64 still matches retail with it
#   tools/objdiff/objdiff         from upstream (only for `make report`)
#   tools/gears/prebuilt/gears    built from tools/gears/src with cargo when
#                                 cargo is installed, else upstream's prebuilt
#   tools/maspsx                  the git submodule (mkst/maspsx, pinned)
#
# While src/slus_006.64/system/memory.c is not published, it also switches
# that file's .sdata segment in config/slus_006.64.yaml to assembly, so splat
# takes it from your own SLUS_006.64.
#
# It never touches disc/ and never downloads game data.  Run it from anywhere
# inside the repository; re-running is safe (existing files are kept unless
# --force is given).
#
#   tools/fetch_upstream_toolchain.sh [--force] [--no-psyq-src] [--no-gears-build]
set -euo pipefail

UPSTREAM_URL="${UPSTREAM_URL:-https://github.com/ladysilverberg/xenogears-decomp.git}"
UPSTREAM_REV="${UPSTREAM_REV:-00cd201ec397bc8eb2db34f63421a7226a450ec2}"
OLDGCC_263_URL="https://github.com/decompals/old-gcc/releases/download/0.12/gcc-2.6.3-psx.tar.gz"
OLDGCC_263_SHA256="f109838028947d1a7b595d0cd85c7ab8c8770ea6bb59ecdf1cb506867bb5d41b"
MASPSX_URL="https://github.com/mkst/maspsx"

FORCE=0
PSYQ_SRC=1
GEARS_BUILD=1
for a in "$@"; do
    case "$a" in
        --force) FORCE=1 ;;
        --no-psyq-src) PSYQ_SRC=0 ;;
        --no-gears-build) GEARS_BUILD=0 ;;
        -h|--help) sed -n '2,34p' "$0"; exit 0 ;;
        *) echo "unknown option: $a" >&2; exit 2 ;;
    esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
CACHE="$ROOT/.upstream-cache"
UP="$CACHE/xenogears-decomp"

say() { printf '==> %s\n' "$*"; }
warn() { printf 'WARNING: %s\n' "$*" >&2; }
need() { command -v "$1" >/dev/null 2>&1 || { echo "ERROR: '$1' is required" >&2; exit 1; }; }
need git; need curl; need tar; need python3; need sha256sum

# sha256 of the compiler binaries the matching build was verified with.
# path<space>sha256
COMPILER_SUMS="
tools/gcc-2.6.0-psx/cc1 12b60bd8fe26c5d63145f190038f2ef498028d1cfa250c60d19231b7ff66ddbe
tools/gcc-2.6.0-psx/cpp 2ac1db289f4ca4dc53f7e52c8b6c0d465374441d43b431806643495b19404a7d
tools/gcc-2.6.0-psx/gcc 61de0d76a969a236694db0234a361cd6bb2706375a55cc41bf3d897bc42e7f0e
tools/gcc-2.7.2-psx/cc1 11f093cbb18b9ef132782bd2f37a50320069fa452acaf9e74e6a32cb28676cdb
tools/gcc-2.7.2-psx/cpp 7b1da3e118fe4c015e71bdf31b6a5d2ca916f6ddf86918d42088433e7840aa03
tools/gcc-2.7.2-psx/gcc 4eb1d9e0335ef61aa484afe1c671e807f54a2526c483a48ee6ae9e5e98f408c6
tools/gcc-2.7.2-cdk-psx/cc1 6f11ba7ed072cc737c3d995274d380e2bd27fc1c64c04147d8cc8e89b79f8d3f
tools/gcc-2.7.2-cdk-psx/cpp 7fd5e654858c408c56eaab95a6f4fbd768918bc527eb4f6e8e2fc738db3dc9e2
tools/gcc-2.7.2-cdk-psx/gcc 9ebdeee2378f4597fc7c8934342c662911ec1751b1252140f56dc373f6160c7d
tools/gcc-2.6.3-psx/cc1 16696ec7b00a15c76bb5c245904799b547af59028416896e833eabc2939ed17a
"

# ---- 1. upstream checkout (pinned) ----------------------------------------
if [ ! -d "$UP/.git" ]; then
    say "Fetching upstream $UPSTREAM_URL @ ${UPSTREAM_REV:0:12}"
    mkdir -p "$CACHE"
    git init -q "$UP"
    git -C "$UP" remote add origin "$UPSTREAM_URL"
fi
if [ "$(git -C "$UP" rev-parse -q --verify HEAD 2>/dev/null || true)" != "$UPSTREAM_REV" ]; then
    git -C "$UP" fetch -q --depth 1 origin "$UPSTREAM_REV"
    git -C "$UP" checkout -q --detach FETCH_HEAD
fi
[ "$(git -C "$UP" rev-parse HEAD)" = "$UPSTREAM_REV" ] || { echo "ERROR: upstream checkout is not $UPSTREAM_REV" >&2; exit 1; }

copy_dir() {  # src(in upstream) dst(in this tree)
    local src="$UP/$1" dst="$ROOT/$2"
    if [ -e "$dst" ] && [ -n "$(ls -A "$dst" 2>/dev/null)" ] && [ "$FORCE" = 0 ]; then
        say "$2: present, kept (use --force to replace)"
        return
    fi
    [ -d "$src" ] || { echo "ERROR: upstream has no $1" >&2; exit 1; }
    rm -rf "$dst"; mkdir -p "$(dirname "$dst")"
    cp -a "$src" "$dst"
    say "$2: fetched from upstream"
}

# ---- 2. compilers ----------------------------------------------------------
for c in gcc-2.6.0-psx gcc-2.7.2-psx gcc-2.7.2-cdk-psx; do
    copy_dir "tools/$c" "tools/$c"
    chmod +x "tools/$c"/* || true
done
if [ ! -x tools/gcc-2.6.3-psx/cc1 ] || [ "$FORCE" = 1 ]; then
    say "tools/gcc-2.6.3-psx: downloading decompals/old-gcc 0.12"
    tmp="$CACHE/gcc-2.6.3-psx.tar.gz"
    curl -fsSL -o "$tmp" "$OLDGCC_263_URL"
    echo "$OLDGCC_263_SHA256  $tmp" | sha256sum -c --quiet -
    rm -rf tools/gcc-2.6.3-psx; mkdir -p tools/gcc-2.6.3-psx
    tar -xzf "$tmp" -C tools/gcc-2.6.3-psx
else
    say "tools/gcc-2.6.3-psx: present, kept"
fi
bad=0
while read -r f want; do
    [ -n "$f" ] || continue
    got="$(sha256sum "$f" 2>/dev/null | cut -d" " -f1)"
    if [ "$got" != "$want" ]; then warn "$f: sha256 ${got:-missing}, expected $want"; bad=1; fi
done <<< "$COMPILER_SUMS"
[ "$bad" = 0 ] && say "compilers: sha256 verified"

# ---- 3. Psy-Q headers (+ this project's port blocks) -----------------------
copy_dir include/psyq include/psyq
python3 tools/scripts/adjust_upstream_psyq_headers.py include/psyq \
    || warn "include/psyq: port adjustments not applied; the port build may fail"

# ---- 4. decompiled Psy-Q library sources -----------------------------------
if [ "$PSYQ_SRC" = 1 ]; then
    copy_dir src/slus_006.64/psyq src/slus_006.64/psyq
    say "src/slus_006.64/psyq: upstream @ ${UPSTREAM_REV:0:12} (older than this project's copy;
    SLUS_006.64 still builds byte-identical with it; the port build does not use it)"
fi

# ---- 5. objdiff (optional, `make report`) ----------------------------------
if [ ! -x tools/objdiff/objdiff ] || [ "$FORCE" = 1 ]; then
    cp "$UP/tools/objdiff/objdiff" tools/objdiff/objdiff && chmod +x tools/objdiff/objdiff
    say "tools/objdiff/objdiff: fetched from upstream"
fi

# ---- 6. gears (the build driver) -------------------------------------------
if [ ! -x tools/gears/prebuilt/gears ] || [ "$FORCE" = 1 ]; then
    mkdir -p tools/gears/prebuilt
    if [ "$GEARS_BUILD" = 1 ] && command -v cargo >/dev/null 2>&1; then
        say "tools/gears: building from source with cargo"
        (cd tools/gears && cargo build --release --quiet)
        cp tools/gears/target/release/gears tools/gears/prebuilt/gears
    else
        warn "cargo not found: using upstream's prebuilt gears, which is older than
         tools/gears/src here.  Install Rust (https://rustup.rs) and re-run with --force."
        cp "$UP/tools/gears/prebuilt/gears" tools/gears/prebuilt/gears
    fi
    chmod +x tools/gears/prebuilt/gears
else
    say "tools/gears/prebuilt/gears: present, kept"
fi

# ---- 7. maspsx submodule ---------------------------------------------------
if [ ! -f tools/maspsx/maspsx.py ]; then
    if [ "$(git -C "$ROOT" ls-files -s tools/maspsx 2>/dev/null | cut -c1-6)" = 160000 ]; then
        say "tools/maspsx: git submodule update --init"
        git -C "$ROOT" submodule update --init tools/maspsx
    else
        say "tools/maspsx: cloning $MASPSX_URL (no git checkout of this repo)"
        rm -rf tools/maspsx; git clone -q "$MASPSX_URL" tools/maspsx
        git -C tools/maspsx checkout -q 5e2ad437970b1ab4de3f59aefe14d20838d88728
    fi
else
    say "tools/maspsx: present"
fi

# ---- 8. what is still missing ----------------------------------------------
# Game source files this repository cannot publish yet (they still contain
# retail bytes or retail data tables) and that no public source provides.
NOT_PUBLISHED="
src/member_change_menu/main/misc.c
src/movie/main.c
src/slus_006.64/main/main_loop.c
src/slus_006.64/system/kernel_menu.c
src/slus_006.64/system/sound.c
src/slus_006.64/system/memory.c
src/slus_006.64/system/animation_scripts.c
src/menu/main/misc.c
src/menu/main/misc3.c
"
missing=()
for f in $NOT_PUBLISHED; do [ -f "$f" ] || missing+=("$f"); done

# While src/slus_006.64/system/memory.c is absent, let splat emit its .sdata
# from your SLUS_006.64 as assembly (the C stub splat writes has no data).
# Undo with: git checkout config/slus_006.64.yaml
MEM_SDATA='\[0x49A48, \.sdata, system/memory\]'
if [ ! -f src/slus_006.64/system/memory.c ] && grep -q "$MEM_SDATA" config/slus_006.64.yaml; then
    sed -i "s|$MEM_SDATA|[0x49A48, sdata, system/memory] # asm: memory.c not published|" config/slus_006.64.yaml
    say "config/slus_006.64.yaml: system/memory .sdata switched to asm (memory.c is not published)"
fi

echo
say "Toolchain and SDK pieces are in place."
if [ "${#missing[@]}" -gt 0 ]; then
    echo "NOTE: ${#missing[@]} game source file(s) are not published yet (see README.md,"
    echo "      'Building from source'):"
    printf '        %s\n' "${missing[@]}"
    echo "      'make' has splat regenerate them from your disc as assembly stubs.  That is"
    echo "      enough for SLUS_006.64 and every overlay except menu.bin, which needs the"
    echo "      real src/menu/main/misc.c.  pc_port/build_port.sh needs all of them."
fi
[ -f disc/SLUS_006.64 ] || echo "NOTE: disc/ has no SLUS_006.64 yet: copy your own extracted disc files there."
