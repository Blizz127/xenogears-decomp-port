#!/usr/bin/env python3
"""tu_test.py OV NAME BODY.c [--write] [--source SOURCE.c --tu WRAPPER.c]

Compile a candidate C definition of NAME *inside its own TU*, on the host,
without touching the tree: the TU source is read, NAME's INCLUDE_ASM site
(plain, or the whole `#ifndef XENO_PC_PORT / INCLUDE_ASM / #else <port body>
/ #endif` block) is replaced by BODY.c's text, and the result is
preprocessed (-I include and the TU's own directory), compiled with the TU's
gears.toml preset and compared with the retail words.  This catches the
in-context differences a standalone draft misses (the TU's own types for
globals and callees).  Prints MATCH / diff / size; the object is
$XENO_ASSIST_CACHE/<XENO_CCT_PREFIX or _tut>.o.  `make rom-check` is still
the authority.

--write: on a MATCH, write the edited TU back (BODY.c's `#include "common.h"`
line dropped), so the verified text is exactly what lands.  Only for plain
sites: one with a port body is left alone (m2c_insert.py --inner keeps it).

For split sources, --source selects the included source's real site (which may
use an INCLUDE_ASM path macro), and --tu selects its same-directory wrapper.
The wrapper must include that source exactly once. The edited source is inlined
only in the scratch probe, preserving wrapper defines and compiler selection.
The active-site marker must still survive preprocessing. --write changes only
the selected source and still refuses sites with native bodies."""
import argparse
import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, REPO, compare, func_words, toolchain

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('overlay')
parser.add_argument('name')
parser.add_argument('body')
parser.add_argument('--write', action='store_true')
parser.add_argument('--source', help='included source containing the active candidate site')
parser.add_argument('--tu', help='wrapper translation unit that directly includes --source')
args = parser.parse_args()
if bool(args.source) != bool(args.tu):
    parser.error('--source and --tu must be supplied together')
ov, name, body_path = args.overlay, args.name, os.path.abspath(args.body)
os.chdir(REPO)
body = open(body_path).read()
if args.write:
    body = re.sub(r'^#include "common.h"\n', '', body, flags=re.M).strip('\n') + '\n'
site = re.compile(r'INCLUDE_ASM\("(?:\.\./)?asm/' + re.escape(ov) + r'/nonmatchings/([^"]+)", ' + re.escape(name) + r'\);')
tu = None
if args.source:
    source_path = os.path.realpath(args.source)
    wrapper_path = os.path.realpath(args.tu)
    root = os.path.realpath(f'src/{ov}') + os.sep
    if not source_path.startswith(root) or not wrapper_path.startswith(root):
        parser.error('--source and --tu must be under the selected overlay')
    if os.path.dirname(source_path) != os.path.dirname(wrapper_path) or source_path == wrapper_path:
        parser.error('--source must be a distinct file in the wrapper directory')
    site = re.compile(r'INCLUDE_ASM\((?:"[^"\n]+"|[A-Za-z_]\w*), ' + re.escape(name) + r'\);')
for f in ([args.source] if args.source else glob.glob(f'src/{ov}/**/*.c', recursive=True)):
    s = open(f).read()
    m = site.search(s)
    if m:
        tu = os.path.relpath(args.tu if args.source else f)
        source = os.path.relpath(f)
        seg = tu[len(f'src/{ov}/'):-2] if args.source else m.group(1)
        if args.source and len(list(site.finditer(s))) != 1:
            sys.exit('explicit source must contain exactly one candidate site')
        break
if tu is None:
    sys.exit(f'no INCLUDE_ASM site for {name} under src/{ov}')
st, en = m.start(), m.end()
pre = s[:st].rsplit('\n', 2)
ported = False
if pre[-2].strip() == '#ifndef XENO_PC_PORT' and s[en:].startswith('\n#else\n'):
    st = len(s[:st]) - len(pre[-1]) - len(pre[-2]) - 1
    pos, depth = en + len('\n#else\n'), 0
    while True:
        end = s.index('\n', pos)
        line = s[pos:end].strip()
        if re.match(r'#\s*if', line):
            depth += 1
        elif re.match(r'#\s*endif', line):
            if depth == 0:
                break
            depth -= 1
        pos = end + 1
    en = end
    ported = True
    body = '#ifndef XENO_PC_PORT\n' + body + '\n#endif\n'
