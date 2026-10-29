#!/usr/bin/env python3
"""Runtime acceptance: field Save -> title Continue -> Load restores the field.

  card_save_load_test.py BINARY OUTDIR CHECKPOINT.xgqs

Run 1 (save): boot, quick-load CHECKPOINT (a field checkpoint), set the
save-point flag with the TEST TOOLING console command `saveflag 1` (what
opcode FE99, func_8008848C, does inside a save point's area), open the field
menu (Triangle), confirm File (the root cursor starts there when the flag is
set), confirm Save on the card screen, pick the first slot and answer Yes to
every yes/no prompt.  The retail Save action (func_801CBD90) writes the file
to the port's virtual card in OUTDIR/memcards.  The driver records the last
POSDIAG line before the menu opened.

Run 2 (load): a new process on the same card directory (a cold return to the
title), open the title menu, confirm Continue, confirm Load, pick the first
slot, answer Yes; the retail Load action (func_801CB304) reads it back,
Continue returns D_800594D0=2 and the field loads the saved map.

PASS needs: the card image holds exactly one retail-format save file; the
load run leaves the title with D_800594D0=2 and loads the saved field; the
loaded POSDIAG matches the saved map, scenario flag and party HP; no
GameHandleError and no [stub] in either run.

Where the player stands after the load: retail does not restore the saved
x/z.  On a Continue load func_800799D4 sets the spawn entrance
g_GameState+0x2320 from script var 0x54 (g_GameState+0x1984; retail
0x80079F88..0x80079F90), which a real save point's script sets to its own
entrance.  So run 3 (reference) warps a fresh process to that entrance of the
saved map with the dev menu (`dev warpx MAP ENTRANCE -1`) and the loaded
position must match it within POS_TOL.
Only the save flag is forced; menus, card I/O and the load are retail code.
"""
import os
import re
import shutil
import struct
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib'))
from xeno_headless import Port, CIRCLE, CROSS, TRIANGLE, LEFT  # noqa: E402
import psx_card_image  # noqa: E402

POS_TOL = 64
POSDIAG = re.compile(r'POSDIAG map=(\d+) pos=\((-?\d+),(-?\d+),(-?\d+)\) .*?scenario=(\d+)')

results = []


def check(name, ok, detail=''):
    results.append(ok)
    print('CARD SAVE/LOAD %s %s%s' % ('PASS' if ok else 'FAIL', name,
                                     (' (%s)' % detail) if detail else ''), flush=True)
    return ok


def last_match(port, pat, start=0):
    with port.lock:
        seg = port.lines[start:]
    for line in reversed(seg):
        m = pat.search(line)
        if m:
            return m, line
    return None, None


def partyhp(port, start=0):
    with port.lock:
        seg = port.lines[start:]
    for line in reversed(seg):
        if '[xeno-port][test] PARTYHP' in line:
            return line.split('PARTYHP', 1)[1].strip()
    return None


def drive(port, needle, start, timeout, poke=None, every=3.0):
    """Wait for needle; answer Yes (Left, Circle) to each interactive prompt
    printed meanwhile; call poke() every `every` s while no prompt is open."""
    seen = start
    deadline = time.time() + timeout
    last = time.time()
    while time.time() < deadline and port.proc.poll() is None:
        i = port.find(needle, start)
        if i is not None:
            return i
        j = port.find('[xeno-port][menu] prompt msg=', seen)
        if j is not None:
            seen = j + 1
            with port.lock:
                line = port.lines[j]
            if 'interactive=1' in line:
                time.sleep(1.5)
                port.pad(LEFT)
                time.sleep(1.0)
                port.pad(CIRCLE)
                last = time.time()
            continue
        if poke and time.time() - last >= every:
            poke()
            last = time.time()
        time.sleep(0.2)
    return port.find(needle, start)


def no_errors(port, tag):
    with port.lock:
        text = '\n'.join(port.lines)
    check(tag + '.no.game.handle.error', 'GameHandleError' not in text)
    stubs = [l for l in text.split('\n') if '[stub]' in l]
    check(tag + '.no.stub.reached', not stubs, stubs[0] if stubs else '')


