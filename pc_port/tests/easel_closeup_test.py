#!/usr/bin/env python3
"""Runtime check: Fei's easel close-up (Lahan, field 14) leaves on confirm only.

Retail behaviour, from the field-14 script as the port runs it (actor 11, the
easel; the ips are offsets into the disc's field-14 script):
  * confirm on the easel runs its handler: FE54 locks the player (ip 1053),
    the camera cuts to the close-up and the script calls a wait at ip 2601;
  * ip 2601 is opcode 0x31 (func_800961A0): "if held buttons (D_800AFE9C) &
    0x0020 continue, else loop" -- 0x0020 is the game's confirm (physical
    Cross, keyboard Z, on the USA layout).  Nothing else is tested, so cancel
    (physical Circle, keyboard C), Start, Triangle, Square or the d-pad do
    not leave the close-up on retail either;
  * confirm continues at ip 2606 -> return, and FE53 at ip 1192 unlocks.

The r6.3 report "Z/C never leaves it" came from the video run's input log:
the close-up was open while the driver pressed Left/Down/Right, X
(Triangle), V (Square), Return (Start) and Down; the first Z afterwards
closed it.  This test pins that behaviour: cancel must NOT exit, confirm
must.

Needs a quick checkpoint taken in field 14 after the painting scene, with Fei
at the easel (the position the scene leaves him at), passed in
XENO_TEST_FIELD14_CHECKPOINT.  Checkpoints hold game state from the user's
disc, so none is in the repository; any field-14 quick save made with F11 (or
the console `save`) right after the scene works.

Usage: easel_closeup_test.py BINARY ROOT OUTDIR CHECKPOINT
"""
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib'))
from xeno_headless import CIRCLE, CROSS, Port  # noqa: E402

TAG = 'actor=11 ip='


def main():
    binary, root, out, checkpoint = sys.argv[1:5]
    os.makedirs(out, exist_ok=True)
    qs = os.path.join(out, 'field14.xgqs')
    shutil.copyfile(checkpoint, qs)   # the run may overwrite its quick save
    port = Port(binary, root, out, {
        'XENO_QUICKSAVE_PATH': qs,
        'XENO_VM_TRACE': '11', 'XENO_VM_TRACE_FIELD': '14',
        'XENO_VM_TRACE_REPEAT': '1', 'XENO_VM_TRACE_MAX': '200000',
    })
    fails, verdict = [], None
    try:
        if port.load_checkpoint() is None:
            verdict = 'NOT_RUN checkpoint did not load'
            return 2
        time.sleep(8)
        # 1. confirm on the easel opens the close-up and reaches the wait.
        s = port.n()
        port.pad(CIRCLE, 3)
        if port.wait_for(TAG + '1053 op=FE54', 30, s) is None:
            verdict = 'NOT_RUN confirm did not start the easel (is Fei at the easel?)'
            return 2
        if port.wait_for(TAG + '2601 op=31', 30, s) is None:
            fails.append('close-up never reached its confirm wait (ip 2601)')
        time.sleep(3)
        # 2. cancel does not leave it.
        s = port.n()
        port.pad(CROSS, 6)
        time.sleep(4)
        if port.find(TAG + '2606 ', s) is not None:
            fails.append('cancel left the close-up (retail waits for confirm only)')
        if port.find(TAG + '2601 op=31', s) is None:
            fails.append('close-up stopped waiting after cancel')
        # 3. confirm leaves it and the handler unlocks the player.
        s = port.n()
        port.pad(CIRCLE, 3)
        if port.wait_for(TAG + '2606 ', 20, s) is None:
            fails.append('confirm did not leave the close-up')
        elif port.wait_for(TAG + '1192 op=FE53', 20, s) is None:
            fails.append('easel handler did not unlock the player (FE53)')
    finally:
        port.stop()
        if verdict:
            print('EASEL CLOSEUP overall=%s' % verdict)
    for f in fails:
        print('EASEL CLOSEUP FAIL %s' % f)
    print('EASEL CLOSEUP overall=%s' % ('FAIL' if fails else 'PASS'))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
