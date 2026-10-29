#!/usr/bin/env python3
"""Runtime acceptance: START+SELECT+L1+R1 in the field restarts the port.

  soft_reset_test.py BINARY OUTDIR CHECKPOINT.xgqs

Retail GameCheckAndHandleSoftReset (src/slus_006.64/main/main_loop.c) calls
GameSoftReset when exactly START+SELECT+L1+R1 are held (CTRL_BTN_* 0x090C,
include/system/controller.h). GameSoftReset tears the subsystems down and calls
crt0 `start`, which boots to the title again. The port's start
(pc_port/src/port_soft_reset.c) leaves through the normal quit path and
re-execs /proc/self/exe with the argv saved at boot.

The driver:
  1. boots the port headless (lib/xeno_headless.py);
  2. quick-loads CHECKPOINT, a field checkpoint;
  3. holds 0x090C with the TEST TOOLING console command `pad`;
  4. waits for the restarted image to reach the title menu.

PASS needs all of:
  - the soft-reset line names the driver's child PID;
  - the process never exits, and the same PID prints the boot lines again
    after that line ("armed", "ArchiveInit done", "FieldLoad begin field=490");
  - both boots report the same inherited descriptors, so nothing the first
    image opened leaked into the second;
  - the restarted image reaches the title menu ("title loop enter");
  - the process name (/proc/PID/comm) is unchanged;
  - no GameHandleError and no crash marker;
  - no [stub] other than CdFlush. CdFlush is left a stub on purpose:
    GameSoftReset calls it right before start, and a port definition of a
    libcd name belongs in the guarded compat layer.
"""
import os
import re
import shutil
import signal
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib'))
from xeno_headless import Port, CIRCLE, CROSS, START  # noqa: E402

SELECT, L1, R1 = 0x100, 0x4, 0x8
RESET_MASK = START | SELECT | L1 | R1          # 0x090C
ARMED = re.compile(r'\[xeno-port\]\[soft-reset\] armed: pid=(\d+) inherited fds=(\S+)')
RESTART = re.compile(r'\[xeno-port\]\[soft-reset\] START\+SELECT\+L1\+R1: restarting pid=(\d+)')
CRASH = ('Segmentation fault', 'Aborted', 'core dumped', 'Assertion', 'runtime error:',
         'AddressSanitizer', 'execv(/proc/self/exe) failed', 'restart unavailable')
DEADLINE = 540                                 # below the runner's 600 s cap

results = []


def check(name, ok, detail=''):
    results.append(ok)
    print('SOFT RESET %s %s%s' % ('PASS' if ok else 'FAIL', name,
                                 (' (%s)' % detail) if detail else ''), flush=True)
    return ok


def line(port, i):
    with port.lock:
        return port.lines[i]


def find_re(port, pat, start=0):
    with port.lock:
        for i in range(start, len(port.lines)):
            m = pat.search(port.lines[i])
            if m:
                return i, m
    return None, None


def wait_re(port, pat, timeout, start=0):
    deadline = time.time() + timeout
    while time.time() < deadline and port.proc.poll() is None:
        i, m = find_re(port, pat, start)
        if i is not None:
            return i, m
        time.sleep(0.1)
    return find_re(port, pat, start)


def comm(pid):
    try:
        with open('/proc/%d/comm' % pid) as f:
            return f.read().strip()
    except OSError:
        return None


def run(binary, root, out, ckpt):
    t0 = time.time()
    left = lambda: max(1, DEADLINE - (time.time() - t0))  # noqa: E731
    qs = os.path.join(out, 'quick.xgqs')
    shutil.copy(ckpt, qs)
    port = Port(binary, root, out, {'XENO_QUICKSAVE_PATH': qs})
    pid = port.proc.pid
    try:
        a1, m1 = wait_re(port, ARMED, min(60, left()))
        if not check('boot1.armed.same.pid', m1 is not None and int(m1.group(1)) == pid,
                     line(port, a1) if a1 is not None else 'no armed line'):
            return
        fds1, comm1 = m1.group(2), comm(pid)
        if not check('boot1.checkpoint.loaded', port.load_checkpoint(min(300, left())) is not None):
            return
        time.sleep(5)
        check('no.restart.before.combo', find_re(port, RESTART)[0] is None)
        s = port.n()
        port.pad(RESET_MASK, 8)
        r, mr = wait_re(port, RESTART, min(30, left()), s)
        if not check('restart.line.names.child.pid', mr is not None and int(mr.group(1)) == pid,
                     line(port, r) if r is not None else 'no restart line'):
            return
        a2, m2 = wait_re(port, ARMED, min(60, left()), r)
        if not check('boot2.armed.same.pid', m2 is not None and int(m2.group(1)) == pid
                     and port.proc.poll() is None,
                     line(port, a2) if a2 is not None else 'no second armed line'):
            return
        check('boot2.same.inherited.fds', m2.group(2) == fds1, 'boot1 %s, boot2 %s' % (fds1, m2.group(2)))
        i = port.wait_for('[xeno-port] ArchiveInit done', min(120, left()), r)
        check('boot2.archive.init', i is not None)
        t490 = port.wait_for('FieldLoad begin field=490', min(300, left()), r, every=1.0,
                             action=lambda: port.pad(CROSS))
        if not check('boot2.title.field.490', t490 is not None):
            return
        i = port.wait_for('title loop enter', min(300, left()), t490, every=1.5,
                          action=lambda: port.pad(CIRCLE))
        check('boot2.title.menu', i is not None, line(port, i) if i is not None else 'no title loop')
        comm2 = comm(pid)
        check('process.name.unchanged', comm1 is not None and comm1 == comm2,
              'boot1 %r, boot2 %r' % (comm1, comm2))
        time.sleep(3)
        check('process.alive.same.pid', port.proc.poll() is None, 'rc=%r' % port.proc.poll())
        print('soft reset: restart at line %d, boot2 armed at %d, title at %r, %.0f s total'
              % (r, a2, i, time.time() - t0), flush=True)
    finally:
        with port.lock:
            text = '\n'.join(port.lines)
        check('no.game.handle.error', 'GameHandleError' not in text)
        crash = [c for c in CRASH if c in text]
        check('no.crash.marker', not crash, ', '.join(crash))
        stubs = sorted(set(l for l in text.split('\n') if '[stub]' in l))
        check('no.stub.but.cdflush', all(l.strip().endswith('[stub] CdFlush') for l in stubs),
              '; '.join(stubs))
        port.stop()


def main():
    binary, out, ckpt = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), sys.argv[3]
    os.makedirs(out, exist_ok=True)
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(124))  # outer timeout: still stop the port
    run(binary, os.getcwd(), out, ckpt)
    ok = results and all(results)
    print('SOFT RESET overall=%s' % ('PASS' if ok else 'FAIL'), flush=True)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
