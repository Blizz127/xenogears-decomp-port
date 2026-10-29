#!/usr/bin/env python3
"""warp_sweep_analyze.py SWEEPDIR [--json OUT] -- judge dev-menu warp/step sweeps.

SWEEPDIR holds one sub-directory per target, each with target.json
({"id","kind":"warp"|"step","map","entrance","scenario","name","walk":true|false,...}),
run.log and optionally route.jsonl / final_frame.bmp, written by
warp_sweep_one.sh.  Per DEV_MENU_SPEC.md 6 a target is VERIFIED when

  (a) arrival   the action was accepted, a FieldLoad for the destination map
                followed, POSDIAG reports that map afterwards, POSDIAG lines
                (field ticks) keep advancing to the end of the idle window,
                and the run is clean (no fatal marker, terminated by us);
  (b) idle      neutral input: POSDIAG keeps advancing through the idle window,
                the map does not change, nothing fatal;
  (c) walk-in   holding a direction moves the player (WALK IN) and nothing
                fatal; skipped only when target.json says "walk": false;
  (d) step      kind "step" targets add a continuation: after the recorded
                delta and the field change, the next talk/touch trigger fires
                (route.jsonl `trigger` event after CONT_BEGIN).

Prints a table and `VERIFIED n/m`.  Exit status 0 always (evidence, not gate).
"""
import json, os, re, sys

FATAL = ['SIGSEGV', 'Segmentation fault', 'SIGABRT', 'Aborted', 'malloc_printerr', 'corrupted size',
         'free(): invalid', 'double free', 'AddressSanitizer', '[stub]', 'ptag length is not valid',
         'missing D_8004FE50', 'implausible size', 'sweep] FAIL']
POS = re.compile(r'POSDIAG map=(\d+) pos=\((-?\d+),(-?\d+),(-?\d+)\)')


def load_lines(path):
    with open(path, 'r', errors='replace') as f:
        return f.read().splitlines()


