"""Shared helpers for the decomp-assist scripts (try_func, m2c_try, m2c_insert,
elf_cmp, perm_setup).  No retail data lives here: everything is read from the
splat output under asm/ and the local build/.

Environment:
  XENO_ASSIST_CACHE  scratch directory (default ~/.cache/xeno/decomp_assist)
  XENO_M2C           path to m2c.py (default ~/.cache/xeno/m2c/m2c.py)
  XENO_CC1           cc1 for standalone draft compiles; overrides the one
                     toolchain() resolves from the TU's gears.toml preset
                     (the module-level CC1 default is tools/gcc-2.7.2-psx/cc1)
  XENO_GP_FLAG       -G value; overrides the preset's gp_flag
  XENO_BUILD_WRAP    command prefix that runs a shell string in the build
                     environment, e.g. a docker wrapper "dbuild.sh <repo>";
                     empty (default) runs the command on the host.
"""
import glob
import tomllib
import os
import re
import shlex
import subprocess

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
CACHE = os.path.expanduser(os.environ.get('XENO_ASSIST_CACHE', '~/.cache/xeno/decomp_assist'))
M2C = os.path.expanduser(os.environ.get('XENO_M2C', '~/.cache/xeno/m2c/m2c.py'))
BUILD_WRAP = os.environ.get('XENO_BUILD_WRAP', '')
# cc1 used for standalone draft compiles (match the TU's gears.toml preset)
CC1 = os.environ.get('XENO_CC1', 'tools/gcc-2.7.2-psx/cc1')

# The TU's -G value (gears.toml preset); menu's MenuG0 preset needs -G0.
GP_FLAG = os.environ.get('XENO_GP_FLAG', '-G8')

CC1_FLAGS = ['-O2', GP_FLAG, '-mips1', '-mcpu=3000', '-w', '-funsigned-char', '-fpeephole',
             '-ffunction-cse', '-fpcc-struct-return', '-fcommon', '-msoft-float', '-mgas',
             '-fgnu-linker', '-quiet']
MASPSX_FLAGS = ['--use-comm-section', '--run-assembler', '-EL', '-Iinclude', '-O2', GP_FLAG,
                '-march=r3000', '-mtune=r3000', '-no-pad-sections']



def _strip_toml_comment(line):
    out, quote = [], None
    for ch in line:
        if quote:
            if ch == quote:
                quote = None
        elif ch in '"\'':
            quote = ch
        elif ch == '#':
            break
        out.append(ch)
    return ''.join(out)


def load_gears_toml():
    """gears.toml as a dict.  gears' TOML reader accepts multi-line inline
    tables with comments and trailing commas (`build_variables = {` ... `}`),
    which tomllib rejects, so flatten those onto one line first."""
    text = open(os.path.join(REPO, 'gears.toml')).read()
    out, buf = [], None
    for line in text.split('\n'):
        code = _strip_toml_comment(line).rstrip()
        if buf is None:
            if code.endswith('{') and '}' not in code:
                buf = [code]
            else:
                out.append(line)
            continue
        if code.strip().startswith('}'):
            items = [x.strip().rstrip(',') for x in buf[1:] if x.strip()]
            out.append(buf[0] + ' ' + ', '.join(items) + ' }')
            buf = None
        else:
            buf.append(code)
    return tomllib.loads('\n'.join(out))


