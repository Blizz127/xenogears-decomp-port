#!/usr/bin/env python3
"""Correlate matched battle leaves with production link ownership.

This reports a bounded source/link intersection. Differential and gameplay
acceptance remain separate: neither nm nor objdiff proves execution correctness.
Inputs are immutable receipts from the same source checkpoint as this checkout.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

import battle_overlay_host_tus as battle


def inventory(report, linked_text, objects_text, args_text):
    matched = {(u['name'], f['name'])
               for u in report['units'] for f in u.get('functions', [])
               if f.get('fuzzy_match_percent') == 100.0}
    symbols = {}
    for line in linked_text.splitlines():
        parts = line.split()
        if len(parts) == 3:
            symbols.setdefault(parts[2], []).append(parts[1])
    objects = {}
    for line in objects_text.splitlines():
        m = re.fullmatch(r'([^:]+):[0-9a-fA-F]+ ([A-Za-z]) (\S+)', line)
        if m:
            key = (Path(m[1]).name, m[3])
            objects.setdefault(key, []).append(m[2])
    if not symbols or not objects:
        raise ValueError('empty production symbol receipts')
    link_args = set(args_text.splitlines())
    definitions = battle.definitions()
    accepted, excluded = [], []
    for name in battle.leaves():
        owners = battle.owners(name, definitions)
        record = {'name': name, 'owners': owners}
        if len(owners) != 1:
            record['reason'] = 'missing or ambiguous source ownership'
        else:
            owner = owners[0]
            unit = 'battle/' + owner.removesuffix('.c')
            obj = 'battle_host_' + owner + '.o'
            record.update(matched_unit=unit, owner='src/battle/' + owner,
                          object=obj)
            if (unit, name) not in matched:
                record['reason'] = 'expected owner unit has no exact retail match'
            elif (owner, False) not in definitions[name]:
                record['reason'] = 'port-only definition; exclude retail match credit'
            else:
                hits = [s for s in (name, 'battle_' + name)
                        if objects.get((obj, s)) == ['T']
                        and symbols.get(s) == ['T']
                        and 'pc_port/build_native/obj/' + obj in link_args]
                if len(hits) != 1:
                    record['reason'] = 'missing or ambiguous strong linked owner'
                else:
                    record['linked_symbol'] = hits[0]
                    record['source_has_port_conditionals'] = 'XENO_PC_PORT' in (
                        battle.BATTLE_SRC / owner).read_text()
        (excluded if 'reason' in record else accepted).append(record)
    return {'scope': 'matched-source battle allowlist bodies linked in production; '
                     'host adaptations allowed; not whole-port or runtime coverage',
            'allowlist_count': len(battle.leaves()),
            'linked_matched_owner_count': len(accepted),
            'accepted': accepted, 'excluded': excluded}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--receipts', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    paths = [args.report] + [args.receipts / name for name in
                            ('linked-symbols.txt', 'object-symbols.txt', 'link-args.txt')]
    result = inventory(json.loads(paths[0].read_text()),
                       *(p.read_text() for p in paths[1:]))
    result['inputs'] = [{'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                        for p in paths]
    result['test_acceptance'] = 'separate differential receipt required'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print('Matched-source battle owners linked:', result['linked_matched_owner_count'],
          'of', result['allowlist_count'], 'allowlist entries (not runtime coverage)')


if __name__ == '__main__':
    main()
