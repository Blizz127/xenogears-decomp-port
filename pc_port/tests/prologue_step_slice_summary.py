#!/usr/bin/env python3
"""Compare a reference and an interpreter-disabled prologue narration run
(run_prologue_step_slice.sh): retail boot -> title -> New Game -> map 4
narration, ending by itself inside map 4 before the map-2 load and the
opening battle.

  prologue_step_slice_summary.py REF_DIR DIS_DIR OUT.json

Exit 0 only if every PASS line holds. Frames and SPU are compared from the
run directories; nothing is copied out of them.
"""
import hashlib
import json
import re
import sys
from pathlib import Path

CHECK = re.compile(r'\[field-step-pad\] checkpoint step=(\d+) at_step=(\d+) presented=(\d+) vblank=(-?\d+)'
                   r' entries=(\d+) native_bridges=(\d+) interpreted_instructions=(\d+)'
                   r' disabled_refusals=(\d+) unresolved_bridges=(\d+)')
TEXTBOX = re.compile(r'\[textbox\] box=(\d+) (shown|hidden) wait=(\d) step=(\d+)')
CAP = re.compile(r'frame cap: exiting at step=(\d+)')


def sha(path):
    path = Path(path)
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None


def run(path):
    path = Path(path)
    text = path.joinpath('run.log').read_text(errors='replace').splitlines()
    out = {'dir': str(path), 'rc': path.joinpath('rc.txt').read_text().strip()
           if path.joinpath('rc.txt').exists() else None}
    out['handover'] = any('hand-over: boot pad schedule retires at map 4' in l for l in text)
    out['armed'] = any('[field-step-pad] armed map=4 ' in l for l in text)
    out['new_game'] = any('[menu] title confirm choice=2' in l for l in text)
    out['maps'] = [int(m.group(1)) for l in text
                   for m in [re.search(r'FieldLoad begin field=(\d+)', l)] if m]
    out['stubs'] = sum('[stub]' in l for l in text)
    out['refusal_lines'] = sum('CPU interpreter disabled: unconverted target' in l for l in text)
    out['battle_lines'] = sum('[cpu-fallback] battle=' in l for l in text)
    out['empty_drains'] = sum('drained no vblank' in l for l in text)
    cap = [int(m.group(1)) for l in text for m in [CAP.search(l)] if m]
    out['exit_step'] = cap[-1] if cap else None
    out['textbox'] = [[int(m.group(1)), m.group(2), int(m.group(3)), int(m.group(4))]
                      for l in text for m in [TEXTBOX.search(l)] if m]
    out['checkpoints'] = []
    for l in text:
        m = CHECK.search(l)
        if m:
            st = int(m.group(1))
            out['checkpoints'].append({
                'step': st, 'at_step': int(m.group(2)), 'presented': int(m.group(3)),
                'vblank': int(m.group(4)), 'entries': int(m.group(5)),
                'native_bridges': int(m.group(6)), 'interpreted': int(m.group(7)),
                'refusals': int(m.group(8)), 'unresolved': int(m.group(9)),
                'frame': sha(path / 'captures' / ('step-%06d.png' % st))})
    spu = path / 'spu.log'
    # Column 1 is the logger's own vblank index; the load before arming is
    # host-paced, so compare (step, page hash) only.
    out['spu'] = [l.split()[1:] for l in spu.read_text().splitlines()] if spu.exists() else []
    return out


def main():
    ref, dis = run(sys.argv[1]), run(sys.argv[2])
    results = []

    def check(name, ok):
        results.append((name, bool(ok)))
        print('%s %s' % ('PASS' if ok else 'FAIL', name))

    for tag, r in (('reference', ref), ('disabled', dis)):
        check('%s: boot schedule handed over at map 4 and the probe armed there' % tag,
              r['handover'] and r['armed'])
        check('%s: New Game chosen on the title menu' % tag, r['new_game'])
        check('%s: map 4 loaded, map 2 NOT loaded (ends before the opening battle)' % tag,
              4 in r['maps'] and 2 not in r['maps'])
        check('%s: exited by itself on the step cap (rc=0)' % tag,
              r['rc'] == 'rc=0' and r['exit_step'] is not None)
        check('%s: no stubs, no battle entered' % tag, r['stubs'] == 0 and r['battle_lines'] == 0)
    check('disabled: zero interpreted instructions, refusals and unresolved bridges at every checkpoint',
          dis['checkpoints'] and all(c['interpreted'] == 0 and c['refusals'] == 0 and c['unresolved'] == 0
                                     for c in dis['checkpoints']) and dis['refusal_lines'] == 0)
    pages_ref = [t for t in ref['textbox'] if t[1] == 'shown']
    check('narration: at least one text-box page shown', len(pages_ref) > 0)
    check('narration: text-box show/hide sequence identical (box, state, wait, step)',
          ref['textbox'] == dis['textbox'])
    check('visual: checkpoint frames present and identical',
          [c['step'] for c in ref['checkpoints']] == [c['step'] for c in dis['checkpoints']] and
          all(c['frame'] for c in ref['checkpoints']) and
          [c['frame'] for c in ref['checkpoints']] == [c['frame'] for c in dis['checkpoints']])
    # Boot and loads before arming are host-paced, so absolute presented /
    # vblank counts carry a per-run offset; compare deltas between checkpoints.
    def deltas(r, key):
        v = [c[key] for c in r['checkpoints']]
        return [b - a for a, b in zip(v, v[1:])]
    check('timing: checkpoint at_step and presented/vblank deltas identical',
          [c['at_step'] for c in ref['checkpoints']] == [c['at_step'] for c in dis['checkpoints']] and
          deltas(ref, 'presented') == deltas(dis, 'presented') and
          deltas(ref, 'vblank') == deltas(dis, 'vblank'))
    check('audio: SPU register page identical every logged vblank (step, hash)',
          ref['spu'] and ref['spu'] == dis['spu'])
    check('exit step identical', ref['exit_step'] == dis['exit_step'])
    summary = {
        'pages': len(pages_ref),
        'all_pages_auto_advance': all(t[2] == 0 for t in ref['textbox']),
        'textbox_events': len(ref['textbox']),
        'checkpoints': len(ref['checkpoints']),
        'spu_rows': len(ref['spu']),
        'exit_step': ref['exit_step'],
        'empty_drains': [ref['empty_drains'], dis['empty_drains']],
        'maps': [ref['maps'], dis['maps']],
    }
    print('summary ' + json.dumps(summary))
    Path(sys.argv[3]).write_text(json.dumps({'summary': summary, 'checks': results,
                                             'reference': ref, 'disabled': dis}, indent=1) + '\n')
    ok = all(ok for _, ok in results)
    print('PROLOGUE SLICE %s' % ('PASS' if ok else 'FAIL'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
