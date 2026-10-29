#!/usr/bin/env python3
"""Drive the port headless into a quicksave for run_fei_hd2d_footprint_test.sh.

usage: fei_hd2d_footprint_drive.py BINARY QUICKSAVE OUTDIR CAPTURES TIMEOUT_S

Boots to the title, sends the cheat console `load` until the quicksave is
accepted, then lets the Fei test cycle (XENO_FEI_HD2D_TEST_CYCLE=1) capture
CAPTURES presents (fei-frame-<present>.png in OUTDIR/cap) and exits.
"""
import os
import subprocess
import sys
import threading
import time


def main():
    binary, qs, out, captures, timeout = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), float(sys.argv[5])
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    cap = os.path.join(out, 'cap')
    os.makedirs(cap, exist_ok=True)
    env = dict(os.environ)
    env.update(SDL_VIDEODRIVER=env.get('SDL_VIDEODRIVER', 'offscreen'), SDL_AUDIODRIVER='dummy',
               ALSOFT_DRIVERS='null', XENO_CHEATS_CONSOLE='1', XENO_QUICKSAVE_PATH=qs,
               XENO_FIELD_CAPTURE_DIR=cap, XENO_FIELD_CAPTURE_EVERY='100000000',
               XENO_FEI_HD2D_TEST_CYCLE='1', XENO_FEI_HD2D_TEST_CAPTURES=str(captures),
               XENO_FEI_HD2D_PART_LOG=os.path.join(out, 'parts.log'))
    proc = subprocess.Popen([binary], cwd=root, env=env, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    lines = []
    log = open(os.path.join(out, 'run.log'), 'w', errors='replace')

    def reader():
        for raw in proc.stdout:
            text = raw.decode('latin1').rstrip('\n')
            lines.append(text)
            log.write(text + '\n')
            log.flush()
    threading.Thread(target=reader, daemon=True).start()
    t0 = time.time()
    state = 'boot'
    rc = 1
    while time.time() - t0 < timeout and proc.poll() is None:
        seen = list(lines)
        if state == 'boot' and any('console reading commands' in l for l in seen) and \
                any('FieldLoad begin' in l for l in seen):
            state = 'load'
        if state == 'load':
            if any('[xeno-port][quick] loading' in l for l in seen):
                state = 'field'
                print(f'[drive] quicksave accepted at {time.time() - t0:.1f}s', flush=True)
            else:
                proc.stdin.write(b'load\n')
                proc.stdin.flush()
                time.sleep(3)
                continue
        if state == 'field':
            n = len([f for f in os.listdir(cap) if f.startswith('fei-frame-')])
            if n >= captures:
                print(f'[drive] {n} captures at {time.time() - t0:.1f}s', flush=True)
                rc = 0
                break
        time.sleep(0.5)
    if rc:
        print(f'[drive] timeout/exit in state {state}', flush=True)
    proc.terminate()
    try:
        proc.wait(15)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
    time.sleep(0.5)
    log.close()
    return rc


if __name__ == '__main__':
    sys.exit(main())
