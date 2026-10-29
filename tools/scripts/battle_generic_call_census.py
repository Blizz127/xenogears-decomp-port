#!/usr/bin/env python3
"""Static census of host functions that can reach the battle bridge's generic
call (runtime_bridge_call -> GenericHostFunction).

Every direct `jal` in disc/battle.bin is classified by its target:
  interpreted    guest code (battle image or the 0x801D0000 module window)
                 not on the host-leaf allowlist
  typed          handled by a typed special case before the generic call
  not-code       target outside the main executable and guest windows
                 (data words in the image that decode as jal)
  generic        main-executable function or allowlisted overlay leaf that
                 falls through to the twelve-uintptr_t generic call
For each generic callee the C prototype is looked up in the decomp/port
sources and graded by how far its real ABI is from the generic one.

Scope: static direct calls in battle.bin only. Indirect jalr calls, other
overlay files loaded during battle (file1, intro/results modules) and
whether the host binary actually exports a name (dlsym) are NOT covered;
a name with no definition in the sources is reported, not assumed absent.
Read-only; no build needed. See docs/evidence/generic-host-call-abi-proposal.md.
"""
import argparse
import json
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

BATTLE_BASE = 0x8006FAF0
BATTLE_SHA256 = '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
RUNTIME = Path('pc_port/src/battle_mips_runtime.c')
SYMBOL_FILES = ['config/symbol_addrs.slus_006.64.txt',
                'linker/undefined_funcs_auto.battle.txt']
SOURCE_DIRS = ['src', 'include', 'pc_port/src', 'pc_port/include_shim']
NARROW = re.compile(r'\b(s8|u8|s16|u16|char|short|int8_t|uint8_t|int16_t|uint16_t)\b')
WIDE = re.compile(r'\b(s64|u64|long long|double|float|int64_t|uint64_t)\b')


def load_symbols():
    names = {}
    for path in SYMBOL_FILES:
        for line in Path(path).read_text().splitlines():
            m = re.match(r'\s*([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;', line)
            if m:
                names.setdefault(int(m.group(2), 16), m.group(1))
    return names


def typed_cases():
    source = RUNTIME.read_text()
    body = source[source.index('static int runtime_bridge_call('):]
    body = body[:body.index('function = (GenericHostFunction)resolved->host;')]
    addresses = {int(a, 16) for a in re.findall(r'resolved->address == (0x[0-9a-f]+)u', body)}
    names = set(re.findall(r'strcmp\(resolved->name, "(\w+)"\) == 0', body))
    return addresses, names


def leaves():
    out = subprocess.check_output(
        [sys.executable, 'tools/scripts/battle_overlay_host_tus.py', '--leaves'], text=True)
    return set(out.split())


def index_prototypes(wanted):
    """name -> {'definitions': {proto: [paths]}, 'declarations': {proto}}."""
    found = defaultdict(lambda: {'definitions': defaultdict(list), 'declarations': set()})
    pattern = re.compile(
        r'^[ \t]*(?:extern[ \t]+|static[ \t]+)?([A-Za-z_][\w \t\*]*?)[ \t\*]*\b('
        + '|'.join(sorted(map(re.escape, wanted))) + r')[ \t]*\(([^;{)]*)\)[ \t]*(\{|\n[ \t]*\{)?',
        re.M)
    for directory in SOURCE_DIRS:
        for path in Path(directory).rglob('*'):
            if path.suffix not in ('.c', '.h', '.inc') or not path.is_file():
                continue
            text = path.read_text(errors='replace')
            for m in pattern.finditer(text):
                ret = ' '.join(m.group(1).split())
                if ret in ('return', 'else', 'if', '') or ret.startswith('#'):
                    continue
                stars = text[m.start(1):m.start(2)].count('*')
                params = ' '.join(m.group(3).split()) or 'void'
                proto = (ret + ' ' + '*' * stars).strip() + '(' + params + ')'
                if m.group(4) and 'static' not in text[m.start():m.start(1)]:
                    found[m.group(2)]['definitions'][proto].append(str(path))
                else:
                    found[m.group(2)]['declarations'].add(proto)
    return found


