#!/usr/bin/env bash
# Dev-menu keyboard path, end to end, headless: real F8 key presses pushed
# through SDL's event queue (XENO_DEV_MENU_KEYS "SDL:F8") must reach the dev
# menu ahead of every host hotkey, and do nothing when the menu is disabled.
#   on   XENO_DEV_MENU=1: "[DEV_MENU] enabled", keys line lists F8,
#        F8 opens the menu and a second F8 closes it
#   off  XENO_DEV_MENU=0: "[DEV_MENU] disabled", F8 opens nothing
# Needs the user's disc files (disc/ or XENO_DATA_DIR) like the title smoke.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
OUT="${XENO_DEV_MENU_KEYS_TEST_OUTDIR:-$ROOT/scratchpad/dev_menu_keys/$(date +%Y%m%d-%H%M%S)}"
SECS="${XENO_DEV_MENU_KEYS_TEST_SECONDS:-120}"
mkdir -p "$OUT"
[ -x "$BIN" ] || { echo "DEV MENU KEYS overall=NOT_RUN binary_missing=$BIN"; exit 2; }
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
fail=0
check() { if eval "$2"; then echo "DEV MENU KEYS [$1] PASS $3"; else echo "DEV MENU KEYS [$1] FAIL $3"; fail=1; fi; }

run() {  # name menu_on
    local d="$OUT/$1"; mkdir -p "$d/xdg"
    ( cd "$ROOT" && env HOME="$d/xdg" XDG_CONFIG_HOME="$d/xdg/config" XDG_DATA_HOME="$d/xdg/data" \
        XDG_CACHE_HOME="${XDG_CACHE_HOME:-$d/xdg/cache}" SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
        ALSOFT_DRIVERS=null XENO_NO_DIALOG=1 XENO_DEV_MENU="$2" \
        XENO_DEV_MENU_KEYS="300:SDL:F8,360:SDL:F8" \
        timeout -s INT "$SECS" "$BIN" > "$d/run.log" 2>&1 )
}

run on 1
L="$OUT/on/run.log"
check on "grep -aq '\[DEV_MENU\] enabled' '$L'" "startup says enabled"
check on "grep -aq '\[xeno-port\] keys: .*F11 quick save.*F8 dev menu' '$L'" "keys line lists F8 dev menu"
check on "grep -aq '\[DEV_MENU\] scripted SDL:F8 at present 300' '$L'" "real F8 key pushed at present 300"
check on "grep -aq '\[DEV_MENU\] open' '$L'" "F8 opened the dev menu"
check on "grep -aq '\[DEV_MENU\] close' '$L'" "second F8 closed it"
check on "! grep -aqE 'quick save requested|Saving screenshot|speed: ' '$L'" "F8 triggered no host hotkey"

run off 0
L="$OUT/off/run.log"
check off "grep -aq '\[DEV_MENU\] disabled' '$L'" "startup says disabled and how to enable"
check off "grep -aq '\[DEV_MENU\] scripted SDL:F8 at present 300' '$L'" "real F8 key pushed at present 300"
check off "! grep -aq '\[DEV_MENU\] open' '$L'" "F8 opened nothing"

[ "$fail" = 0 ] && echo "DEV MENU KEYS overall=PASS" || echo "DEV MENU KEYS overall=FAIL outdir=$OUT"
exit "$fail"
