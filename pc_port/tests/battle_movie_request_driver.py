#!/usr/bin/env python3
"""TEST TOOLING: drive the post-battle movie request (run_battle_movie_request_test.sh).

boot -> quick-load the field-14 checkpoint -> set the boot movie's frames
aside -> `battle 1` (the LD_PRELOAD probe stores the movie request and the
D_800D3338 flag through the battle bus once the outcome is decided) -> battle.bin
returns (the probe sets a non-win result byte to 1, as event battles leave
it) -> the battle exit chooses state 6 -> wait until the probe logs the
return to state 1 after the movie and the field loads again, then a few more
seconds -> stop.  Fails on an abort, a stopped battle or a port exit, and
when MovieMain did not play the request: type FE44 & 0x7F, movie number FE45
and, through the bit-7 branch, the end frame 0x80062514 (MRP_REQUEST).

usage: battle_movie_request_driver.py BINARY OUT PROBE_LOG TIMEOUT [ENV=VALUE ...]
       battle_movie_request_driver.py --check PROBE_LOG TYPE:NUMBER:END:NOSKIP
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import battle_results_route_driver as route  # noqa: E402


def movie_verdict(probe_log, request):
    """('ok'|'fail', detail) from the probe's return line against the request."""
    want_type, want_number, want_end = (int(x) for x in request.split(':')[:3])
    want = (want_type & 0x7F, want_number, want_end)
    with open(probe_log, errors='replace') as f:
        for line in f:
            m = re.search(r' R returned to field state 1 after the movie: MovieMain '
                          r'type=(-?\d+) number=(-?\d+) end_frame=(-?\d+)', line)
            if m:
                got = tuple(int(x) for x in m.groups())
                text = 'MovieMain type=%d number=%d end_frame=%d' % got
                if got != want:
                    return 'fail', '%s, requested type=%d number=%d end_frame=%d' % ((text,) + want)
                return 'ok', text
    return 'fail', 'no return to the field after the movie'


def main():
    if len(sys.argv) == 4 and sys.argv[1] == '--check':
        return report(*movie_verdict(sys.argv[2], sys.argv[3]))
    binary, out, probe_log, timeout = sys.argv[1:5]
    env = dict(a.split('=', 1) for a in sys.argv[5:])
    p = route.Port(binary, out, env)
    try:
        if p.wait_for('[cheat] console reading commands', 180) is None:
            return report('fail', 'console not ready')
        if p.wait_for('FieldLoad begin field=', 300) is None:
            return report('fail', 'no field loaded')
        for _ in range(60):
            n = p.n()
            p.send('load')
            if p.wait_for('[xeno-port][quick] loading', 5, n) is not None:
                break
            time.sleep(5)
        else:
            return report('fail', 'load not accepted')
        if p.wait_for('FieldLoad begin field=14', 180) is None:
            return report('fail', 'field 14 not restored')
        time.sleep(5)
        frames = os.path.join(os.path.dirname(os.path.abspath(probe_log)), 'movie-frames')
        boot = os.path.join(frames, 'boot')
        os.makedirs(boot, exist_ok=True)
        for name in os.listdir(frames):
            if name.endswith('.png'):
                os.replace(os.path.join(frames, name), os.path.join(boot, name))
        n = p.n()
        p.send('battle 1')
        if p.wait_for('enter retail battle.bin', 120, n) is None:
            return report('fail', 'battle not entered')
        returned = p.wait_for('retail battle returned', 900, n)
        if returned is None:
            return report('fail', 'battle did not return')
        end = time.time() + float(timeout)
        while time.time() < end:
            if p.proc.poll() is not None:
                return report('fail', 'port exited (rc %s)' % p.proc.returncode)
            with open(probe_log, errors='replace') as f:
                if any(' R returned to field state 1' in line for line in f):
                    break
            time.sleep(2)
        else:
            return report('timeout', 'no return to the field within %s s' % timeout)
        if p.wait_for('FieldLoad begin field=', 120, returned) is None:
            return report('fail', 'no field load after the movie')
        time.sleep(10)
        if p.proc.poll() is not None:
            return report('fail', 'port exited (rc %s)' % p.proc.returncode)
        status, detail = movie_verdict(probe_log, env.get('MRP_REQUEST', ''))
        return report(status, detail + ('; back in the field, no abort' if status == 'ok' else ''))
    finally:
        p.stop()


def report(status, detail):
    print('MOVIE REQUEST %s %s' % (status, detail))
    return 0 if status == 'ok' else 1


if __name__ == '__main__':
    sys.exit(main())