def save_run(binary, root, out, ckpt, cards):
    os.makedirs(out, exist_ok=True)
    qs = os.path.join(out, 'quick.xgqs')
    shutil.copy(ckpt, qs)
    port = Port(binary, root, out, {'XENO_QUICKSAVE_PATH': qs, 'XENO_FIELD_POS_DIAG': '30',
                                    'XENO_MEMCARD_DIR': cards})
    saved = None
    try:
        if not check('save.checkpoint.loaded', port.load_checkpoint() is not None):
            return None
        time.sleep(5)
        s = port.n()
        port.con('saveflag 1')
        if not check('save.flag.forced', port.wait_for('[cheat] saveflag 1', 30, s) is not None):
            return None
        s = port.n()
        if not check('save.flag.visible', port.wait_for('SAVEFLAG 1', 30, s) is not None):
            return None
        time.sleep(2)
        m, line = last_match(port, POSDIAG)
        hp = partyhp(port)
        s = port.n()
        if not check('save.field.menu.opened',
                     port.wait_for('func_801C55A0 input loop', 60, s, every=3.0,
                                   action=lambda: port.pad(TRIANGLE)) is not None):
            return None
        saved = (m, hp, line)
        time.sleep(3)
        s = port.n()
        port.pad(CIRCLE)
        i = port.wait_for('root confirm choice=', 20, s)
        check('save.root.confirm.file', i is not None and 'choice=1' in port.lines[i],
              port.lines[i] if i is not None else 'no root confirm')
        i = port.wait_for('card screen enter mode=0', 30, s)
        if not check('save.card.screen.offers.save', i is not None and 'saveflag=1' in port.lines[i],
                     port.lines[i] if i is not None else 'no card screen'):
            return None
        time.sleep(3)
        s = port.n()
        port.pad(CIRCLE)
        i = port.wait_for('card action begin choice=', 20, s)
        if not check('save.action.is.save', i is not None and 'choice=2' in port.lines[i],
                     port.lines[i] if i is not None else 'no action'):
            return None
        time.sleep(3)
        port.pad(CIRCLE)  # pick the first slot
        i = drive(port, 'card action choice=', s, 120)
        check('save.action.returns', i is not None, port.lines[i] if i is not None else 'no action end')
        # func_801CBD90 reports success with message 0x5C, failure with 0x35.
        check('save.write.succeeded', port.find('prompt msg=0x5c', s) is not None
              and port.find('prompt msg=0x35', s) is None)
        i = port.wait_for('card screen exit', 30, s)
        check('save.card.screen.exit', i is not None)
        s = port.n()
        port.wait_for('func_801C55A0 cancel/close', 30, s, every=3.0, action=lambda: port.pad(CROSS))
        s = port.n()
        time.sleep(25)
        loops = [int(x) for x in re.findall(r'fieldLoop=(\d+)', '\n'.join(port.lines[s:]))]
        check('save.field.loop.runs.after.menu', len(loops) >= 2 and loops[-1] > loops[0],
              'fieldLoop %s' % loops[-3:])
        check('save.field.not.stalled', port.find('GameHandleError', s) is None)
    finally:
        no_errors(port, 'save')
        port.stop()
    return saved


def load_run(binary, root, out, cards, saved):
    port = Port(binary, root, out, {'XENO_FIELD_POS_DIAG': '30', 'XENO_MEMCARD_DIR': cards})
    try:
        if not check('load.title.field',
                     port.wait_for('FieldLoad begin field=490', 300, every=1.0,
                                   action=lambda: port.pad(CROSS)) is not None):
            return
        i = port.wait_for('title loop enter', 300, every=1.5, action=lambda: port.pad(CIRCLE))
        if not check('load.title.menu', i is not None and 'choice=1' in port.lines[i],
                     port.lines[i] if i is not None else 'no title loop'):
            return
        # The Circle pulses that opened the title may already have confirmed
        # Continue (and Load): look from the title-loop line, press only
        # when the next step has not happened yet.
        s = i
        time.sleep(3)
        if port.find('card screen enter mode=1', s) is None:
            port.pad(CIRCLE)
        i = port.wait_for('card screen enter mode=1', 30, s)
        if not check('load.continue.opens.card.screen', i is not None):
            return
        time.sleep(3)
        if port.find('card action begin choice=', s) is None:
            port.pad(CIRCLE)
        i = port.wait_for('card action begin choice=', 20, s)
        if not check('load.action.is.load', i is not None and 'choice=2' in port.lines[i]
                     and 'D_80059460=2' in port.lines[i],
                     port.lines[i] if i is not None else 'no action'):
            return
        time.sleep(3)
        port.pad(CIRCLE)  # pick the first slot
        i = drive(port, 'card action choice=', s, 120)
        check('load.action.handled', i is not None and 'handled=1' in port.lines[i],
              port.lines[i] if i is not None else 'no action end')
        # func_801CB304 reports success with message 0x5C, failure with 0x3E.
        check('load.read.succeeded', port.find('prompt msg=0x5c', s) is not None
              and port.find('prompt msg=0x3e', s) is None)
        i = port.wait_for('title confirm choice=1', 60, s)
        check('load.continue.leaves.title', i is not None and 'keep=0 D_800594D0=2' in port.lines[i],
              port.lines[i] if i is not None else 'no title confirm')
        m0, hp0, line0 = saved
        want = int(m0.group(1)) if m0 else -1
        s2 = port.n()
        i = port.wait_for('FieldLoad begin field=%d' % want, 120, s)
        if not check('load.saved.field.loads', i is not None, 'field=%d' % want):
            return
        port.wait_for('POSDIAG', 60, s2)
        time.sleep(8)
        m1, line1 = last_match(port, POSDIAG, s2)
        hp1 = partyhp(port, s2)
        print('saved : %s\nloaded: %s' % (line0, line1), flush=True)
        print('saved PARTYHP %s / loaded PARTYHP %s' % (hp0, hp1), flush=True)
        if check('load.posdiag.present', m1 is not None and m0 is not None):
            check('load.map.matches', m1.group(1) == m0.group(1))
            check('load.scenario.matches', m1.group(5) == m0.group(5))
        check('load.partyhp.matches', hp0 is not None and hp0 == hp1)
        return m1
    finally:
        no_errors(port, 'load')
        port.stop()


