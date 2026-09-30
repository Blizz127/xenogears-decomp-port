#!/usr/bin/env python3
"""Port Dev Menu warp audit on the real disc (headless).

Boots with the title/New Game smoke schedule (run_title_newgame_smoke.sh) to
Lahan (map 2), then drives the dev menu's own action path through the stdin
cheat console ("dev warp N", the command the menu queues) along a chain of
destinations.  A destination PASSES only when, after the warp request,
  1. the menu logs the request ("[dev-menu] loading field N"),
  2. the field loader starts that map ("FieldLoad begin field=N"), and
  3. a later "dev heal" is accepted, which the menu only allows once
     PcPort_QuickCheckpointFieldIsSafe() holds: field active, player actor
     present, no script lock (quick_checkpoint.c checkpoint_is_safe) -- i.e.
     the player is free to move again on the new map;
and the port is still running.  Leaving each map by the next warp in the
chain checks that a warped-in field tears down cleanly.

  run_port_dev_menu_warp_audit.py [--maps 1,15,16,2] [--outdir DIR]

Needs disc/ (the user's files) and pc_port/build_native/xeno-port.  Prints
one PASS/FAIL line per destination and exits 0 only if all pass.
"""
import argparse
import os
import subprocess
import sys
import threading
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def smoke_schedule():
    # Same defaults as run_title_newgame_smoke.sh (Vsync-shim frames).
    sched = ["0:0", "300:0x40", "320:0"]
    for f, b in ((3700, 0x20), (4500, 0x20), (4900, 0x1000), (5000, 0x20)):
        sched += [f"{f}:{b:#x}", f"{f + 2}:0"]
    frame, steps = 5500, 11
    while steps + 2 <= 4096:
        sched += [f"{frame}:0x20", f"{frame + 2}:0"]
        frame += 4
        steps += 2
    return ",".join(sched)


class Log:
    def __init__(self, proc, path):
        self.lines = []
        self.lock = threading.Lock()
        self.out = open(path, "w", encoding="utf-8", errors="replace")
        threading.Thread(target=self._pump, args=(proc.stdout,), daemon=True).start()

    def _pump(self, stream):
        for raw in iter(stream.readline, b""):
            line = raw.decode("utf-8", "replace")
            self.out.write(line)
            with self.lock:
                self.lines.append(line)

    def wait_for(self, needle, start, timeout):
        """Index of the first line at or after `start` containing `needle`."""
        end = time.time() + timeout
        while time.time() < end:
            with self.lock:
                for i in range(start, len(self.lines)):
                    if needle in self.lines[i]:
                        return i
            time.sleep(0.2)
        return -1

    def mark(self):
        with self.lock:
            return len(self.lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--maps", default="1,15,16,2")
    ap.add_argument("--outdir", default=os.path.join(ROOT, "scratchpad", "dev_menu_warp_audit",
                                                     time.strftime("%Y%m%d-%H%M%S")))
    ap.add_argument("--binary", default=os.path.join(ROOT, "pc_port", "build_native", "xeno-port"))
    args = ap.parse_args()
    os.makedirs(os.path.join(args.outdir, "captures"), exist_ok=True)
    env = dict(os.environ,
               SDL_VIDEODRIVER=os.environ.get("SDL_VIDEODRIVER", "offscreen"),
               XENO_PAD_TEST_INPUT=smoke_schedule(),
               XENO_CHEATS_DEV_MENU="1",
               XENO_CHEATS_CONSOLE="1",
               XENO_NO_DIALOG="1",
               XENO_FIELD_CAPTURE_DIR=os.path.join(args.outdir, "captures"),
               XENO_FIELD_CAPTURE_EVERY="300")
    proc = subprocess.Popen([args.binary], cwd=ROOT, env=env, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = Log(proc, os.path.join(args.outdir, "run.log"))

    def send(cmd):
        proc.stdin.write((cmd + "\n").encode())
        proc.stdin.flush()

    def free_control(start, timeout):
        """Poll 'dev heal' until the menu accepts it (field safe)."""
        end = time.time() + timeout
        while time.time() < end and proc.poll() is None:
            m = log.mark()
            send("dev heal")
            if log.wait_for("[dev-menu] heal:", m, 3) >= 0:
                return True
        return False

    results = []
    try:
        title = log.wait_for("title confirm choice=2", 0, 240)
        lahan = log.wait_for("FieldLoad begin field=2", max(title, 0), 240) if title >= 0 else -1
        if lahan < 0 or not free_control(lahan, 240):
            print("DEV MENU WARP FAIL setup: never reached free control on map 2 "
                  f"(title={title >= 0} map2={lahan >= 0})")
            return 1
        print("DEV MENU WARP setup: free control on map 2")
        for m in (int(x) for x in args.maps.split(",")):
            start = log.mark()
            send(f"dev warp {m}")
            req = log.wait_for(f"[dev-menu] loading field {m} ", start, 30)
            load = log.wait_for(f"FieldLoad begin field={m}", max(req, start), 90) if req >= 0 else -1
            ok = load >= 0 and free_control(load, 120) and proc.poll() is None
            results.append((m, ok))
            print(f"DEV MENU WARP {'PASS' if ok else 'FAIL'} map={m} "
                  f"request={req >= 0} load={load >= 0} running={proc.poll() is None}")
            if proc.poll() is not None:
                break
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(15)
            except subprocess.TimeoutExpired:
                proc.kill()
    print(f"DEV MENU WARP outdir={args.outdir}")
    return 0 if results and all(ok for _, ok in results) else 1


if __name__ == "__main__":
    sys.exit(main())
