#!/usr/bin/env python3
"""Headless actual-field composition timings; requires an approved local checkpoint.

No state or disc is copied. The caller supplies the native instrumented binary,
existing checkpoint, existing retail data directory and isolated output folder.
Raw logs stay on the runner; only metrics are returned. This is presentation
composition wall time with GPU completion, not total game GPU time.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import threading
import time


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',required=True,type=Path)
    p.add_argument('--checkpoint',required=True,type=Path)
    p.add_argument('--data-dir',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path)
    p.add_argument('--enabled',required=True,type=int,choices=(0,1))
    p.add_argument('--require-hardware',action='store_true')
    a=p.parse_args(); a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=False)
    for q in (a.binary,a.checkpoint,a.data_dir/'disc1.bin'):
        if not q.is_file():p.error('missing approved input '+str(q))
    env={k:v for k,v in os.environ.items() if not k.startswith(('XENO_','DISPLAY','WAYLAND_DISPLAY','XAUTHORITY'))}
    for name in ('state','config','runtime'):(a.out/name).mkdir(mode=0o700)
    env.update(SDL_VIDEODRIVER='offscreen',EGL_PLATFORM='surfaceless',
        SDL_AUDIODRIVER='dummy',ALSOFT_DRIVERS='null',LP_NUM_THREADS='2',
        XDG_STATE_HOME=str(a.out/'state'),XDG_CONFIG_HOME=str(a.out/'config'),XDG_RUNTIME_DIR=str(a.out/'runtime'),
        XENO_DATA_DIR=str(a.data_dir.resolve()),XENO_QUICKSAVE_PATH=str(a.checkpoint.resolve()),
        XENO_CHEATS_CONSOLE='1',XENO_FEI_HD2D='0',XENO_HD2D_OPENING_MAPS='14',
        XENO_HD2D_OCTOPATH=str(a.enabled),XENO_HD2D_INTEGRATED_BENCHMARK='1',
        XENO_HD2D_SPRITE_DIAG='1',XENO_HD2D_BATCH_STATS='1')
    lines=[]
    with (a.out/'private-runtime.log').open('w') as f:
        proc=subprocess.Popen([str(a.binary.resolve())],cwd=Path.cwd(),env=env,stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        def read():
            for raw in proc.stdout:
                line=raw.decode(errors='replace');lines.append(line);f.write(line);f.flush()
        thread=threading.Thread(target=read);thread.start();start=time.monotonic();sent=0
        try:
            while proc.poll() is None and time.monotonic()-start<300:
                text=''.join(lines)
                if re.search(r'\[hd2d-integrated-benchmark\].*samples=240 ',text):break
                if '[xeno-port][quick] loading' not in text and 'console reading commands' in text and 'FieldLoad begin' in text and time.monotonic()-sent>3:
                    proc.stdin.write(b'load\n');proc.stdin.flush();sent=time.monotonic()
                time.sleep(.25)
        finally:
            proc.terminate()
            try:proc.wait(10)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()
            thread.join()
    text=''.join(lines)
    samples=re.findall(r'\[hd2d-integrated-benchmark\] map=14 enabled=([01]) samples=(\d+) composition_median_ms=([\d.]+) composition_p95_ms=([\d.]+) present_interval_mean_ms=([\d.]+)',text)
    renderer=re.findall(r'GL_RENDERER=(.*?) GL_VERSION=',text)
    splits=[int(x) for x in re.findall(r'peak_splits=(\d+)',text)]
    rejected=[int(x) for x in re.findall(r'capacity_rejections=(\d+)',text)]
    accepted=[int(x) for x in re.findall(r'\[hd2d-sprite-gpu\].*?accepted=(\d+)',text)]
    hardware=bool(renderer and not any(word in renderer[-1].lower()
        for word in ('llvmpipe','softpipe','lavapipe','swrast','software')))
    ok=bool(samples and int(samples[-1][0])==a.enabled and int(samples[-1][1])==240 and
            (not a.require_hardware or hardware) and not any(rejected) and (not a.enabled or any(accepted)))
    metrics={'scope':'actual field14 presentation composition; not total engine GPU time',
        'enabled':a.enabled,'renderer':renderer[-1] if renderer else None,'hardware':hardware,
        'samples':int(samples[-1][1]) if samples else 0,
        'composition_median_ms':float(samples[-1][2]) if samples else None,
        'composition_p95_ms':float(samples[-1][3]) if samples else None,
        'present_interval_mean_ms':float(samples[-1][4]) if samples else None,
        'peak_splits':max(splits,default=0),'capacity_rejections':max(rejected,default=0),
        'actual_sprite_gpu_acceptances':max(accepted,default=0),'verdict':'PASS' if ok else 'FAIL'}
    (a.out/'metrics.json').write_text(json.dumps(metrics,indent=2)+'\n');print(json.dumps(metrics,indent=2))
    return 0 if ok else 1

if __name__=='__main__':raise SystemExit(main())
