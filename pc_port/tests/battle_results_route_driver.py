#!/usr/bin/env python3
"""Headless battle route diagnostic driver.

Adapted from assists_test.py at ebbeb468; no assists implementation imported.
A returned battle does not establish victory, native execution or parity.

XENO_RESULTS_DIRECT_FIELD=1 selects the map-1 test bootstrap instead of a
checkpoint. Its previously observed empty-party RESULTS route is diagnostic
only, not a valid victory or useful playable-slice fixture.

Each run: boot -> quick-load the field-14 checkpoint ($XENO_ASSISTS_CKPT) ->
cheat console `battle <id>` -> the battle runs with battle-frame-keyed pad
presses (XENO_BATTLE_TEST_PAD, battle_state_hash.c) -> wait for battle.bin to
return -> stop.  battle_state_hash.c logs, per battle frame, a CRC of the
battle state and g_GameState, every damage row, the end-of-battle module call
and decoded guest stores into party records / gold. Decoded stores omit native
writes and are not a parity metric. battle_results_trace.c separately snapshots
bus-visible memory at RESULTS entry, renders and the observed return.

  battle_results_route_driver.py probe BINARY OUT NAME BATTLE_ID [ENV=VALUE ...]
  battle_results_route_driver.py summary STATE_LOG
"""
import os
import shutil
from pathlib import Path
import re
import subprocess
import sys
import threading
import time

PAD = os.environ.get('XENO_ASSISTS_PAD', '30:12:0x20,0x10,0x20,0x40,0x20,0x80')