def grade(prototypes):
    """Grade one prototype set: the definitions if any, else declarations."""
    if not prototypes:
        return 'unknown'
    if len(prototypes) > 1:
        return 'conflicting'
    proto = next(iter(prototypes))
    ret, params = proto.split('(', 1)
    params = params[:-1]
    arity = 0 if params.strip() in ('', 'void') else params.count(',') + 1
    if '...' in params or arity > 12 or WIDE.search(proto):
        return 'abi-unsafe'
    if NARROW.search(params) or NARROW.search(ret):
        return 'narrow'
    if ret.strip() == 'void':
        return 'void-return'
    # Only a pointer or a 64-bit integer defines all of RAX; for a 32-bit
    # return the upper half (stored into guest v1) is unspecified.
    if '*' in ret or re.search(r'\b(long|uintptr_t|intptr_t|size_t)\b', ret):
        return 'word-sized'
    return 'int-return'


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--json', type=Path)
    args = parser.parse_args()
    from hashlib import sha256
    image = Path('disc/battle.bin').read_bytes()
    assert sha256(image).hexdigest() == BATTLE_SHA256, 'unexpected disc/battle.bin'
    symbols = load_symbols()
    typed_addresses, typed_names = typed_cases()
    allow = leaves()
    sites = defaultdict(list)
    jalr = 0
    for off in range(0, len(image) - 3, 4):
        word, = struct.unpack_from('<I', image, off)
        pc = BATTLE_BASE + off
        if word >> 26 == 3:
            sites[(pc & 0xF0000000) | ((word & 0x3FFFFFF) << 2)].append(pc)
        elif word >> 26 == 0 and word & 0x3F == 9 and (word >> 16) & 0x1F == 0:
            jalr += 1
    end = BATTLE_BASE + len(image)
    rows, counts = [], defaultdict(int)
    for target in sorted(sites):
        # Mirrors target_is_guest_code(): the battle image plus the
        # 0x801D0000..0x80300000 window other battle modules load into.
        guest = BATTLE_BASE <= target < end or 0x801D0000 <= target < 0x80300000
        name = symbols.get(target, 'func_%08X' % target)
        if not (0x80010000 <= target < BATTLE_BASE or guest):
            kind = 'not-code'   # a data word that decodes as jal
        elif guest and name not in allow and ('func_%08X' % target) not in allow:
            kind = 'interpreted'
        elif target in typed_addresses or name in typed_names:
            kind = 'typed'
        else:
            kind = 'generic'
        counts[kind] += 1
        rows.append({'target': '0x%08x' % target, 'name': name, 'kind': kind,
                     'overlay': guest, 'sites': len(sites[target])})
    generic = [r for r in rows if r['kind'] == 'generic']
    protos = index_prototypes({r['name'] for r in generic})
    grades = defaultdict(int)
    for r in generic:
        entry = protos.get(r['name'], {'definitions': {}, 'declarations': set()})
        r['definitions'] = {k: sorted(v) for k, v in sorted(entry['definitions'].items())}
        r['declarations'] = sorted(entry['declarations'])
        r['prototypes'] = sorted(r['definitions']) or r['declarations']
        r['defined'] = bool(r['definitions'])
        r['grade'] = grade(r['prototypes'])
        if r['grade'] == 'conflicting' and r['defined']:
            r['grade'] = 'conflicting-definitions'
        grades[r['grade']] += 1
    summary = {'battle_sha256': BATTLE_SHA256, 'targets': dict(counts),
               'generic_grades': dict(grades),
               'generic_sites': sum(r['sites'] for r in generic),
               'generic_without_definition': sum(not r['defined'] for r in generic),
               'indirect_jalr_sites_not_classified': jalr,
               'scope': 'static direct jal in battle.bin only; no file1/intro/results, no dlsym check'}
    if args.json:
        args.json.write_text(json.dumps({'summary': summary, 'targets': rows}, indent=2) + '\n')
    print(json.dumps(summary, indent=2))
    for g in ('abi-unsafe', 'conflicting-definitions', 'conflicting', 'unknown'):
        for r in generic:
            if r['grade'] == g:
                print('%-12s %s %-32s sites=%-3d %s' % (g, r['target'], r['name'], r['sites'],
                                                        ' | '.join(r['prototypes'])))


if __name__ == '__main__':
    main()
