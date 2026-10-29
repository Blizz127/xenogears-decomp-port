#!/usr/bin/env python3
"""Partition head-UV trials and run the unchanged strict b84 analyzer per variant.
Read-only source captures are symlinked; no source pixels or thresholds change.
This reports test candidates, never a production silhouette PASS.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]

def kv(line):
    return dict(t.split('=',1) for t in line.split() if '=' in t)

def groups(variant_lines,captures):
    by_block={}
    for line in variant_lines:
        d=kv(line)
        if d.get('registered')!='1': continue
        by_block.setdefault(int(d['present'])//8,set()).add(int(d['variant']))
    result={}
    for c in captures:
        block=c//8
        variants=by_block.get(block,set())
        if block%3!=1 or len(variants)!=1 or c-8 not in captures or c+8 not in captures: continue
        result.setdefault(next(iter(variants)),set()).update((c-8,c,c+8))
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path)
    p.add_argument('--out',required=True,type=Path)
    args=p.parse_args();run=args.run.resolve();out=args.out.resolve();out.mkdir(exist_ok=False,parents=True)
    variants=(run/'head-variants.log').read_text().splitlines()
    captures={int(f.stem.rsplit('-',1)[1]) for f in (run/'cap').glob('fei-frame-*.png')}
    partition=groups(variants,captures)
    env=dict(os.environ,FEI_FOOTPRINT_STRICT_HORIZONTAL='1')
    lane=(run/'lane-parts.log').read_text().splitlines()
    reports=[]
    from PIL import Image
    for index,chosen in sorted(partition.items()):
        dest=out/f'variant-{index}';cap=dest/'cap';cap.mkdir(parents=True)
        for c in sorted(chosen):
            name=f'fei-frame-{c:06d}.png';(cap/name).symlink_to(run/'cap'/name)
        with (dest/'analysis.txt').open('w') as f:
            result=subprocess.run([sys.executable,str(ROOT/'pc_port/tests/fei_hd2d_footprint_analyze.py'),str(run/'merged-parts.log'),str(cap),str(dest),'--tol','1'],env=env,stdout=f,stderr=subprocess.STDOUT)
        measured={i for c in chosen if c//8%3==1 for i in range(c//8*8,c+1)}
        gpu=sum('gpu_accepted=1' in line and int(kv(line)['present']) in measured for line in lane)
        rows=[kv(line) for line in variants if int(kv(line)['present']) in measured and 'rect=' in line]
        head_gpu=sum('gpu_consumed=1' in line and int(kv(line)['present']) in measured for line in variants)
        rects={row.get('part','5'):row['rect'] for row in rows}
        gpu_parts={part:sum('gpu_consumed=1' in line and kv(line).get('part','5')==part and int(kv(line)['present']) in measured for line in variants) for part in rects}
        changed=0
        if all((dest/f'full_{name}.png').exists() for name in ('retail','hd2d','hidden')):
            images=[Image.open(dest/f'full_{name}.png').convert('RGB') for name in ('retail','hd2d','hidden')]
            changed=sum(r!=h and (r!=x or h!=x) for r,h,x in zip(*(image.getdata() for image in images)))
        analysis=(dest/'analysis.txt').read_text()
        per_part=all(f'p{i}:' in analysis for i in range(6))
        evidence_pass=result.returncode==0 and bool(rects) and all(gpu_parts.values()) and changed>0 and per_part
        reports.append(dict(variant=index,name=rows[0]['name'] if rows else None,
            rect=rows[0]['rect'] if rows else None,rects_by_part=rects,gpu_by_part=gpu_parts,changed_sprite_pixels=changed,per_part_measured=per_part,evidence_pass=evidence_pass,captures=sorted(chosen),gpu_acceptances=gpu,head_part_gpu_consumptions=head_gpu,
            strict_analyzer='PASS' if result.returncode==0 else 'FAIL',
            status='candidate requiring unwrapped native production rebuild and rerun'))
    report=dict(test_only=True,all_eight_variants_measured=set(partition)==set(range(8)),variants=reports)
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return 0 if reports else 1
if __name__=='__main__':raise SystemExit(main())
