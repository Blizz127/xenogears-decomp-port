#!/usr/bin/env python3
"""Read-only checkpoint/route/probe inventory; emits metadata, never game-state bytes.

Example: --route route.jsonl.gz --checkpoint field.xgqs --probe probe.txt
A route actor ID is NOT a sprite identity or character mapping. Only explicit
probe FNV/frame/part/UV observations are exported as sprite keys.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import struct


def checkpoint(path):
    d=path.read_bytes()
    if len(d)!=48+0x2358 or d[:8]!=b'XGQCKPT\0' or struct.unpack_from('<III',d,8)!=(1,48,0x2358):
        raise ValueError(f'{path}: invalid checkpoint format')
    h=2166136261
    for v in d[24:]: h=((h^v)*16777619)&0xffffffff
    if h!=struct.unpack_from('<I',d,20)[0]: raise ValueError(f'{path}: checksum mismatch')
    return {'source':str(path),'sha256':hashlib.sha256(d).hexdigest(),'valid_checksum':True,
            'map':struct.unpack_from('<H',d,24)[0],'entrance':struct.unpack_from('<H',d,26)[0],
            'position_raw_fixed16':struct.unpack_from('<iii',d,28),
            'position_integer':tuple(v>>16 for v in struct.unpack_from('<iii',d,28)),
            'rotation':struct.unpack_from('<hhh',d,40)}


def route(path):
    events=[]; fields=set(); count=0
    with gzip.open(path,'rt') as f:
        for line in f:
            r=json.loads(line);fields.update(r);count+=1
            if r.get('ev') in ('fieldload','change_field','changefield'):
                events.append({k:r[k] for k in ('n','tick','ev','map','ent','scen','from','to','to_ent','actor') if k in r})
    return {'source':str(path),'events':count,'field_events':events,
            'contains_sprite_identity':any(k in fields for k in ('fnv','identity','uv','tpage','clut'))}


def probe(path):
    facts={}
    for line in path.read_text().splitlines():
        if not line.startswith('part '): continue
        r=dict(t.split('=',1) for t in line.split()[1:] if '=' in t)
        keys=('map','actor','player','fnv','frame','part','uv','tpage','clut')
        if not all(k in r for k in keys): continue
        key=tuple(r[k] for k in keys)
        facts[key]={'source':str(path),'observed_player':r['actor']==r['player'],**{k:r[k] for k in keys}}
    return list(facts.values())


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('checkpoint','route','probe'):p.add_argument('--'+name,type=Path,action='append',default=[])
    a=p.parse_args()
    print(json.dumps({'checkpoints':[checkpoint(q) for q in a.checkpoint],
        'routes':[route(q) for q in a.route], 'observed_sprite_keys':[r for q in a.probe for r in probe(q)]},indent=2))
if __name__=='__main__': main()
