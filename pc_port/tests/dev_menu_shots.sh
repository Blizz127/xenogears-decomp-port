#!/usr/bin/env bash
# Dev-menu screenshots of the WARP page, an area folder and the FINISH AREA page,
# HEADLESS (offscreen GL, null audio, private XDG dirs, no X display).
#   dev_menu_shots.sh BIN OUTDIR
# Uses the harness-only XENO_DEV_MENU_KEYS="present:KEY,..." script
# (psycross_port_dev_menu.inl); `SHOT=name` saves the presented frame WITH the
# overlay to $XENO_DEV_MENU_SHOT_DIR/name.bmp.  The menu model does not need a
# field, so the pages are captured at the title.
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
set -u
BIN=$(readlink -f "$1"); OUT=$(mkdir -p "$2" && readlink -f "$2")
DATA=${XENO_DATA_DIR:?XENO_DATA_DIR must point at the disc files}
LIBDIR="$(cd "$(dirname "$BIN")/../lib" 2>/dev/null && pwd)"
mkdir -p "$OUT/xdg/config" "$OUT/xdg/data" "$OUT/xdg/cache" "$OUT/xdg/state" "$OUT/xdg/run" "$OUT/work"
chmod 700 "$OUT/xdg/run"
B=${SHOT_START:-900}
KEYS="$B:F8,$((B+10)):SHOT=01_root,$((B+20)):ENTER,$((B+30)):SHOT=02_warp,$((B+40)):ENTER,$((B+50)):SHOT=03_folder,\
$((B+60)):ESC,$((B+70)):ESC,$((B+80)):DOWN,$((B+90)):ENTER,$((B+100)):SHOT=04_finish,$((B+110)):ESC,$((B+120)):F8"
cd "$OUT/work" || exit 2
env ${LIBDIR:+LD_LIBRARY_PATH="$LIBDIR"} SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
    ALSOFT_DRIVERS=null XENO_NO_DIALOG=1 XENO_DATA_DIR="$DATA" XENO_DISC="$DATA/disc1.bin" \
    XDG_CONFIG_HOME="$OUT/xdg/config" XDG_DATA_HOME="$OUT/xdg/data" XDG_CACHE_HOME="$OUT/xdg/cache" \
    XDG_STATE_HOME="$OUT/xdg/state" XDG_RUNTIME_DIR="$OUT/xdg/run" XENO_MEMCARD_DIR="$OUT/xdg/data/memcards" \
    XENO_DEV_MENU=1 XENO_DEV_MENU_KEYS="$KEYS" XENO_DEV_MENU_SHOT_DIR="$OUT" XENO_SPEED=5 \
    timeout --signal=INT --kill-after=15 ${SHOT_TIMEOUT:-300} nice -n 19 "$BIN" >"$OUT/run.log" 2>&1
echo "rc=$?" >>"$OUT/run.log"
rm -rf "$OUT/xdg" "$OUT/work"
ls "$OUT"/*.bmp 2>/dev/null
