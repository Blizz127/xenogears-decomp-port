#!/usr/bin/env python3
"""Runtime regression: closing a menu must not show VRAM garbage.

Boots the port headless (offscreen GL, dummy audio, private XDG dirs), opens
the title menu, picks New Game and watches every presented frame from the
moment MenuMain returns until the New Game FE60 STR delivers its first frame.

Retail func_800799D4 copies the menu's last frame (0,0xE0) to (0x140,0) and
from there into both field frame buffers, so the screen holds that frame and
then goes black.  The r6.3 port copied it to (0,0) instead and then spread
the texture pages at (0x140,0) over both frame buffers: ~6 frames of VRAM
garbage (visible right before the New Game FMV and on every field-menu
close).

Evidence is the port's XENO_PRESENT_LOG line per presented frame (hash and
non-black pixel count of the finished back buffer).  Verdict:
  * at least one frame is presented in the window (the check ran);
  * every frame there is black or shows no more non-black pixels than the
    menu frames just before the return (garbage texture pages are ~3x
    denser: 120208 vs 43892 on the title);
  * no more than two distinct non-black pictures (the held frame and at most
    one intermediate copy) appear.

Usage: menu_close_present_test.py BINARY ROOT OUTDIR
"""
import os
import re
import subprocess
import sys
import threading
import time

CIRCLE, CROSS, UP = 0x20, 0x40, 0x1000
PRESENT = re.compile(r'^\[present\] n=(\d+) hash=([0-9a-f]+) nonblack=(\d+)/(\d+)')


def main():
    binary, root, out = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(out, exist_ok=True)
    env = {k: v for k, v in os.environ.items()
           if k not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY')}
    for d in ('config', 'data', 'cache', 'state', 'run'):
        os.makedirs(os.path.join(out, 'xdg', d), exist_ok=True)
    os.chmod(os.path.join(out, 'xdg', 'run'), 0o700)
    env.update(SDL_VIDEODRIVER='offscreen', SDL_AUDIODRIVER='dummy', ALSOFT_DRIVERS='null',
               XENO_NO_DIALOG='1', XENO_CHEATS_CONSOLE='1', XENO_PRESENT_LOG='1',
               XDG_CONFIG_HOME=out + '/xdg/config', XDG_DATA_HOME=out + '/xdg/data',
               XDG_CACHE_HOME=out + '/xdg/cache', XDG_STATE_HOME=out + '/xdg/state',
               XDG_RUNTIME_DIR=out + '/xdg/run', XENO_MEMCARD_DIR=out + '/xdg/data/memcards')
    log = open(os.path.join(out, 'run.log'), 'w', errors='replace')
    lines, lock = [], threading.Lock()
    proc = subprocess.Popen([binary], cwd=root, env=env, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def reader():
        for raw in proc.stdout:
            line = raw.decode('latin1', 'replace').rstrip('\n')
            with lock:
                lines.append(line)
            log.write(line + '\n')
    threading.Thread(target=reader, daemon=True).start()

    def pad(mask, frames=3):
        try:
            proc.stdin.write(('pad 0x%x %d\n' % (mask, frames)).encode())
            proc.stdin.flush()
        except BrokenPipeError:
            pass

    def find(needle, start=0):
        with lock:
            for i in range(start, len(lines)):
                if needle in lines[i]:
                    return i
        return None

    def wait_for(needle, timeout, start=0, every=None, action=None):
        end, last = time.time() + timeout, 0.0
        while time.time() < end and proc.poll() is None:
            i = find(needle, start)
            if i is not None:
                return i
            if action and time.time() - last >= every:
                action()
                last = time.time()
            time.sleep(0.1)
        return find(needle, start)

    verdict = 'NOT_RUN'
    try:
        if wait_for('FieldLoad begin field=490', 300, every=1.0, action=lambda: pad(CROSS)) is None:
            verdict = 'NOT_RUN title field not reached'
            return 2
        if wait_for('title loop enter', 300, every=1.5, action=lambda: pad(CIRCLE)) is None:
            verdict = 'NOT_RUN title menu not reached'
            return 2
        time.sleep(1.5)
        start = len(lines)
        pad(UP)
        time.sleep(1.0)
        if wait_for('title confirm choice=2', 30, start, every=2.0, action=lambda: pad(CIRCLE)) is None:
            verdict = 'NOT_RUN New Game not confirmed'
            return 2
        ret = wait_for('MenuMain returned', 120, start)
        first = wait_for('fe60] first STR frame', 180, start)
        if ret is None or first is None:
            verdict = 'NOT_RUN FE60 STR not reached'
            return 2
        time.sleep(1.0)
        verdict = 'RAN'
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(15)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        time.sleep(0.3)
        log.close()
        if verdict.startswith('NOT_RUN'):
            print('MENU CLOSE PRESENT overall=%s' % verdict)

    with lock:
        before = [PRESENT.match(l) for l in lines[:ret]]
        window = [PRESENT.match(l) for l in lines[ret:first]]
    before = [m for m in before if m][-10:]
    window = [m for m in window if m]
    menu_max = max((int(m.group(3)) for m in before), default=0)
    fails = []
    if not window:
        fails.append('no frame presented between MenuMain return and the STR')
    pictures = set()
    for m in window:
        nonblack = int(m.group(3))
        if nonblack == 0:
            continue
        pictures.add(m.group(2))
        if nonblack > menu_max + menu_max // 20:
            fails.append('frame n=%s nonblack=%d > menu frame %d (VRAM garbage)'
                         % (m.group(1), nonblack, menu_max))
    if len(pictures) > 2:
        fails.append('%d distinct non-black pictures after the close' % len(pictures))
    print('MENU CLOSE PRESENT frames=%d menu_nonblack=%d pictures=%d'
          % (len(window), menu_max, len(pictures)))
    for f in fails:
        print('MENU CLOSE PRESENT FAIL %s' % f)
    print('MENU CLOSE PRESENT overall=%s' % ('FAIL' if fails else 'PASS'))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
