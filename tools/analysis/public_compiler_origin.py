"""Check the game compiler inputs actually recorded for a paired public release."""
import argparse
import functools
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys

from public_world_cohort import BODIES, WITHDRAWN, token_reason

HERE = Path(__file__).resolve().parent
SYMBOL = rb'[A-Za-z_][A-Za-z_0-9]*'
HEX = re.compile(r'[0-9a-f]{64}')
BRIDGE_HEAD = re.compile(
    rb'/\* Generated from retail symbol maps; do not edit\. \*/\n'
    rb'typedef struct \{\n    uint32_t address, size;\n    uint8_t is_function;\n'
    rb'    char name\[[1-9][0-9]*\];\n\} PcPortInlineBattleSymbol;\n'
    rb'static const PcPortInlineBattleSymbol g_BattleBridgeSymbols\[\] HOST_ONLY_RODATA = \{\n')
BRIDGE_LINE = re.compile(rb'    \{ 0x[0-9a-f]{8}u, [0-9]+u, [01], "' + SYMBOL + rb'" \},')
BRIDGE_END = b'};\nstatic const size_t g_BattleBridgeSymbolCount = sizeof(g_BattleBridgeSymbols) / sizeof(g_BattleBridgeSymbols[0]);\n'

def require(condition, message):
    if not condition:
        raise ValueError(message)

def digest(data):
    return hashlib.sha256(data).hexdigest()

def git(repo, *args):
    return subprocess.check_output(['git', '-C', repo, *args])

def file_pin(base, record):
    path = (base / record['path']).resolve(strict=True)
    require(path.is_relative_to(base.resolve()), f'manifest file outside build directory: {path}')
    data = path.read_bytes()
    require(len(data) == record['bytes'] and digest(data) == record['sha256'], f'changed build input: {path}')
    return data

def safe_path(rel):
    p = PurePosixPath(rel)
    return bool(rel) and not p.is_absolute() and '..' not in p.parts and p.as_posix() == rel

def generated_allowed(name, data, unit, reviewed_stubs):
    if name == 'stubs.c':
        return unit == 'stubs.o' and digest(data) == reviewed_stubs
    if name == 'battle_bridge_map.inc':
        head = BRIDGE_HEAD.match(data)
        if not head or not data.endswith(BRIDGE_END):
            return False
        rows = data[head.end():-len(BRIDGE_END)].splitlines()
        return bool(rows) and all(BRIDGE_LINE.fullmatch(row) for row in rows)
    if name == 'battle_namespace.h':
        pattern = re.compile(rb'#define (' + SYMBOL + rb') battle_\1')
    elif name == 'battle_win32_binding_sizes.inc' and unit == 'battle_win32_host.c.o':
        pattern = re.compile(rb'\{"' + SYMBOL + rb'", [1-9][0-9]*\},')
    elif name == unit + '.h':
        pattern = re.compile(rb'#pragma weak ' + SYMBOL)
    else:
        return False
    return all(pattern.fullmatch(line) for line in data.splitlines())

LEX = re.compile(r'''(?P<string>(?:u8|[uUL])?"(?:\\.|[^"\\])*"|(?:u8|[uUL])?'(?:\\.|[^'\\])*')|(?P<comment>/\*.*?\*/|//[^\n]*)|(?P<directive>^[ \t]*\#[^\n]*)|(?P<space>[^\S\n]+|\n)|(?P<id>[A-Za-z_]\w*)|(?P<num>(?:\.\d|\d)(?:[eEpP][+-]|[\w.])*)|(?P<op>\.\.\.|>>=|<<=|->|>>|<<|\+\+|--|&&|\|\||[=!<>+*/%&|^\-]=|\#\#|.)''', re.S | re.M)

def body_findings(data, signatures):
    # Compare the interior of balanced C blocks. File names, function names,
    # whitespace and comments do not identify independently written bodies.
    text = data.decode('utf-8', errors='replace').replace('\\\r\n','').replace('\\\n','')
    tokens = [m[0] for m in LEX.finditer(text) if m.lastgroup not in ('space','comment','directive')]
    sizes = {size for size, _ in signatures.values()}
    starts, found = [], set()
    for idx,token in enumerate(tokens):
        if token == '{':
            starts.append(idx+1)
        elif token == '}' and starts:
            start = starts.pop()
            if idx-start in sizes:
                sha = digest(b'\0'.join(x.encode() for x in tokens[start:idx]))
                if sha in signatures:
                    found.add(signatures[sha][1])
    return found

