#!/usr/bin/env python3
"""Runtime regression: no quick save in a scene; the one taken after it loads
without replaying the scene (Lahan, field 14, the painting scene).

Phase 1 (New Game, ~12 min headless): boot -> title -> New Game -> page the
prologue and the opening battle at speed 5 -> field 14 at speed 1.  Through
the painting scene the driver asks for a quick save every 3 s and presses
confirm while an event holds control (from the port's POSDIAG canRun/b21d0),
then asks once the player has been free for six frames.  Every
request while a field script event holds control must be refused with
"CAN'T QUICKSAVE DURING A SCENE"; the first accepted one comes after the
scene, and the checkpoint's scenario flag (g_GameState+0x1930) must equal the
live one the port logged (XENO_FIELD_POS_DIAG) when the save was taken.

Phase 2: a fresh boot loads that checkpoint from the title field, waits, and
quick-saves again: with no scene replaying that save is accepted at once and
nothing is refused.  (The r6.3 checkpoint from the scene's last frame said
scenario 3 and its load replayed "Whew! That about does it..." -- run
`--load-only CKPT` on such a file to see this phase fail.)

Usage: quicksave_scene_gate_test.py BINARY ROOT OUTDIR [--load-only CKPT]
"""
import os
import re
import shutil
import struct
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib'))
from xeno_headless import CIRCLE, CROSS, UP, Port  # noqa: E402

NOTICE = "[xeno-port][quick] notice: CAN'T QUICKSAVE DURING A SCENE"
REFUSED = '[xeno-port][quick] save refused:'
SAVED = '[xeno-port][quick] saved '
POSDIAG = re.compile(r'POSDIAG map=(\d+) .*scenario=(\d+)')


def scenario_in(path):
    data = open(path, 'rb').read()
    return struct.unpack_from('<H', data, 48 + 0x1930)[0]


STATE = re.compile(r'POSDIAG map=14 .*canRun=(\d) owner=0x[0-9a-f]+ b21d0=(\d)')


def recent_states(port, start, count):
    """(canRun, b21d0) of the last `count` field-14 POSDIAG frames."""
    out = []
    with port.lock:
        for line in reversed(port.lines[start:]):
            m = STATE.search(line)
            if m:
                out.append((int(m.group(1)), int(m.group(2))))
                if len(out) == count:
                    break
    return out[::-1]


def last_scenario(port, before):
    with port.lock:
        for line in reversed(port.lines[:before]):
            m = POSDIAG.search(line)
            if m and m.group(1) == '14':
                return int(m.group(2))
    return None


def phase_newgame(binary, root, out, qs, fails):
    port = Port(binary, root, out, {'XENO_QUICKSAVE_PATH': qs, 'XENO_FIELD_POS_DIAG': '1'},
                drop=('[vm-trace]',))
    try:
        if port.wait_for('FieldLoad begin field=490', 300, every=1.0,
                         action=lambda: port.pad(CROSS)) is None:
            return 'NOT_RUN title field not reached'
        if port.wait_for('title loop enter', 300, every=1.5,
                         action=lambda: port.pad(CIRCLE)) is None:
            return 'NOT_RUN title menu not reached'
        time.sleep(1.5)
        s = port.n()
        port.pad(UP)
        time.sleep(1.0)
        if port.wait_for('title confirm choice=2', 30, s, every=2.0,
                         action=lambda: port.pad(CIRCLE)) is None:
            return 'NOT_RUN New Game not confirmed'
        port.con('speed 5')
        if port.wait_for('FieldLoad begin field=14 ', 3000, s, every=0.25,
                         action=lambda: port.pad(CIRCLE, 2)) is None:
            return 'NOT_RUN field 14 not reached'
        port.con('speed 1')
        f14 = port.n()
        # State-driven, so host speed cannot phase-lock it: confirm (to page
        # dialogs or leave the easel close-up) only while an event holds
        # control, ask for a save during the scene every 3 s, and once the
        # player has been free for several frames ask once more and press
        # nothing.  A fixed confirm/save cadence under load kept reopening
        # the easel close-up just before each save.
        deadline = time.time() + 900
        last_save = last_press = 0.0
        while time.time() < deadline and port.find(SAVED, f14) is None:
            states = recent_states(port, f14, 6)
            now = time.time()
            if len(states) == 6 and all(c == 1 and b == 0 for c, b in states):
                s = port.n()
                port.con('save')
                last_save = now
                port.wait_for(SAVED, 15, s)
                if port.find(SAVED, s) is None and port.find(REFUSED, s) is None:
                    fails.append('a save at free control got no answer')
                    break
                continue
            if states and (states[-1][0] == 0 or states[-1][1] == 1):
                if now - last_save >= 3.0:
                    port.con('save')
                    last_save = now
                    time.sleep(0.5)
                if time.time() - last_press >= 2.0:
                    port.pad(CIRCLE, 8)
                    last_press = time.time()
            time.sleep(0.25)
        saved = port.find(SAVED, f14)
        if saved is None:
            fails.append('no quick save was accepted in field 14 within 15 min')
            return None
        time.sleep(2)
        refused = [i for i in range(f14, saved) if REFUSED in port.lines[i]]
        notices = [i for i in range(f14, saved) if NOTICE in port.lines[i]]
        print('QUICKSAVE SCENE phase1 refused=%d notices=%d before the accepted save'
              % (len(refused), len(notices)))
        if not refused:
            fails.append('no save was refused during the painting scene')
        if len(notices) < len(refused):
            fails.append('a refusal showed no on-screen notice')
        live = last_scenario(port, saved)
        saved_scn = scenario_in(qs)
        print('QUICKSAVE SCENE phase1 live scenario=%s checkpoint scenario=%d' % (live, saved_scn))
        if live is None or saved_scn != live:
            fails.append('checkpoint scenario %d != live scenario %s' % (saved_scn, live))
    finally:
        port.stop()
    return None


def phase_load(binary, root, out, qs, fails):
    port = Port(binary, root, out, {'XENO_QUICKSAVE_PATH': qs, 'XENO_FIELD_POS_DIAG': '30'})
    try:
        restored = port.load_checkpoint()
        if restored is None:
            return 'NOT_RUN checkpoint did not load'
        time.sleep(10)
        s = port.n()
        port.con('save')
        port.wait_for(SAVED, 20, s)
        time.sleep(1)
        if port.find(REFUSED, restored) is not None:
            fails.append('after the load a save was refused: a scene is replaying')
        if port.find(SAVED, s) is None:
            fails.append('after the load the field never settled for a save')
    finally:
        port.stop()
    return None


def main():
    binary, root, out = sys.argv[1:4]
    os.makedirs(out, exist_ok=True)
    fails, verdict = [], None
    qs = os.path.join(out, 'field14.xgqs')
    if len(sys.argv) >= 6 and sys.argv[4] == '--load-only':
        shutil.copyfile(sys.argv[5], qs)
    else:
        verdict = phase_newgame(binary, root, os.path.join(out, 'newgame'), qs, fails)
    if verdict is None and not fails:
        verdict = phase_load(binary, root, os.path.join(out, 'load'), qs, fails)
    if verdict:
        print('QUICKSAVE SCENE overall=%s' % verdict)
        return 2
    for f in fails:
        print('QUICKSAVE SCENE FAIL %s' % f)
    print('QUICKSAVE SCENE overall=%s' % ('FAIL' if fails else 'PASS'))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
