#!/usr/bin/env bash
# package_linux.sh OUTDIR [BINARY]
#
# Package the built PC port (pc_port/build_native/xeno-port) as a
# self-contained Linux x86_64 release directory plus tarball in OUTDIR:
#
#   xenogears-port-<id>/
#     xenogears.sh        launcher (fullscreen by default; data dir = arg 1,
#                         $XENOGEARS_DATA, or ~/.config/xenogears-port/data_dir)
#     bin/xeno-port       the port
#     lib/                non-system shared libraries the port needs
#     config.example.ini  settings reference (copy to ~/.config/xenogears-port/)
#     version.txt, build-info.json, MANIFEST (sha256 of every file),
#     MANIFEST.sizes.tsv (sha256, size, path for every file)
#     README-TEST.txt     what to supply, how far it plays, known issues
#     THIRD_PARTY.md      licences of bundled/linked third-party code
#
# No game data is packaged: the owner supplies the retail files (see
# README-TEST.txt), and the finished directory is scanned with
# tools/analysis/retail_data_guard.py before the tarball is written.
# Run inside the build container (xenogears-dev), where the port was built,
# so the bundled libraries are the ones it linked against.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUTDIR="${1:?usage: package_linux.sh OUTDIR [BINARY]}"
BIN="${2:-$ROOT/pc_port/build_native/xeno-port}"
[ -x "$BIN" ] || { echo "no port binary at $BIN" >&2; exit 1; }

COMMIT="$(git -C "$ROOT" rev-parse HEAD)"
SHORT="$(git -C "$ROOT" rev-parse --short=10 HEAD)"
DIRTY="$(git -C "$ROOT" status --porcelain --untracked-files=no | wc -l)"
[ "$DIRTY" -eq 0 ] || { echo "tree has uncommitted tracked changes; package a committed HEAD" >&2; exit 1; }
STAMP="$(date -u +%Y%m%d%H%M)"
if [ -n "${XENO_PACKAGE_ID:-}" ]; then
    ID="$XENO_PACKAGE_ID"
    NAME="$ID"
else
    ID="${STAMP}-${SHORT}"
    NAME="xenogears-port-${ID}"
fi
PKG="$OUTDIR/$NAME"
[ ! -e "$PKG" ] || { echo "$PKG exists" >&2; exit 1; }
mkdir -p "$PKG/bin" "$PKG/lib"

install -m 0755 "$BIN" "$PKG/bin/xeno-port"
strip --strip-debug "$PKG/bin/xeno-port"

# Bundle the libraries a stock Bazzite/SteamOS/Fedora install may lack or
# carry in another ABI version.  glibc, libstdc++, X11/Wayland/GL/DRM,
# ALSA and PulseAudio stay with the host (listed in README-TEST.txt).
BUNDLE=(libSDL2-2.0.so.0 libopenal.so.1 libsndio.so.7 libcrypto.so.3
        libsamplerate.so.0 libXss.so.1 libdecor-0.so.0)
for lib in "${BUNDLE[@]}"; do
    src="$(ldconfig -p | awk -v l="$lib" '$1 == l && /x86-64/ {print $NF; exit}')"
    [ -n "$src" ] || { echo "library $lib not found" >&2; exit 1; }
    install -m 0644 "$(readlink -f "$src")" "$PKG/lib/$lib"
done
# The port finds lib/ through its RUNPATH-free launcher (LD_LIBRARY_PATH).

cp "$ROOT/pc_port/config.example.ini" "$PKG/config.example.ini"
cp "$ROOT/pc_port/THIRD_PARTY.md" "$PKG/THIRD_PARTY.md"

cat > "$PKG/xenogears.sh" <<'LAUNCHER'
#!/usr/bin/env bash
# Xenogears PC port launcher.
#   xenogears.sh [DATA_DIR] [extra xeno-port args...]
# DATA_DIR holds your own retail files (README-TEST.txt lists them). It is
# taken from arg 1, else $XENOGEARS_DATA, else the path stored in
# ~/.config/xenogears-port/data_dir (written the first time you pass one),
# else the first disc/ folder holding SLUS_006.64 next to or above the
# install (a Banshee library keeps it at games/xenogears/disc, two levels up
# from versions/<build>/).  With none found xeno-port still starts, and its
# retail-data check names the missing files in a message box.
# Fullscreen is the default; XENOGEARS_WINDOWED=1 opens a window instead.
# Saves, memory cards, logs and captures go to
# ${XDG_DATA_HOME:-~/.local/share}/xenogears-port, never the install dir.
set -eu
HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
CONF_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/xenogears-port"
DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}/xenogears-port"
mkdir -p "$CONF_DIR" "$DATA_HOME/memcards" "$DATA_HOME/quicksaves"

has_data() { [ -f "$1/SLUS_006.64" ] || [ -f "$1/disc/SLUS_006.64" ]; }
data=""
if [ -n "${1:-}" ] && [ -d "$1" ]; then
    data="$(cd "$1" && pwd)"
    shift
    printf '%s\n' "$data" > "$CONF_DIR/data_dir"