class Port:
    def __init__(self, binary, out, env_extra):
        os.makedirs(out, exist_ok=True)
        out = os.path.abspath(out)
        self.out = out
        self.log = open(os.path.join(out, 'run.log'), 'w', buffering=1, errors='replace')
        env = dict(os.environ)
        for k in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY'):
            env.pop(k, None)
        env.update(SDL_VIDEODRIVER='offscreen', EGL_PLATFORM='surfaceless',
                   ALSOFT_DRIVERS='null', XENO_NO_DIALOG='1', XENO_MODS_ENABLED='0',
                   XENO_CHEATS_CONSOLE='1', XENO_DEV_MENU='1',
                   XENO_BATTLE_STATE_HASH_LOG=os.path.join(out, 'state.log'),
                   XENO_BATTLE_TEST_PAD=PAD)
        env.update(env_extra)
        direct_field = env_extra.get('XENO_RESULTS_DIRECT_FIELD') == '1'
        if direct_field:
            env.update(XENO_FIELD_TEST='1', XENO_KERNEL_SEL='0', XENO_FIELD_MAP='1')
        snapshots = os.path.join(out, 'results-snapshots')
        os.makedirs(snapshots, exist_ok=True)
        env['XENO_BATTLE_RESULTS_SNAPSHOT_DIR'] = snapshots
        # Test fixture files are private to this run, even if inherited
        # environment variables point at the owner's settings or saves.
        for k in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY'):
            env.pop(k, None)
        for key, leaf in (('HOME', 'home'), ('XDG_CONFIG_HOME', 'config'),
                          ('XDG_STATE_HOME', 'state'), ('XENO_MEMCARD_DIR', 'memcards')):
            env[key] = os.path.join(out, leaf)
            os.makedirs(env[key], exist_ok=True)
        private_checkpoint = os.path.join(out, 'checkpoint.xgqs')
        if not direct_field:
            checkpoint = os.path.abspath(os.environ['XENO_ASSISTS_CKPT'])
            if checkpoint == private_checkpoint:
                raise ValueError('checkpoint input must be outside the run output')
            shutil.copyfile(checkpoint, private_checkpoint)
        env['XENO_QUICKSAVE_PATH'] = private_checkpoint
        config = os.path.join(out, 'config.ini')
        Path(config).write_text('[enhancements]\n')
        env['XENO_CONFIG'] = config
        root = os.environ.get('XENO_ASSISTS_ROOT', os.getcwd())
        env.setdefault('XENO_DATA_DIR', os.path.join(root, 'disc'))
        env.setdefault('XENO_DISC', os.path.join(root, 'disc', 'disc1.bin'))
        self.t0 = time.time()
        self.proc = subprocess.Popen([os.path.abspath(binary)], cwd=out, env=env, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.lines = []
        self.lock = threading.Lock()
        self.reader = threading.Thread(target=self._reader, daemon=True)
        self.reader.start()

    def _reader(self):
        for raw in self.proc.stdout:
            line = raw.decode('latin1', 'replace').rstrip('\n')
            with self.lock:
                self.lines.append(line)
            self.log.write(f'{time.time() - self.t0:8.2f} {line}\n')

    def mark(self, text):
        self.log.write(f'{time.time() - self.t0:8.2f} [driver] {text}\n')

    def send(self, cmd):
        self.mark('stdin: ' + cmd)
        self.proc.stdin.write((cmd + '\n').encode())
        self.proc.stdin.flush()

    def n(self):
        with self.lock:
            return len(self.lines)

    def wait_for(self, needle, timeout, start=0):
        end = time.time() + timeout
        while time.time() < end:
            with self.lock:
                for i in range(start, len(self.lines)):
                    if needle in self.lines[i]:
                        return i
            if self.proc.poll() is not None:
                return None
            time.sleep(0.25)
        return None

    def stop(self):
        if self.log.closed:
            return self.proc.returncode
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(15)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        self.reader.join()
        self.mark('stop rc=%s' % self.proc.returncode)
        self.proc.stdin.close()
        self.proc.stdout.close()
        self.log.close()
        return self.proc.returncode


def run_battle(binary, out, battle_id, env_extra, battle_timeout=600):
    """Returns (status, detail); status 'ok' when battle.bin returned."""
    p = Port(binary, out, env_extra)
    try:
        if p.wait_for('[cheat] console reading commands', 180) is None:
            return 'fail', 'console not ready'
        if p.wait_for('FieldLoad begin field=', 300) is None:
            return 'fail', 'no field loaded'
        if env_extra.get('XENO_RESULTS_DIRECT_FIELD') != '1':
            for _ in range(60):
                n = p.n()
                p.send('load')
                if p.wait_for('[xeno-port][quick] loading', 5, n) is not None:
                    break
                time.sleep(5)
            else:
                return 'fail', 'load not accepted'
            if p.wait_for('FieldLoad begin field=14', 180) is None:
                return 'fail', 'field 14 not restored'
        time.sleep(5)
        n = p.n()
        p.send('battle %d' % battle_id)
        if p.wait_for('enter retail battle.bin', 120, n) is None:
            return 'fail', 'battle not entered'
        r = p.wait_for('retail battle returned', battle_timeout, n)
        if r is None:
            return 'timeout', 'battle did not return within %d s' % battle_timeout
        time.sleep(8)
        return 'ok', p.lines[r]
    finally:
        p.stop()


def parse(path):
    recs = []
    with open(path, errors='replace') as f:
        for line in f:
            parts = line.split()
            if parts:
                recs.append(parts)
    return recs


def summary(path):
    recs = parse(path)
    frames = [r for r in recs if r[0] == 'F']
    out = ['frames=%d' % len(frames)]
    if frames:
        out.append('first=' + ' '.join(frames[0][3:]))
        out.append('last=' + ' '.join(frames[-1][3:]))
    out.append('damage_rows=%d' % sum(1 for r in recs if r[0] == 'A'))
    for r in recs:
        if r[0] in ('A', 'R', 'W', 'B'):
            out.append(' '.join(r))
    return '\n'.join(out)


def main(argv):
    if len(argv) >= 6 and argv[1] == 'probe':
        binary, out, name, bid = argv[2], argv[3], argv[4], int(argv[5])
        env = dict(kv.split('=', 1) for kv in argv[6:])
        status, detail = run_battle(binary, os.path.join(out, name), bid, env,
                                    int(os.environ.get('XENO_ASSISTS_BATTLE_TIMEOUT', '600')))
        print('PROBE %s battle=%d status=%s %s' % (name, bid, status, detail))
        st = os.path.join(out, name, 'state.log')
        if os.path.exists(st):
            print(summary(st))
        return 0 if status == 'ok' else 1
    if len(argv) == 3 and argv[1] == 'summary':
        print(summary(argv[2]))
        return 0
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