def entrance_run(binary, root, out, ckpt, field, entrance):
    """Where the retail map places the player for this entrance."""
    os.makedirs(out, exist_ok=True)
    qs = os.path.join(out, 'quick.xgqs')
    shutil.copy(ckpt, qs)
    port = Port(binary, root, out, {'XENO_QUICKSAVE_PATH': qs, 'XENO_FIELD_POS_DIAG': '30',
                                    'XENO_DEV_MENU': '1', 'XENO_DEV_MENU_UNVERIFIED': '1',
                                    'XENO_MEMCARD_DIR': os.path.join(out, 'memcards')})
    try:
        if port.load_checkpoint() is None:
            return None
        time.sleep(4)
        s = port.n()
        port.con('dev warpx %d %d -1' % (field, entrance))
        if port.wait_for('FieldLoad begin field=%d' % field, 90, s) is None:
            return None
        s2 = port.n()
        port.wait_for('POSDIAG', 60, s2)
        time.sleep(8)
        m, line = last_match(port, POSDIAG, s2)
        print('entrance %d of field %d: %s' % (entrance, field, line), flush=True)
        return m
    finally:
        port.stop()


def saved_entrance(cards):
    """Script var 0x54 (g_GameState+0x1984) from the save payload: the payload
    at file +0x100 holds g_GameState+0x1920.. at +0x1124 (func_801E4A28)."""
    for f in sorted(os.listdir(cards)):
        if f.endswith('.mcd'):
            errs, got = psx_card_image.read_card(os.path.join(cards, f))
            if got:
                d = got[0].data
                return struct.unpack_from('<H', d, 0x100 + 0x1124 + 0x1984 - 0x1920)[0]
    return None


def main():
    binary, out, ckpt = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
    root = os.getcwd()
    cards = os.path.join(out, 'memcards')
    os.makedirs(cards, exist_ok=True)
    saved = save_run(binary, root, os.path.join(out, 'save'), ckpt, cards)
    images = sorted(f for f in os.listdir(cards) if f.endswith('.mcd'))
    files = []
    for f in images:
        errs, got = psx_card_image.read_card(os.path.join(cards, f))
        check('card.%s.valid' % f, not errs, '; '.join(errs))
        for c in got:
            probs = psx_card_image.check_save_file(c)
            check('card.%s.header' % c.name, not probs, '; '.join(probs))
            check('card.%s.payload.checksum' % c.name, psx_card_image.xenogears_payload_ok(c))
        files += [(f, c.name, c.blocks) for c in got]
    print('card files: %s' % files, flush=True)
    check('card.holds.one.save', len(files) == 1, str(files))
    if saved is not None and files:
        m1 = load_run(binary, root, os.path.join(out, 'load'), cards, saved)
        entrance = saved_entrance(cards)
        if m1 is not None and entrance is not None:
            field = int(m1.group(1))
            ref = entrance_run(binary, root, os.path.join(out, 'entrance'), ckpt, field, entrance)
            if check('entrance.reference', ref is not None, 'field %d entrance %d' % (field, entrance)):
                d = max(abs(int(m1.group(k)) - int(ref.group(k))) for k in (2, 3, 4))
                check('load.spawns.at.saved.entrance', d <= POS_TOL,
                      'entrance %d, max delta %d' % (entrance, d))
    ok = results and all(results)
    print('CARD SAVE/LOAD overall=%s' % ('PASS' if ok else 'FAIL'), flush=True)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