def analyze(d):
    t = json.load(open(os.path.join(d, 'target.json')))
    res = {'id': t['id'], 'name': t.get('name', ''), 'kind': t.get('kind', 'warp'), 'map': t['map'],
           'entrance': t.get('entrance'), 'scenario': t.get('scenario'), 'checks': {}, 'notes': []}
    log = os.path.join(d, 'run.log')
    if not os.path.exists(log):
        res['verified'] = False
        res['notes'].append('no run.log')
        return res
    L = load_lines(log)
    idx = {}
    for i, l in enumerate(L):
        for key in ('TARGET mode', 'ARRIVAL_WINDOW', 'IDLE_BEGIN', 'IDLE_END', 'CONT_BEGIN', 'CONT_END', 'EXIT rc',
                    'checkpoint loaded'):
            if ('[sweep] ' + key) in l and key not in idx:
                idx[key] = i
    for i, l in enumerate(L):
        if '[sweep] WALK_BEGIN' in l:
            idx.setdefault('walk_begin', []).append(i)
        if '[sweep] WALK_END' in l:
            idx.setdefault('walk_end', []).append(i)
    fatal = [m for m in FATAL if any(m in l for l in L)]
    res['fatal'] = fatal
    rc = None
    if 'EXIT rc' in idx:
        m = re.search(r'EXIT rc=(\d+)', L[idx['EXIT rc']])
        rc = int(m.group(1)) if m else None
    res['exit_rc'] = rc
    clean = not fatal and rc in (0, 124, 130, 137, 143)
    target = int(t['map'])
    # (a) arrival
    start = idx.get('TARGET mode', len(L))
    accepted = any(('warpx requested' in l or 'warp requested' in l or 'step finished' in l) for l in L[start:])
    # the destination FieldLoad logs the raw g_GameSceneMapNum
    loads = [(i, int(m.group(1))) for i, l in enumerate(L[start:], start)
             for m in [re.search(r'FieldLoad begin field=(\d+)', l)] if m]
    load_ok = any((v & 0x3FFF) == target for _, v in loads)
    pos_after = [(i, int(m.group(1)), int(m.group(2)), int(m.group(4)))
                 for i, l in enumerate(L[start:], start) for m in [POS.search(l)] if m]
    arr_pos = [p for p in pos_after if p[1] == target and p[0] >= idx.get('ARRIVAL_WINDOW', len(L))]
    i_end = idx.get('IDLE_END', len(L))
    i_beg = idx.get('IDLE_BEGIN', len(L))
    idle_pos = [p for p in pos_after if i_beg <= p[0] < i_end]
    res['checks']['accepted'] = accepted
    res['checks']['field_load_for_map'] = load_ok
    res['checks']['map_token_matches'] = len(arr_pos) >= 2
    res['checks']['ticks_advance'] = len(idle_pos) >= 4
    res['checks']['clean_run'] = clean
    arrival = accepted and load_ok and len(arr_pos) >= 2 and clean
    # (b) idle
    idle = len(idle_pos) >= 4 and all(p[1] == target for p in idle_pos)
    res['checks']['idle_live'] = idle
    # (c) walk
    walk_needed = t.get('walk', True)
    moved = 0.0
    wb, we = idx.get('walk_begin', []), idx.get('walk_end', [])
    best = 0.0
    for b, e in zip(wb, we):
        before = [p for p in pos_after if p[0] < b]
        inside = [p for p in pos_after if b <= p[0] < e]
        if before and inside:
            x0, z0 = before[-1][2], before[-1][3]
            for p in inside:
                best = max(best, ((p[2] - x0) ** 2 + (p[3] - z0) ** 2) ** 0.5)
    moved = best
    res['walk_max_displacement'] = round(moved, 1)
    res['checks']['walk_in'] = (moved >= 20.0) if walk_needed else None
    walk_ok = (moved >= 20.0 and clean) if walk_needed else True
    # (d) continuation for steps
    cont_ok = True
    if t.get('kind') == 'step':
        cont_ok = False
        rj = os.path.join(d, 'route.jsonl')
        pat = {}
        for kv in (t.get('continuation') or '').split(','):
            if '=' in kv:
                k, _, v = kv.partition('=')
                pat[k] = int(v) if v.lstrip('-').isdigit() else v
        if os.path.exists(rj) and pat:
            # after the step's own field load, the game must itself produce the expected event
            ev = [json.loads(x) for x in open(rj) if x.strip().startswith('{')]
            want = int(t['map'])
            armed = False
            for e in ev:
                if e['ev'] == 'fieldload' and (e.get('to', -1) & 0x3FFF) == want:
                    armed = True
                    continue
                if armed and all(e.get(k) == v for k, v in pat.items()):
                    cont_ok = True
                    break
        res['checks']['continuation'] = cont_ok
        res['continuation'] = t.get('continuation')
    res['verified'] = bool(arrival and idle and walk_ok and cont_ok)
    res['evidence'] = d
    return res


def main():
    root = sys.argv[1]
    out = None
    if '--json' in sys.argv:
        out = sys.argv[sys.argv.index('--json') + 1]
    results = []
    for name in sorted(os.listdir(root)):
        d = os.path.join(root, name)
        if os.path.isfile(os.path.join(d, 'target.json')):
            results.append(analyze(d))
    n = sum(1 for r in results if r['verified'])
    print('%-28s %-5s %-6s %-4s %-9s %s' % ('target', 'map', 'entr', 'walk', 'verdict', 'notes'))
    for r in results:
        failed = [k for k, v in r['checks'].items() if v is False]
        print('%-28s %-5s %-6s %-4s %-9s %s' % (r['id'], r['map'], r.get('entrance'), r.get('walk_max_displacement'),
                                               'VERIFIED' if r['verified'] else 'FAILED',
                                               ','.join(failed + r.get('fatal', []) + r['notes'])))
    print('VERIFIED %d/%d' % (n, len(results)))
    if out:
        json.dump({'verified': n, 'total': len(results), 'results': results}, open(out, 'w'), indent=1)


if __name__ == '__main__':
    main()
