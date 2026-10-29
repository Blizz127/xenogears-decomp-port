"""Headless driver for runtime tests of the native port.

Runs the port offscreen (SDL offscreen GL, dummy audio, private XDG dirs,
never a window) with the cheat console on stdin, and presses GAME buttons
(CTRL_BTN_* bits, post-remap) through the console's TEST TOOLING
`pad <mask> <frames>` command.  Tests wait on the port's own log lines.
"""
import os
import subprocess
import threading
import time

CIRCLE, CROSS, TRIANGLE, SQUARE, START = 0x20, 0x40, 0x10, 0x80, 0x800
UP, RIGHT, DOWN, LEFT = 0x1000, 0x2000, 0x4000, 0x8000


class Port:
    def __init__(self, binary, root, out, env_extra=None, drop=()):
        os.makedirs(out, exist_ok=True)
        env = {k: v for k, v in os.environ.items()
               if k not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY')}
        for d in ('config', 'data', 'cache', 'state', 'run'):
            os.makedirs(os.path.join(out, 'xdg', d), exist_ok=True)
        os.chmod(os.path.join(out, 'xdg', 'run'), 0o700)
        env.update(SDL_VIDEODRIVER='offscreen', SDL_AUDIODRIVER='dummy',
                   ALSOFT_DRIVERS='null', XENO_NO_DIALOG='1', XENO_CHEATS_CONSOLE='1',
                   XDG_CONFIG_HOME=out + '/xdg/config', XDG_DATA_HOME=out + '/xdg/data',
                   XDG_CACHE_HOME=out + '/xdg/cache', XDG_STATE_HOME=out + '/xdg/state',
                   XDG_RUNTIME_DIR=out + '/xdg/run',
                   XENO_MEMCARD_DIR=out + '/xdg/data/memcards')
        env.update(env_extra or {})
        self.drop = tuple(drop)
        self.lines = []
        self.lock = threading.Lock()
        self.log = open(os.path.join(out, 'run.log'), 'w', errors='replace')
        self.proc = subprocess.Popen([binary], cwd=root, env=env, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        for raw in self.proc.stdout:
            line = raw.decode('latin1', 'replace').rstrip('\n')
            if line.startswith('[ALSOFT]') or any(d in line for d in self.drop):
                continue
            with self.lock:
                self.lines.append(line)
            self.log.write(line + '\n')

    def con(self, cmd):
        with self.lock:
            self.lines.append('[test-driver] stdin: ' + cmd)
        try:
            self.proc.stdin.write((cmd + '\n').encode())
            self.proc.stdin.flush()
        except BrokenPipeError:
            pass

    def pad(self, mask, frames=3):
        self.con('pad 0x%x %d' % (mask, frames))

    def n(self):
        with self.lock:
            return len(self.lines)

    def find(self, needle, start=0, end=None):
        with self.lock:
            for i in range(start, len(self.lines) if end is None else min(end, len(self.lines))):
                if needle in self.lines[i]:
                    return i
        return None

    def wait_for(self, needle, timeout, start=0, every=None, action=None):
        deadline, last = time.time() + timeout, 0.0
        while time.time() < deadline and self.proc.poll() is None:
            i = self.find(needle, start)
            if i is not None:
                return i
            if action and time.time() - last >= every:
                action()
                last = time.time()
            time.sleep(0.1)
        return self.find(needle, start)

    def stop(self):
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(15)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        time.sleep(0.3)
        self.log.close()

    def load_checkpoint(self, timeout=300):
        """Boot to the title field and quick-load XENO_QUICKSAVE_PATH."""
        if self.wait_for('FieldLoad begin field=490', timeout, every=1.0,
                         action=lambda: self.pad(CROSS)) is None:
            return None
        start = self.n()
        if self.wait_for('[xeno-port][quick] loading', 120, start, every=5.0,
                         action=lambda: self.con('load')) is None:
            return None
        return self.wait_for('player position restored', 120, start)