edited = s[:st] + body + s[en:]
# A splat-only INCLUDE_ASM list can sit inside #if 0 in an include-wrapper
# TU. Comparing its already-defined real function would falsely admit a draft
# that never reached the compiler. Require the replacement site to be active.
probe_marker = 'xeno_tu_candidate_site_is_active'
s = s[:st] + 'extern int ' + probe_marker + ';\n' + body + s[en:]
if args.source:
    wrapper = open(tu).read()
    includes = []
    for inc in re.finditer(r'^[ \t]*#[ \t]*include[ \t]+"([^"\n]+)"[^\n]*', wrapper, re.M):
        if os.path.realpath(os.path.join(os.path.dirname(tu), inc.group(1))) == source_path:
            includes.append(inc)
    if len(includes) != 1:
        sys.exit('wrapper must directly include the selected source exactly once')
    inc = includes[0]
    s = wrapper[:inc.start()] + '\n' + s + '\n' + wrapper[inc.end():]
# the TU in the matching build: drop other INCLUDE_ASM bodies (SKIP_ASM does that)
pre_ = os.environ.get('XENO_CCT_PREFIX', '_tut')
c_file, i_file, s_file, o_file = (os.path.join(CACHE, pre_ + e) for e in ('.c', '.i', '.s', '.o'))
if os.path.exists(o_file):
    os.remove(o_file)  # Never leave an earlier successful probe after a failure.
open(c_file, 'w').write(s)
tc = toolchain(ov, seg if '/' not in tu[len(f'src/{ov}/'):] else tu[len(f'src/{ov}/'):-2])
p = subprocess.run(['tools/gcc-2.7.2-psx/cpp', '-Iinclude', '-I', os.path.dirname(tu), '-D_LANGUAGE_C', '-DSKIP_ASM',
                    '-P', '-undef', '-lang-c', '-nostdinc', c_file, i_file], capture_output=True, text=True)
if p.returncode:
    sys.exit(p.stderr[-2000:])
if probe_marker not in open(i_file).read():
    sys.exit('inactive INCLUDE_ASM site: candidate was preprocessed out; inspect the included source')
p = subprocess.run([tc['cc1']] + tc['cc1_flags'] + ['-o', s_file, i_file], capture_output=True, text=True)
if p.returncode:
    sys.exit('cc1-fail: ' + '\n'.join(p.stderr.strip().split('\n')[-5:]))
if os.path.exists(o_file):
    os.remove(o_file)
subprocess.run(['python3', 'tools/maspsx/maspsx.py'] + tc['maspsx_flags'] + ['-o', o_file, s_file],
               capture_output=True, stdin=subprocess.DEVNULL)
asm = glob.glob(f'asm/{ov}/**/{name}.s', recursive=True)[0]
ins = []
in_func = False  # only the function's own .text words: a .rodata jump table in the same file is not code
for line in open(asm):
    if re.match(r'\s*glabel ', line) or line.startswith('.section'):
        in_func = bool(re.match(r'\s*glabel ' + re.escape(name) + r'\b', line))
        continue
    mm = re.match(r'\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) ([0-9A-F]{8}) \*/\s+(\S+)\s*(.*)', line)
    if mm and in_func:
        word = int.from_bytes(bytes.fromhex(mm[3]), 'little')
        mask = 0xFFFFFFFF
        rel = None  # (kind, symbol, addend) the row relocates against, if any
        r = re.search(r'%(hi|lo)\(([A-Za-z_]\w*)\s*([+-]\s*0x[0-9A-Fa-f]+|[+-]\s*\d+)?\)', mm[5])
        if '%hi(' in mm[5] or '%lo(' in mm[5] or '%gp_rel' in mm[5]:
            mask = 0xFFFF0000
            if r:
                rel = (r[1].upper() + '16', r[2], int(r[3].replace(' ', ''), 0) if r[3] else 0)
        elif mm[4] in ('jal', 'j'):
            mask = 0xFC000000
            if mm[4] == 'jal' and re.match(r'[A-Za-z_]\w*$', mm[5].strip()):
                rel = ('26', mm[5].strip(), 0)
        ins.append((int(mm[1], 16), word, mask, rel))
