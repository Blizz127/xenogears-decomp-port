#!/usr/bin/env python3
"""route_steps.py ROUTE.jsonl [ROUTE2.jsonl ...] [--json OUT]

Turns XENO_ROUTE_LOG recordings (pc_port/src/route_recorder.c) into a timeline
of story milestones and candidate steps for the dev-menu Finish Current Step
table.  Only state the game itself wrote is reported; nothing is guessed.

A milestone is one of: a scenario-word write (script var at address 0), a write
to a persistent script variable (address < 0x400, the range the game saves into
g_GameState+0x1930) that is not per-frame noise, a change of party / gold /
inventory, a CHANGE_FIELD (map, entrance), or the first talk/touch trigger
against an actor on a map.  A candidate step is the span between two scenario
changes: its recorded delta is the set of persistent non-noise variable writes
(final value per address) in that span, together with the trigger that was
active at the moment the scenario word changed.

Noise = addresses the script VM rewrites constantly (camera, time, scratch):
any address changed more than NOISE_MAX times in a recording, plus a fixed
list of system slots.
"""
import json, sys
from collections import Counter, OrderedDict

NOISE_MAX = 12
SYSTEM_SLOTS = {0x2, 0x4, 0x12, 0x6, 0x8, 0xA, 0xC, 0xE, 0x10, 0x1E, 0x20, 0x24, 0x3C, 0x3E, 0x40, 0x42, 0x44, 0x46}
PERSIST_LIMIT = 0x400


def load(paths):
    ev = []
    for i, p in enumerate(paths):
        for l in open(p):
            l = l.strip()
            if l.startswith('{'):
                e = json.loads(l)
                e['src'] = i
                ev.append(e)
    return ev


def external_scenario_writes(ev):
    """Indices of scenario-word writes that were NOT made by the game's own script flow:
    the dev-menu/harness warp writes the word then field-loads with no CHANGE_FIELD op in between."""
    out = set()
    for i, e in enumerate(ev):
        if e['ev'] == 'var' and e['addr'] == 0:
            for f in ev[i + 1:]:
                if f['ev'] == 'change':
                    break
                if f['ev'] == 'fieldload':
                    # the harness writes, then the warp path field-loads within a few frames
                    if f['src'] == e['src'] and 0 <= f['tick'] - e['tick'] <= 60:
                        out.add(i)
                    break
    return out


def analyze(ev):
    ext = external_scenario_writes(ev)
    writes = Counter(e['addr'] for e in ev if e['ev'] == 'var')
    noise = {a for a, n in writes.items() if n > NOISE_MAX} | SYSTEM_SLOTS
    noise.discard(0)  # the scenario word is the spine, never noise
    steps, cur = [], None
    last_trigger = None
    timeline = []
    prev_items = prev_party = None
    prev_scen = None
    for idx, e in enumerate(ev):
        k = e['ev']
        if k == 'trigger':
            last_trigger = e
            key = (e['map'], e['actor'], e['kind'])
            if key not in {t.get('key') for t in timeline if t['what'] == 'trigger'}:
                timeline.append({'what': 'trigger', 'key': key, 'map': e['map'], 'actor': e['actor'],
                                 'kind': e['kind'], 'actor_xz': e['actor_xz'], 'scen': e['scen'], 'n': e['n']})
        if k in ('fieldload', 'change', 'state'):
            items = json.dumps(e.get('items'))
            if prev_items is not None and (items != prev_items or e.get('party') != prev_party):
                timeline.append({'what': 'inventory/party', 'map': e['map'], 'scen': e['scen'], 'party': e.get('party'),
                                 'gold': e.get('gold'), 'items': e.get('items'), 'n': e['n']})
            prev_items, prev_party = items, e.get('party')
        if k == 'change':
            timeline.append({'what': 'change_field', 'from': e['from'], 'to': e['to'], 'to_ent': e['to_ent'],
                             'scen': e['scen'], 'actor': e.get('actor'), 'n': e['n']})
        if k == 'fieldload':
            if e['to'] == 490:
                cur = None   # a new session starts at the title: never carry a span across recordings
            timeline.append({'what': 'fieldload', 'map': e['to'], 'ent': e['ent'], 'scen': e['scen'],
                             'party': e.get('party'), 'gold': e.get('gold'), 'n': e['n']})
        if k == 'var':
            a = e['addr']
            if a == 0 and idx in ext:
                timeline.append({'what': 'scenario(harness write, not a game step)', 'from': e['old'], 'to': e['new'],
                                 'map': e['map'], 'n': e['n']})
            elif a == 0:
                step = {'scen_from': e['old'], 'scen_to': e['new'], 'map': e['map'], 'ent': e['ent'],
                        'writer_actor': e['actor'], 'writer_ip': e['ip'], 'n': e['n'],
                        'trigger': {kk: last_trigger[kk] for kk in ('kind', 'actor', 'actor_xz', 'map')}
                        if last_trigger and last_trigger['map'] == e['map'] else None,
                        'writes': OrderedDict()}
                steps.append(step)
                cur = step
                timeline.append({'what': 'scenario', 'from': e['old'], 'to': e['new'], 'map': e['map'],
                                 'actor': e['actor'], 'ip': e['ip'], 'n': e['n']})
            elif a < PERSIST_LIMIT and a not in noise:
                if cur is not None and e['ip'] >= 0:   # ip -1 = written outside the script VM (start-up / port code)
                    cur['writes'][a] = e['new']
                timeline.append({'what': 'var', 'addr': a, 'old': e['old'], 'new': e['new'], 'map': e['map'],
                                 'actor': e['actor'], 'ip': e['ip'], 'scen': e['scen'], 'n': e['n']})
    return noise, steps, timeline


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    out = sys.argv[sys.argv.index('--json') + 1] if '--json' in sys.argv else None
    if out:
        args = [a for a in args if a != out]
    ev = load(args)
    noise, steps, timeline = analyze(ev)
    print('events: %d   noise addresses: %s' % (len(ev), ' '.join('0x%x' % a for a in sorted(noise))))
    print('--- scenario-word steps ---')
    for s in steps:
        print('scen %d -> %d  map %d  writer actor %s ip %s  trigger %s' % (
            s['scen_from'], s['scen_to'], s['map'], s['writer_actor'], s['writer_ip'], s['trigger']))
        if s['writes']:
            print('   persistent writes: ' + ', '.join('var[0x%x]=%d' % kv for kv in s['writes'].items()))
    print('--- timeline (map changes, loads, inventory) ---')
    for t in timeline:
        if t['what'] in ('change_field', 'fieldload', 'inventory/party', 'scenario'):
            print({k: v for k, v in t.items() if k != 'items'} if t['what'] != 'inventory/party' else t)
    if out:
        json.dump({'noise': sorted(noise), 'steps': steps, 'timeline': timeline}, open(out, 'w'), indent=1)


if __name__ == '__main__':
    main()
