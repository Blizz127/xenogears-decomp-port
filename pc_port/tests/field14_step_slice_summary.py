#!/usr/bin/env python3
"""Summarise and compare run_field14_step_slice.sh runs.

  field14_step_slice_summary.py REFERENCE_DIR DISABLED_DIR OUT.json

Per run: fixture load, field-step arming, step transitions, empty drains,
interpreter refusals, CPU execution counters, and the stable map-14 position
at the end of each neutral phase (the POSDIAG sample just before each
transition, and the last one).  PASS needs both runs to load the fixture,
arm, complete all transitions, show identical checkpoint positions, and the
disabled run to report zero interpreted instructions and no refusal.  It is
movement-slice evidence only: no audio, no frame-exact timing.
"""
import hashlib
import os
import json
import re
import sys
from pathlib import Path

POS = re.compile(r'POSDIAG map=(\d+) pos=\((-?\d+),(-?\d+),(-?\d+)\)')
STEP = re.compile(r'\[field-step-pad\] step=(\d+) held=(0x[0-9a-f]+)')
CHECK = re.compile(r'\[field-step-pad\] checkpoint step=(\d+) at_step=(\d+) presented=(\d+) vblank=(-?\d+)'
                   r'(?: .*interpreted_instructions=(\d+) disabled_refusals=(\d+))?')


def sha(path):
    path = Path(path)
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None


def run(path):
    text = Path(path, 'run.log').read_text(errors='replace').splitlines()
    out = {'dir': str(path), 'loaded': False, 'armed': False, 'transitions': [],
           'empty_drains': 0, 'refusals': 0, 'stubs': 0, 'checkpoints': [],
           'execution_stats': [], 'fallback_stats': []}
    last_pos = None
    for line in text:
        if '[xeno-port][quick] loading ' in line:
            out['loaded'] = True
        if '[field-step-pad] armed map=14' in line:
            out['armed'] = True
        if 'drained no vblank' in line:
            out['empty_drains'] += 1
        if 'CPU interpreter disabled: unconverted target' in line:
            out['refusals'] += 1
        if '[stub]' in line:
            out['stubs'] += 1
        if '[menu] func_800799D4 request=128' in line and '-> MenuMain' in line:
            out['menu_opens'] = out.get('menu_opens', 0) + 1
        if '[menu] MenuMain returned' in line:
            out['menu_returns'] = out.get('menu_returns', 0) + 1
        if 'cpu-execution' in line:
            out['execution_stats'].append(line.strip())
        if '[cpu-fallback] battle=' in line:
            out['fallback_stats'].append(line.strip())
        m = POS.search(line)
        if m and m.group(1) == '14' and out['armed']:
            last_pos = [int(m.group(i)) for i in (2, 3, 4)]
        m = STEP.search(line)
        if m and out['armed']:
            out['transitions'].append([int(m.group(1)), m.group(2)])
            out['checkpoints'].append(last_pos)
    out['checkpoints'].append(last_pos)
    out['timing'] = []
    out['frames'] = {}
    for line in text:
        m = CHECK.search(line)
        if m:
            st = int(m.group(1))
            out['timing'].append({'step': st, 'at_step': int(m.group(2)),
                                  'presented': int(m.group(3)), 'vblank': int(m.group(4)),
                                  'interpreted': None if m.group(5) is None else int(m.group(5)),
                                  'refusals': None if m.group(6) is None else int(m.group(6))})
            out['frames'][st] = sha(Path(path, 'captures', 'step-%06d.png' % st))
            mp = re.search(r'player=\((-?\d+),(-?\d+),(-?\d+)\)', line)
            if mp:
                out.setdefault('checkpoint_player', []).append([int(mp.group(i)) for i in (1, 2, 3)])
    out['saved'] = any('F11 quick save requested' in l for l in text) and \
        any('[xeno-port][quick] saved ' in l for l in text)
    out['reloaded'] = any('F12 quick load requested' in l for l in text)
    out['save_sha256'] = sha(Path(path, 'quicksave.xgqs'))
    out['entry_sha256'] = sha(Path(path, 'entry.xgqs'))
    spu = Path(path, 'spu.log')
    # Same fixed game-time window as field14_spu_pairs.py (steps <= 1000).
    out['spu'] = [r for r in (l.split() for l in spu.read_text().splitlines())
                  if int(r[1]) <= 1000] if spu.exists() else []
    interp = [int(x) for l in out['execution_stats']
              for x in re.findall(r'interpreted_instructions=(\d+)', l)]
    # Exit receipt if present, else the last in-run checkpoint receipt.
    live = [t['interpreted'] for t in out['timing'] if t.get('interpreted') is not None]
    interp += live[-1:]
    out['interpreted_instructions'] = max(interp) if interp else None
    out['checkpoint_counters'] = len(live)
    out['rc'] = Path(path, 'rc.txt').read_text().strip() if Path(path, 'rc.txt').exists() else None
    return out