def check(args):
    repo = args.project.resolve(strict=True)
    commit = git(repo, 'rev-parse', '--verify', args.commit + '^{commit}').decode().strip()
    tree = git(repo, 'rev-parse', commit + '^{tree}').decode().strip()
    @functools.cache
    def blob(path):
        require(safe_path(path), f'invalid project input {path!r}')
        return git(repo, 'show', f'{commit}:{path}')
    linux_profile = json.loads(blob('pc_port/tools/public_linux/linux-build.json'))
    windows_profile = blob('pc_port/tools/win64/compile-profile.json')
    policy = json.loads((HERE/'public_compiler_origin.json').read_bytes())
    denied = {**policy['translated_sources'], **{sig: 'withdrawn World release input: '+path for sig,path in WITHDRAWN.items()}}
    body_rules = {**policy['body_digests'], **BODIES}
    blocked_by_unit = {}
    @functools.cache
    def project_sha(path):
        return digest(blob(path))
    @functools.cache
    def project_findings(path):
        return (body_findings(blob(path),body_rules) | set(token_reason(blob(path)))) if Path(path).suffix.lower() in ('.c','.h','.cpp','.inc','.cc','.cxx','.hpp') else set()
    linux = json.loads(args.linux.read_bytes())
    windows = json.loads(args.windows.read_bytes())
    linked = json.loads(args.windows_link.read_bytes())
    require(linked['mode']=='full' and linked['source']==commit and linked['tree']==tree,
            'incorrect or partial Windows link receipt')
    for key,path in (('objects',args.windows),('linux',args.linux)):
        link_pin = linked[key]
        require(link_pin['sha256']==digest(path.read_bytes()) and link_pin['size']==path.stat().st_size,
                'Windows link used a different producer: '+key)
    def image_hash(base, pin):
        path=(base/pin['path']).resolve(strict=True)
        require(path.is_relative_to(base.resolve()) and path.stat().st_size==pin['bytes'],
                f'wrong release image: {path}')
        with path.open('rb') as fp:
            actual=hashlib.file_digest(fp,'sha256').hexdigest()
        require(actual==pin['sha256'],f'release image changed: {path}')
        return actual
    images={'linux':image_hash(args.linux.parent,linux['release']),
            'windows':image_hash(args.windows_link.parent,linked['executable'])}

    require(all(p['source'] == commit and p['tree'] == tree for p in (linux, windows)),
            'producer revisions differ')
    require(windows['mode'] == 'full' and windows['profile']['sha256'] == digest(windows_profile),
            'incomplete or different Windows compiler profile')
    require(windows['linux']['sha256'] == digest(args.linux.read_bytes()), 'Windows used a different Linux producer')
    rosters = {'linux':{k:v['source'] for k,v in linux_profile['units'].items()},
               'windows':{k:src for k,src,_ in json.loads(windows_profile)['units']}}
    require(set(linux['objects']) == rosters['linux'].keys()
            and linux['game_order'] == [v.removeprefix('@obj@/') for v in linux_profile['link'] if v.endswith('.o')]
            and set(linux['game_order']) | set(linux_profile['renderer_order']) == set(linux['objects']),
            'incomplete Linux object roster')
    require([r['name'] for r in windows['objects']] == list(rosters['windows'])
            and list(rosters['windows']) == linux['game_order'] + ['battle_win32_host.c.o'],
            'incomplete or mismatched Windows object order')
    def inspect(osname, unit, direct, inputs, base, gen, direct_hash=None):
        require(inputs and direct == rosters[osname][unit], f'{unit}: incorrect source or empty dependencies')
        direct_pred = ('generated/stubs.c' if unit == 'stubs.o' else
                       'renderer/'+direct.removeprefix('@renderer@/') if direct.startswith('@renderer@/') else
                       ('project/' if osname == 'linux' else 'source/') + direct)
        if direct_pred not in inputs:
            blocked_by_unit.setdefault((osname,unit),set()).add('uncommitted direct input: '+direct)
        elif direct_hash is not None:
            require(inputs[direct_pred] == direct_hash, f'{unit}: inconsistent direct source hash')
        for path, sha in inputs.items():
            require(HEX.fullmatch(sha), f'{unit}: invalid SHA-256')
            key = (osname,unit)
            if sha in denied:
                blocked_by_unit.setdefault(key,set()).add(path+' ('+denied[sha]+')')
            group,_,rel = path.partition('/')
            require(safe_path(rel),f'{unit}: invalid dependency {path}')
            if group in ('project','source','renderer-companion'):
                require(project_sha(rel) == sha, f'{unit}: compiler input differs from revision: {path}')
                for family in project_findings(rel):
                    blocked_by_unit.setdefault(key,set()).add(path+' ('+family+')')
            elif group == 'generated':
                if rel not in gen or gen[rel]['sha256'] != sha:
                    blocked_by_unit.setdefault(key,set()).add('unregistered generated input: '+path)
                else:
                    data = generated_bytes(str(base),rel,sha,gen[rel]['path'],gen[rel]['bytes'])
                    if not generated_allowed(rel,data,unit,policy['host_stubs_sha256']):
                        blocked_by_unit.setdefault(key,set()).add('unqualified generated source or include: '+path)
            else:
                allowed = {'renderer','toolchain'} if osname == 'linux' else {'renderer','external','toolchain'}
                require(group in allowed, f'{unit}: unknown dependency origin {group}')
    @functools.lru_cache(maxsize=512)
    def generated_bytes(base, name, sha, path, byte_count):
        return file_pin(Path(base),dict(path=path,bytes=byte_count,sha256=sha))
    for unit, obj in linux['objects'].items():
        row = json.loads(file_pin(args.linux.parent,obj['build']))
        require(row['object']==unit, f'{unit}: mismatched compiler receipt')
        inspect('linux',unit,row['direct_input'],row['inputs'],args.linux.parent,linux['generated'])
    for row in windows['objects']:
        inspect('windows',row['name'],row['source'],row['dependencies'],args.windows.parent,
                windows['generated'],row['source_sha256'])
    return dict(commit=commit,tree=tree,linux_receipt=digest(args.linux.read_bytes()),
                windows_receipt=digest(args.windows.read_bytes()),windows_link=digest(args.windows_link.read_bytes()),
                binaries=images,units={k:len(v) for k,v in rosters.items()},
                findings=[dict(platform=k,object=n,inputs=sorted(v)) for (k,n),v in sorted(blocked_by_unit.items())])

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project',type=Path,required=True)
    p.add_argument('--commit',required=True)
    p.add_argument('--linux',type=Path,required=True,help='Linux linux-link.json')
    p.add_argument('--windows',type=Path,required=True,help='Windows windows-compile.json')
    p.add_argument('--windows-link',type=Path,required=True,help='Windows windows-link.json')
    try:
        report=check(p.parse_args())
        print(json.dumps(report,indent=2))
        sys.exit(int(bool(report['findings'])))
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as err:
        sys.exit('Compiler origin check failed: '+str(err))
