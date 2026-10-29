#!/usr/bin/env python3
"""Export raw map1 player parts only from completed selected walking captures.
A screenshot is associated only when adjacent logged presents agree on the
sprite frame and exact part UV keys. Raw quads/OTs retain their own present IDs.
"""
import argparse,json
from pathlib import Path
from collections import defaultdict

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);a=p.parse_args();run=a.run.resolve();trace=run/'TRACE'
 captures=json.loads((trace/'captures.json').read_text());present_ids=sorted(int(n.rsplit('-',1)[1].split('.')[0]) for n in captures);lo,hi=present_ids[0],present_ids[-1]
 groups=defaultdict(list)
 for line in (trace/'probe.txt').read_text().splitlines():
  if not line.startswith('part '):continue
  r=dict(s.split('=',1) for s in line.split()[1:])
  if r['map']=='1' and r['actor']==r['player'] and r['fnv']=='db7d0105':groups[int(r['present'])].append(r)
 def key(rows):return tuple((r['frame'],r['part'],r['uv'],r['tpage'],r['clut']) for r in rows)
 variants={}
 records=[]
 for present,rows in sorted(groups.items()):
  if not lo<=present<=hi:continue
  records.extend(rows)
  k=key(rows)
  entry=variants.setdefault(k,{'raw_frame':int(rows[0]['frame']),'variant_id':len(variants),'observed_presents':[],'sample_presents':[],'stable_screenshot':None,'parts':rows})
  entry['observed_presents'].append(present)
  if present not in present_ids:continue
  entry['sample_presents'].append(present)
  if entry['stable_screenshot'] is None and groups.get(present-1) and groups.get(present+1) and k==key(groups[present-1])==key(groups[present+1]):
   entry.update(stable_screenshot=f'TRACE/cap/field-frame-{present:06d}.png',parts=rows,adjacent_presents=[present-1,present,present+1])
 record_path=run/'walking-player-parts.jsonl'
 record_path.write_text(''.join(json.dumps(r,sort_keys=True)+'\n' for r in records))
 report={'source':str(trace/'probe.txt'),'selected_window':[lo,hi],'map':1,'actor':1,'fnv':'db7d0105','variants':sorted(variants.values(),key=lambda r:(r['raw_frame'],r['variant_id'])),
         'full_part_records':str(record_path),'part_record_count':len(records),
         'note':'Frame labels are raw indices; each distinct ordered frame/part/UV/tpage/clut tuple set is preserved separately. No atlas facing or animation assignment inferred. Screenshot associations require matching tuples at adjacent presents; raw quads and OT belong to their logged present.'}
 (run/'walking-player-inventory.json').write_text(json.dumps(report,indent=2));print(json.dumps({'window':[lo,hi],'frames':[(r['raw_frame'],r['variant_id'],r['stable_screenshot']) for r in report['variants']]},indent=2))
if __name__=='__main__':main()