def main():
    ref, dis = run(sys.argv[1]), run(sys.argv[2])
    # Expected transitions and checkpoints come from what the driver ran.
    sched = Path(sys.argv[1], 'schedule.txt').read_text().strip().split(',')
    want = [[int(e.split(':')[0]), '0x%04x' % int(e.split(':')[1], 0)] for e in sched
            if int(e.split(':')[0]) > 0]
    cp_file = Path(sys.argv[1], 'checkpoints.txt')
    n_checkpoints = len(cp_file.read_text().strip().split(',')) if cp_file.exists() else 9

    def deltas(run, key):
        t = run['timing']
        return [t[i + 1][key] - t[i][key] for i in range(len(t) - 1)]
    rs, ds = ref['spu'], dis['spu']
    common = min(len(rs), len(ds))
    first_diff = next((i for i in range(common) if rs[i][2] != ds[i][2]), None)
    first_by_step_ref, first_by_step_dis = {}, {}
    for row in rs:
        first_by_step_ref.setdefault(row[1], row[2])
    for row in ds:
        first_by_step_dis.setdefault(row[1], row[2])
    def distinct(rows):
        out, prev = [], None
        for row in rows:
            if row[2] != prev:
                out.append(row[2])
                prev = row[2]
        return out
    da, db = distinct(rs), distinct(ds)
    dn = min(len(da), len(db))
    seq_diff = next((i for i in range(dn) if da[i] != db[i]), None)
    spu = {'distinct_states_ref': len(da), 'distinct_states_disabled': len(db),
           'distinct_sequence_first_divergence': seq_diff,
           'vblanks_ref': len(rs), 'vblanks_disabled': len(ds),
           'first_vblank_divergence': first_diff,
           'distinct_hashes_ref': len({r[2] for r in rs}),
           'per_step_first_hash_equal': sum(1 for k in first_by_step_ref
                                            if first_by_step_dis.get(k) == first_by_step_ref[k]),
           'per_step_compared': len(first_by_step_ref)}
    checks = {
        'both loaded the fixture': ref['loaded'] and dis['loaded'],
        'both armed in field 14': ref['armed'] and dis['armed'],
        'both completed every transition': ref['transitions'] == want and dis['transitions'] == want,
        'checkpoint positions identical': ref['checkpoints'] == dis['checkpoints'] and None not in ref['checkpoints'],
        # Prefer positions read at the checkpoints themselves (always logged);
        # POSDIAG-derived quiescent positions are silent after a field menu.
        'movement observed (at least one move between checkpoints)': len(
            {tuple(c) for c in (ref.get('checkpoint_player') or ref['checkpoints']) if c}) >= 2,
        'checkpoint player positions identical': ref.get('checkpoint_player') == dis.get('checkpoint_player'),
        'disabled: no interpreter refusal': dis['refusals'] == 0,
        'disabled: zero interpreted instructions counted': dis['interpreted_instructions'] == 0,
        'no stubs reached': ref['stubs'] == 0 and dis['stubs'] == 0,
        'visual: checkpoint frames identical': len(ref['frames']) == n_checkpoints and ref['frames'] == dis['frames']
            and all(isinstance(v, str) and len(v) == 64 for v in ref['frames'].values()),
        'timing: checkpoint steps and presented-frame deltas identical':
            [t['at_step'] for t in ref['timing']] == [t['at_step'] for t in dis['timing']]
            and deltas(ref, 'presented') == deltas(dis, 'presented') and len(ref['timing']) == n_checkpoints,
        'save: F11 saved (and F12 reloaded when scheduled) in both': ref['saved'] and dis['saved']
            and (os.environ.get('FIELD14_LOAD_AT', '900') == '-1'
                 or (ref['reloaded'] and dis['reloaded'])),
        'save: saved checkpoint byte-identical': ref['save_sha256'] == dis['save_sha256']
            and isinstance(ref['save_sha256'], str) and len(ref['save_sha256']) == 64
            and ref['save_sha256'] != ref['entry_sha256'],
        'both runs exited by themselves (rc=0 on the frame cap)': ref['rc'] == 'rc=0' and dis['rc'] == 'rc=0',
        'counters: both runs have a receipt (exit or in-run)': ref['interpreted_instructions'] is not None
            and dis['interpreted_instructions'] is not None,
    }
    informational = {
        'audio: SPU register page identical every vblank': first_diff is None and len(rs) > 0
            and len(rs) == len(ds),
        'audio: SPU distinct-state sequence identical': seq_diff is None and len(da) == len(db)
            and len(da) > 0,
        'timing: vblank deltas identical': deltas(ref, 'vblank') == deltas(dis, 'vblank'),
    }
    want_menus = os.environ.get('FIELD14_EXPECT_MENUS')
    if want_menus:
        n = int(want_menus)
        checks['menu: %d field-menu opens and returns in both runs' % n] = all(
            side.get('menu_opens', 0) == n and side.get('menu_returns', 0) == n for side in (ref, dis))
    # Under the deterministic clocks (XENO_SOUND_TICK_LOCKSTEP +
    # XENO_DETERMINISTIC_VBLANK) audio and vblank equality are required too.
    if os.environ.get('FIELD14_REQUIRE_DETERMINISM') == '1':
        checks.update(informational)
    for side in (ref, dis):
        side['spu_lines'] = len(side.pop('spu'))
    result = {'pass': all(checks.values()), 'checks': checks, 'informational': informational,
              'spu': spu, 'presented_deltas': [deltas(ref, 'presented'), deltas(dis, 'presented')],
              'vblank_deltas': [deltas(ref, 'vblank'), deltas(dis, 'vblank')],
              'reference': ref, 'disabled': dis,
              'limits': 'audio evidence = the SPU register page the sound driver writes (the port '
                        'does not yet render it to a device); vblank pacing is host real time'}
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + '\n')
    for k, v in checks.items():
        print(('PASS ' if v else 'FAIL ') + k)
    for k, v in informational.items():
        print(('SAME ' if v else 'DIFFERS ') + k + ' (explain if DIFFERS)')
    print('spu ' + json.dumps(spu))
    print('FIELD14 SLICE ' + ('PASS' if result['pass'] else 'FAIL'))
    return 0 if result['pass'] else 1


if __name__ == '__main__':
    sys.exit(main())