def toolchain(ov, seg):
    """cc1, cc1 flags and maspsx flags for src/<ov>/<seg>.c, resolved from
    gears.toml the way gears does: the Default preset's build_variables,
    overridden by the preset whose path key ({binary} expanded) is a
    substring of the TU path (the longest key wins, so the result does not
    depend on table order).  XENO_CC1 / XENO_GP_FLAG still override."""
    cfg = load_gears_toml()
    binary = cfg['compilation']['binary']
    presets = cfg.get('build_preset', [])
    bv = dict(next(p for p in presets if p['name'] == 'Default')['build_variables'])
    tu = f'src/{ov}/{seg}.c'
    best = None
    for p in presets:
        for key in p.get('paths', []):
            k = key.replace('{binary}', binary)
            if k in tu and (best is None or len(k) > len(best[0])):
                best = (k, p)
    name = 'Default'
    if best:
        name = best[1]['name']
        bv.update(best[1].get('build_variables', {}))
    gp = os.environ.get('XENO_GP_FLAG', bv.get('gp_flag', '-G8'))
    opt = bv.get('optimization_flag', '-O2')
    cc1 = os.environ.get('XENO_CC1', f"tools/{bv['gcc']}/cc1")
    cc1_flags = [opt, gp] + bv['cc_flags'].split()
    maspsx = bv['maspsx_flags'].split() + [bv.get('endianness', '-EL'), '-Iinclude', '-I', 'build', opt, gp,
                                           '-march=r3000', '-mtune=r3000', '-no-pad-sections']
    return {'preset': name, 'cc1': cc1, 'cc1_flags': cc1_flags, 'maspsx_flags': maspsx}

def run_build(cmd):
    """Run a shell command in the build environment (host or XENO_BUILD_WRAP)."""
    if BUILD_WRAP:
        return subprocess.run(shlex.split(BUILD_WRAP) + [cmd], capture_output=True, text=True, cwd=REPO)
    return subprocess.run(['bash', '-lc', cmd], capture_output=True, text=True, cwd=REPO)


def load_table(ov):
    """{segment: [ {name, ins:[(offset, word, mask)]} ]} from asm/<ov>/**/*.s.

    The mask hides relocated fields (%hi/%lo/%gp_rel immediates and j/jal
    targets), so a match here is a code match; `make rom-check` is still the
    byte-exact authority (relocation targets are not compared)."""
    table = {}
    for f in glob.glob(os.path.join(REPO, f'asm/{ov}/**/*.s'), recursive=True):
        if 'matchings' in f or '/data/' in f:
            continue
        rel = os.path.relpath(f, os.path.join(REPO, 'asm', ov))[:-2]
        cur = None
        funcs = []
        for line in open(f):
            m = re.match(r'\s*glabel (\S+)', line)
            if m:
                cur = {'name': m[1], 'ins': []}
                funcs.append(cur)
                continue
            m = re.match(r'\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) ([0-9A-F]{8}) \*/\s+(\S+)\s*(.*)', line)
            if m and cur:
                word = int.from_bytes(bytes.fromhex(m[3]), 'little')
                op, args = m[4], m[5]
                mask = 0xFFFFFFFF
                if '%hi(' in args or '%lo(' in args or '%gp_rel' in args:
                    mask = 0xFFFF0000
                elif op in ('jal', 'j'):
                    mask = 0xFC000000
                cur['ins'].append((int(m[1], 16), word, mask))
        table[rel] = funcs
    return table


def func_words(obj, name):
    """(size, [words]) of symbol `name` in an object file's .text, or None."""
    nm = subprocess.run(['mips-linux-gnu-nm', '-S', obj], capture_output=True, text=True).stdout
    sym = [l.split() for l in nm.splitlines() if len(l.split()) == 4 and l.split()[3] == name]
    if not sym:
        return None
    off, size = int(sym[0][0], 16), int(sym[0][1], 16)
    tmp = os.path.join(CACHE, '_text.bin')
    os.makedirs(CACHE, exist_ok=True)
    subprocess.run(['mips-linux-gnu-objcopy', '-O', 'binary', '-j', '.text', obj, tmp])
    data = open(tmp, 'rb').read()
    return size, [int.from_bytes(data[off + 4 * i:off + 4 * i + 4], 'little') for i in range(size // 4)]


def compare(ins, words):
    """Number of masked mismatches, or -1 on a size mismatch."""
    if len(words) != len(ins):
        return -1
    return sum(1 for (o, w, k), b in zip(ins, words) if (b & k) != (w & k))