elif [ -n "${XENOGEARS_DATA:-}" ] && [ -d "$XENOGEARS_DATA" ]; then
    data="$(cd "$XENOGEARS_DATA" && pwd)"
else
    saved=""
    [ -f "$CONF_DIR/data_dir" ] && saved="$(head -n1 "$CONF_DIR/data_dir")"
    for cand in "$saved" "$HERE/disc" "$HERE/../disc" "$HERE/../../disc" "$HERE/../../../disc"; do
        if [ -n "$cand" ] && [ -d "$cand" ] && has_data "$cand"; then
            data="$(cd "$cand" && pwd)"
            break
        fi
    done
fi

if [ -n "$data" ]; then
    data_files="$data"
    if [ ! -f "$data_files/SLUS_006.64" ] && [ -f "$data/disc/SLUS_006.64" ]; then
        data_files="$data/disc"
    fi
    export XENO_DATA_DIR="$data_files"
    if [ -z "${XENO_DISC:-}" ]; then
        if [ -f "$data_files/disc1.bin" ]; then
            export XENO_DISC="$data_files/disc1.bin"
        elif [ -f "$data/disc/disc1.bin" ]; then
            export XENO_DISC="$data/disc/disc1.bin"
        fi
    fi
else
    echo "xenogears.sh: no folder with your Xenogears files found; pass it as the" \
         "first argument (see README-TEST.txt)" >&2
fi
export XENO_MEMCARD_DIR="$DATA_HOME/memcards"
export XENO_QUICKSAVE_PATH="$DATA_HOME/quicksaves/quick.xgqs"
[ -n "${XENOGEARS_WINDOWED:-}" ] || export XENO_VIDEO_FULLSCREEN="${XENO_VIDEO_FULLSCREEN:-on}"
[ -f "$CONF_DIR/config.ini" ] && export XENO_CONFIG="$CONF_DIR/config.ini"
export LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$DATA_HOME"
exec "$HERE/bin/xeno-port" "$@"
LAUNCHER
chmod 0755 "$PKG/xenogears.sh"

cat > "$PKG/version.txt" <<EOF
$ID
commit $COMMIT
EOF

GATE_JSON="${XENO_GATE_JSON:-{\}}"
cat > "$PKG/build-info.json" <<EOF
{
  "name": "xenogears-port",
  "build_id": "$ID",
  "commit": "$COMMIT",
  "built_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "platform": "linux-x86_64",
  "glibc_min": "$(objdump -T "$PKG/bin/xeno-port" | grep -oE 'GLIBC_[0-9.]+' | sort -Vu | tail -1)",
  "bundled_libs": [$(printf '"%s",' "${BUNDLE[@]}" | sed 's/,$//')],
  "contains_game_data": false,
  "gate": $GATE_JSON
}
EOF

cp "$ROOT/pc_port/tools/package_readme.txt" "$PKG/README-TEST.txt"
sed -i "s/@BUILD_ID@/$ID/; s/@COMMIT@/$COMMIT/" "$PKG/README-TEST.txt"

# ldd check: every dependency must resolve with the bundled lib/ first.
if LD_LIBRARY_PATH="$PKG/lib" ldd "$PKG/bin/xeno-port" | grep -q 'not found'; then
    echo "ldd: unresolved libraries" >&2
    LD_LIBRARY_PATH="$PKG/lib" ldd "$PKG/bin/xeno-port" | grep 'not found' >&2
    exit 1
fi
LD_LIBRARY_PATH="$PKG/lib" ldd "$PKG/bin/xeno-port" > "$OUTDIR/$NAME.ldd.txt"

# No-game-data scan over every packaged file.
(cd "$ROOT" && python3 tools/analysis/retail_data_guard.py --strict \
    $(find "$PKG" -type f | sort)) > "$OUTDIR/$NAME.datascan.txt" 2>&1 || {
    echo "retail data guard flagged the package:" >&2
    cat "$OUTDIR/$NAME.datascan.txt" >&2
    exit 1
}

(cd "$PKG" && find . -type f ! -name MANIFEST -print0 | sort -z | xargs -0 sha256sum > MANIFEST)
(cd "$PKG" && find . -type f ! -name MANIFEST ! -name MANIFEST.sizes.tsv -print0 | sort -z |
    while IFS= read -r -d '' file; do
        printf '%s\t%s\t%s\n' "$(sha256sum "$file" | cut -d' ' -f1)" \
            "$(stat -c '%s' "$file")" "$file"
    done > MANIFEST.sizes.tsv)
tar -C "$OUTDIR" -czf "$OUTDIR/$NAME.tar.gz" "$NAME"
(cd "$OUTDIR" && sha256sum "$NAME.tar.gz" > "$NAME.tar.gz.sha256")
echo "$OUTDIR/$NAME.tar.gz"
