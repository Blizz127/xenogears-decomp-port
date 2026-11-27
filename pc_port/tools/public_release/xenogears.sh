#!/usr/bin/env bash
# Xenogears PC port launcher.
#   xenogears.sh [DATA_DIR] [extra xeno-port args...]
# DATA_DIR holds your own retail files (README.md lists them). It is
# taken from arg 1, else $XENOGEARS_DATA, else the path stored in
# ~/.config/xenogears-port/data_dir (written the first time you pass one),
# else the first disc/ folder holding SLUS_006.64 next to or above the
# install.  With none found xeno-port still starts, and its
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
         "first argument (see README.md)" >&2
fi
export XENO_CHEATS=0
export XENO_MEMCARD_DIR="$DATA_HOME/memcards"
export XENO_QUICKSAVE_PATH="$DATA_HOME/quicksaves/quick.xgqs"
[ -n "${XENOGEARS_WINDOWED:-}" ] || export XENO_VIDEO_FULLSCREEN="${XENO_VIDEO_FULLSCREEN:-on}"
[ -f "$CONF_DIR/config.ini" ] && export XENO_CONFIG="$CONF_DIR/config.ini"
export LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$DATA_HOME"
exec "$HERE/bin/xeno-port" "$@"
