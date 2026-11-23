"""Link a matching set of Windows COFF objects in r7.21's verified ELF object order."""
import argparse
import json
import shutil
import re
import shlex
import struct
import subprocess
import tempfile
from pathlib import Path
from pe_compact import check
from battle_binding_audit import verify as verify_battle_bindings
from battle_function_audit import verify as verify_battle_functions

def nm_symbols(inputs, dynamic=False):
    tool = shutil.which('llvm-nm-21') or shutil.which('llvm-nm')
    if not tool: raise ValueError('llvm-nm is required for both COFF and ELF definitions')
    argv=[tool,'--extern-only','--defined-only','--format=posix']
    if dynamic: argv+=['--dynamic']
    result={}
    for start in range(0,len(inputs),100):
        text=subprocess.check_output(argv+[str(p) for p in inputs[start:start+100]],text=True)
        for line in text.splitlines():
            match=re.fullmatch(r'([A-Za-z_][A-Za-z0-9_]*) ([TDBRSGCVW]) [0-9a-fA-F]+ [0-9a-fA-F]+',line)
            if match:
                name,kind=match.groups()
                if kind != 'W' or name not in result:
                    result[name]=kind
    return result

def crt_options(zig,scratch,base,syslibs):
    dummy=scratch/'crt-probe.c'
    dummy.write_text('int main(void) { return 0; }\n')
    subprocess.run([zig,'cc','-target','x86_64-windows-gnu','-O2','-c',str(dummy),'-o',str(scratch/'crt-probe.o')],check=True)
    argv=[zig,'build-exe','-OReleaseFast','-target','x86_64-windows-gnu','--verbose-link','--image-base',str(base),'-lc']
    result=subprocess.run(argv+['-l'+lib for lib in syslibs]+['crt-probe.o','-femit-bin=crt-probe.exe'],cwd=scratch,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if result.returncode: raise ValueError(result.stdout)
    lines=[shlex.split(row) for row in result.stdout.splitlines() if row.startswith('lld-link ')]
    if len(lines)!=1: raise ValueError('no unique Zig LLD invocation')
    row=lines[0]
    split=row.index('-lldmingw')
    before=[x for x in row[1:split] if x.startswith('-') and not x.upper().startswith(('-OUT:','-PDB','-DEBUG','-IMPLIB:'))]
    return before,row[split:]

def audit_exports(binary, names, wraps, known):
    tool=shutil.which('llvm-readobj-21') or shutil.which('llvm-readobj')
    if not tool: raise ValueError('llvm-readobj is required for the linked PE export check')
    output=subprocess.check_output([tool,'--coff-exports','--coff-imports',str(binary)],text=True)
    exports={name:int(rva,16) for name,rva in re.findall(r'  Name: ([^\n]+)\n  RVA: (0x[0-9A-Fa-f]+)',output)}
    # COFF weak externals can silently resolve to absolute zero. With a low
    # image base their exported RVA then wraps near the top of 32-bit memory.
    image = Path(binary).read_bytes()
    pe = struct.unpack_from("<I", image, 0x3c)[0]
    sections = struct.unpack_from("<H", image, pe + 6)[0]
    optional_size = struct.unpack_from("<H", image, pe + 20)[0]
    start = pe + 24 + optional_size
    ranges = []
    for index in range(sections):
        entry = start + index * 40
        size, rva = struct.unpack_from("<II", image, entry + 8)
        readable = struct.unpack_from("<I", image, entry + 36)[0] & 0x40000000
        if readable:
            ranges.append((rva, rva + size))
    for name, rva in exports.items():
        if not any(lo <= rva < hi for lo, hi in ranges):
            raise ValueError(f"PE export {name} at {rva:#x} falls outside mapped sections")
    missing=set(names)-set(exports)
    if missing: raise ValueError('PE export table lacks '+', '.join(sorted(missing)[:12]))
    imports=set(re.findall(r'  Symbol: (\w+) \(\d+\)',output))
    for sym in wraps:
        wrapper='__wrap_'+sym
        if sym not in exports or wrapper not in exports or exports[sym]==exports[wrapper] or sym in imports:
            raise ValueError(f'{sym}: distinct SDK and port wrapper exports required; SDK may not be imported')
    print(f'{len(names)} exported functions/data checked against the PE table; {len(wraps)} distinct SDK/wrapper pairs')
    return exports

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--link-args',type=Path,required=True,help='verified Linux xeno-port.link-args, including SDK wrap inventory')
    parser.add_argument('--order',type=Path,required=True,help='target linker object list, in verified order')
    parser.add_argument('--prefix',type=Path,required=True,help='Windows SDL2/OpenAL dependency prefix')
    parser.add_argument('--elf',type=Path,required=True)
    parser.add_argument('--objects-dir','--obj-dir',dest='obj_dir',type=Path,required=True)
    parser.add_argument('--archive','--sdk',dest='sdk',type=Path,required=True)
    parser.add_argument('--library',action='append',type=Path,default=[])
    parser.add_argument('--object',action='append',type=Path,default=[])
    parser.add_argument('--system-lib',action='append',default=[])
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--base',default='0x100000',type=lambda x:int(x,0))
    parser.add_argument('--zig',default='zig')
    args=parser.parse_args()
    rows=args.link_args.read_text().splitlines()
    objects=[args.obj_dir/Path(row).name for row in rows if row.endswith('.o')]
    ordered=[item.strip() for item in args.order.read_text().splitlines() if item.strip()]
    windows_bridge = args.obj_dir / 'battle_win32_host.c.o'
    objects.append(windows_bridge)
    if ordered != [p.name for p in objects]: raise ValueError('Windows object order differs from verified Linux link manifest')
    objects+=args.object
    if len({p.name for p in objects})!=len(objects): raise ValueError('duplicate object basename in ordered manifest')
    libraries=[args.sdk,args.prefix/'lib/libSDL2.dll.a',args.prefix/'lib/libOpenAL32.dll.a',*args.library]
    for path in [*objects,*libraries]:
        if not path.is_file(): raise FileNotFoundError(path)
    if not objects: raise ValueError('empty object manifest')
    wraps=[row.removeprefix('-Wl,--wrap=') for row in rows if row.startswith('-Wl,--wrap=')]
    windows_wraps=[x for x in wraps if x not in ('asinf','acosf','sqrtf')]
    selected=nm_symbols(objects+[args.sdk])
    absent=[sym for sym in windows_wraps if '__wrap_'+sym not in selected or sym not in selected]
    if absent: raise ValueError('missing wrapper or SDK definition: '+', '.join(absent))
    elf=nm_symbols([args.elf],dynamic=True)
    if "XenoPsyQEnterCriticalSection" not in selected:
        raise ValueError("missing guest interrupt mask implementation")
    # The guest map looks up the retail spelling; OS calls use the distinct
    # kernel32 import table entry. Only this exported name is an alias.
    aliases = {"EnterCriticalSection": "XenoPsyQEnterCriticalSection"}
    selected.update({alias: selected[target] for alias, target in aliases.items()})
    exports=sorted(set(elf)&set(selected))
    platform_exports = {"_IO_stdin_used", "__bss_start", "__data_start", "_edata", "_end", "_start",
                        "data_start", "__wrap_acosf", "__wrap_asinf", "__wrap_sqrtf"}
    missing = sorted(n for n in set(elf) - set(selected) if n not in platform_exports and not n.startswith("_Z"))
    if missing:
        raise ValueError("Windows lacks game runtime exports: " + ", ".join(missing))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    out=args.output.resolve()
    defs=out.with_suffix('.def')
    defs.write_text('EXPORTS\n'+''.join('  '+s+('='+aliases[s] if s in aliases else '')+(' DATA' if elf[s] in 'BDRSCGV' else '')+'\n' for s in exports))
    out.with_suffix('.missing-elf-exports').write_text('\n'.join(sorted(set(elf)-set(exports)))+'\n')
    files=[str(x.resolve()) for x in objects+libraries]
    log=out.with_suffix('.link.log')
    with tempfile.TemporaryDirectory(prefix='xeno-coff-crt-',dir=out.parent) as temp:
        system_libs=list(dict.fromkeys(['opengl32','user32','bcrypt','gdi32','winmm','ws2_32','ole32','shell32','imm32','version','setupapi','advapi32',*args.system_lib]))
        options,crt=crt_options(args.zig,Path(temp),args.base,system_libs)
        # Passing separated argv to LLD avoids shell interpretation and avoids embedding private source paths in a PDB.
        argv=[args.zig,'lld-link',*options,'/debug:dwarf','/fixed','/dynamicbase:no','/highentropyva:no','/opt:noref','/timestamp:0','/subsystem:console',
              '/map:'+str(out.with_suffix('.map')),'/lldmap:'+str(out.with_suffix('.lldmap')),'/def:'+str(defs),'/out:'+str(out)]
        argv+=['/wrap:'+s for s in windows_wraps]+files+crt
        out.with_suffix('.link.json').write_text(json.dumps(argv,indent=2)+'\n')
        proc=subprocess.run(argv,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        log.write_text(proc.stdout)
        if proc.returncode: raise SystemExit('link failed: '+str(log)+'\n'+proc.stdout[:1800])
    print(f"{len(objects)} pinned objects, {len(windows_wraps)} SDK wrappers, {len(exports)} native symbol exports")
    actual_exports = audit_exports(out,exports,windows_wraps,selected)
    bridge_map = args.link_args.parent / "battle_bridge_map.inc"
    print(verify_battle_functions(out, actual_exports, elf, bridge_map))
    print(verify_battle_bindings(out, actual_exports, bridge_map,
                                 out.with_suffix(".lldmap"), args.order.parent / "battle_win32_binding_evidence.json"))
    print(check(out,out.with_suffix('.lldmap'),objects))

if __name__=='__main__':
    main()
