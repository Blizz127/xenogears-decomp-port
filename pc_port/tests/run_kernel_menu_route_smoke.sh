#!/usr/bin/env bash
# Developer KernelMenu -> Menu route smoke (exercises MenuExecute's
# g_MenuDebugEnabled branch, src/slus_006.64/system/menu.c).
#
# XENO_FIELD_TEST=1 boots into the KernelMenu (state 0) instead of the retail
# movie; XENO_KERNEL_SEL=4 presses Circle on "Menu" XENO_KERNEL_DELAY frames
# later.  No field has run, so g_MenuDebugEnabled still holds its retail
# initial value 1 and MenuExecute enters the debug page loop; the pad
# schedule then presses CONFIRM (exit, input 4) so the branch's overlay
# loads run.  Under gdb, breakpoints report (a) MenuExecute's entry with the
# flag value and (b) the overlay-load statement after the debug loop; the
# run must reach both and end at the observation deadline without a fatal
# signal.  Needs gdb, the port binary and the disc (XIMG=xenogears-dev:clang
# image has gdb).
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
OUT="${XENO_KMENU_OUTDIR:-$(mktemp -d "${TMPDIR:-/tmp}/kmenu-route.XXXXXX")}"
SECONDS_MAX="${XENO_KMENU_SECONDS:-180}"
mkdir -p "$OUT"
for need in gdb; do command -v "$need" >/dev/null || { echo "KMENU NOT_RUN missing $need"; exit 2; }; done
[ -x "$BINARY" ] || { echo "KMENU NOT_RUN binary_missing=$BINARY"; exit 2; }
[ -f disc/disc1.bin ] || { echo "KMENU NOT_RUN disc_missing"; exit 2; }

line=$(grep -n 'pBuf0 = HeapAlloc(0x4, 0x1);' src/slus_006.64/system/menu.c | head -1 | cut -d: -f1)
[ -n "$line" ] || { echo "KMENU FAIL overlay-load statement not found in menu.c"; exit 1; }

# CONFIRM pulses (2 frames on / 18 off) once the debug page is up.  Schedule
# values are GAME buttons (XENO_PAD_TEST_INPUT presses the physical button the
# active layout maps to them): game Circle (0x20) = MENU_INPUT_CONFIRM, which
# under the retail USA layout is the physical Cross.
schedule="0:0"
# Start well after the forced KernelMenu select (~30 frames after MainLoop,
# about frame 440): a confirm while the KernelMenu is still up would confirm
# its default entry (Field) instead.  XENO_KMENU_CONFIRM_FROM overrides.
from="${XENO_KMENU_CONFIRM_FROM:-900}"
for f in $(seq "$from" 20 $((from + 2000))); do schedule+=",$f:0x20,$((f + 2)):0"; done

cat > "$OUT/gdb.cmds" <<GDB
set pagination off
set confirm off
handle SIGPIPE nostop noprint pass
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
break MenuExecute
commands
silent
printf "KMENU MenuExecute entry g_MenuDebugEnabled=%d\n", (int)g_MenuDebugEnabled
continue
end
break menu.c:$line
commands
silent
printf "KMENU debug-branch overlay loads reached\n"
continue
end
run
printf "KMENU stopped; backtrace follows\n"
bt 16
GDB

cd pc_port/build_native
XENO_FIELD_TEST=1 XENO_KERNEL_SEL=4 XENO_KERNEL_DELAY="${XENO_KERNEL_DELAY:-30}" \
XENO_PAD_TEST_INPUT="$schedule" SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" \
    timeout --signal=INT --kill-after=15 "${SECONDS_MAX}s" \
    gdb -q -batch -x "$OUT/gdb.cmds" --args "$BINARY" > "$OUT/run.log" 2>&1
rc=$?
cd "$ROOT"

fail=0
check() { if grep -q "$2" "$OUT/run.log"; then echo "KMENU PASS $1"; else echo "KMENU FAIL $1"; fail=1; fi; }
check kernel.menu.select.fired 'forcing KernelMenu select 4'
check menuexecute.entered.with.debug.flag 'KMENU MenuExecute entry g_MenuDebugEnabled=1'
check debug.branch.overlay.loads.reached 'KMENU debug-branch overlay loads reached'
if grep -qE 'SIGSEGV|SIGBUS|SIGILL|SIGABRT|SIGFPE|received signal SIG(SEGV|BUS|ILL|ABRT|FPE)' "$OUT/run.log"; then
    echo "KMENU FAIL no.fatal.signal"; grep -m1 -A4 'received signal' "$OUT/run.log"
    sed -n '/KMENU stopped; backtrace follows/,$p' "$OUT/run.log" | head -20; fail=1
else
    echo "KMENU PASS no.fatal.signal"
fi
echo "KMENU runtime_rc=$rc log=$OUT/run.log"
if [ "$fail" -eq 0 ]; then echo "KMENU overall=PASS"; else echo "KMENU overall=FAIL"; fi
exit "$fail"
