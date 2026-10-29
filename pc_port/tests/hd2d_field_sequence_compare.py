#!/usr/bin/env python3
"""Strict complete-window OFF comparison; never discard ambiguous/different frames.

Usage: hd2d_field_sequence_compare.py RUN_A RUN_B
Inputs are field_validation.py evidence directories. Output CRC is column four;
input key is column five. Permit a circular phase offset ONLY for equal-length
sequences composed of at least two complete repetitions of the same cycle.
Nonperiodic sequences must match index-for-index. Counts and every sample are
checked; this does not auto-select a convenient subwindow or certify ON art.
"""
import hashlib
import json
from pathlib import Path
import re
import sys


def compare_sequence(a, b):
    if not a or len(a) != len(b):
        return {'pass':False, 'reason':'empty sequence or unequal counts', 'counts':[len(a),len(b)]}
    if a == b:
        return {'pass':True, 'count':len(a), 'phase':0, 'method':'index exact'}
    n=len(a)
    for period in range(1,n//2+1):
        if n % period or any(a[i]!=a[i%period] for i in range(n)):
            continue
        # Require B to have the same period across its entire supplied window.
        if any(b[i]!=b[i%period] for i in range(n)):
            continue
        for phase in range(1,period):
            if all(a[i]==b[(i+phase)%n] for i in range(n)):
                return {'pass':True,'count':n,'phase':phase,'period':period,'cycles':n//period,'method':'complete periodic phase shift'}
    return {'pass':False,'count':n,'reason':'complete sequences differ; no periodic phase equivalence'}


def load_run(path):
    hashes=[]; keys=[]; indices=[]
    for line in (path/'field-hash.txt').read_text().splitlines():
        p=line.split()
        if len(p)!=6: raise ValueError('malformed hash row')
        indices.append(int(p[0])); hashes.append(tuple(p[1:4])); keys.append(p[4])
    if any(b!=a+1 for a,b in zip(indices,indices[1:])): raise ValueError('noncontiguous hash sequence')
    manifest=json.loads((path/'captures.json').read_text())
    shots=[]
    for name,expected in sorted(manifest.items()):
        actual=hashlib.sha256((path/'cap'/name).read_bytes()).hexdigest()
        if actual!=expected: raise ValueError('capture digest does not match manifest')
        shots.append(actual)
    log=(path/'run.log').read_text()
    loads=re.findall(r'\[xeno-port\]\[quick\] loading .*? (map=\d+ pos=\([^\n]+\))',log)
    if not loads or 'player position restored' not in log: raise ValueError('missing checkpoint restoration evidence')
    return hashes,keys,shots,loads[-1]


def main():
    a,b=map(Path,sys.argv[1:3]); ah,ak,ac,al=load_run(a); bh,bk,bc,bl=load_run(b)
    hashes=compare_sequence(ah,bh); captures=compare_sequence(ac,bc)
    lines=(b/'probe.txt').read_text().splitlines()
    posts=[l for l in lines if l.startswith('post ') and 'map=14 ' in l and 'enabled=0 ' in l]
    begins=[l for l in lines if l.startswith('begin ') and 'map=14 ' in l and 'accepted=0' in l]
    result={'hashes':hashes,'captures':captures,'input_keys_index_equal':ak==bk,
            'same_checkpoint_restore':al==bl,'strong_off_posts':len(posts),'declined_begins':len(begins)}
    result['verdict']='PASS' if hashes['pass'] and captures['pass'] and al==bl and posts and begins else 'FAIL'
    print(json.dumps(result,indent=2)); return 0 if result['verdict']=='PASS' else 1


if __name__=='__main__':
    raise SystemExit(main())