got = func_words(o_file, name)
if not got:
    sys.exit('no symbol')
bad = compare([i[:3] for i in ins], got[1])


def reloc_mismatches(obj, name, ins, words):
    """Rows whose relocation symbol (and, for %lo with a derivable symbol
    address, addend) differs from retail.  compare() masks these fields, so
    two swapped globals or a wrong struct offset would otherwise pass."""
    nm = subprocess.run(['mips-linux-gnu-nm', '-S', obj], capture_output=True, text=True).stdout
    text = {}
    base = size = None
    for l in nm.splitlines():
        p = l.split()
        if len(p) == 4 and p[2] in 'tT':
            text[int(p[0], 16)] = p[3]
            if p[3] == name:
                base, size = int(p[0], 16), int(p[1], 16)
    if base is None:
        return []
    rels = {}
    sect = None
    for l in subprocess.run(['mips-linux-gnu-objdump', '-r', obj], capture_output=True, text=True).stdout.splitlines():
        m = re.match(r'RELOCATION RECORDS FOR \[(\S+)\]', l)
        if m:
            sect = m[1]
            continue
        m = re.match(r'([0-9a-f]+)\s+R_MIPS_(HI16|LO16|26)\s+(\S+)', l)
        if m and sect == '.text':
            off = int(m[1], 16)
            if base <= off < base + size:
                sym = m[3]
                mt = re.match(r'\.text([+-]0x[0-9a-f]+)?$', sym)
                if mt:  # a same-TU callee or label: name it
                    sym = text.get(int(mt[1] or '0', 16), sym)
                rels[off - base] = (m[2], sym)
    out = []
    for i, (off, word, mask, rel) in enumerate(ins):
        if rel is None:
            continue
        kind, sym, add = rel
        got_rel = rels.get(i * 4)
        if got_rel is None or got_rel[0] != kind:
            continue  # local label or section-relative target: not comparable
        bsym = got_rel[1].split('+')[0].split('-')[0]
        if bsym.startswith('.'):
            continue  # section-relative (jump table label etc.): not comparable by name
        badd = (words[i] & 0xFFFF) if kind == 'LO16' else 0
        if badd >= 0x8000:
            badd -= 0x10000
        addr = lambda n: int(n[n.rindex('_') + 1:], 16) if re.match(r'(?:D|func|jtbl)_[0-9A-Fa-f]{8}$', n) else None
        ra, ba = addr(sym), addr(bsym)
        if ra is not None and ba is not None and kind == 'LO16':
            # same address spelt differently (D_x+4 vs D_x4) links identically
            if ra + add != ba + badd:
                out.append((off, f'%lo({sym}{add:+#x}) vs %lo({bsym}{badd:+#x})'))
        elif ra is not None and ba is not None and kind == 'HI16':
            if ra != ba and sym != bsym and abs(ra - ba) >= 0x10000:
                out.append((off, f'{sym} vs {bsym}'))
        elif bsym != sym:
            out.append((off, f'{sym} vs {bsym}'))
    return out


relbad = reloc_mismatches(o_file, name, ins, got[1]) if bad == 0 else []
if bad == 0 and relbad:
    bad = len(relbad)
    print(f"{tc['preset']}: code MATCH but {len(relbad)} relocation row(s) differ, first at +{relbad[0][0]:#x}: {relbad[0][1]}")
else:
    print(f"{tc['preset']}:", 'MATCH' if bad == 0 else (f'diff {bad}/{len(ins)}' if bad > 0 else f'size {got[0]} vs {len(ins) * 4}'))
if bad == 0 and args.write and ported:
    sys.exit('not written: the site has a port body (use m2c_insert.py --inner)')
if bad == 0 and args.write:
    open(source, 'w').write(edited)
    print('wrote', source)
