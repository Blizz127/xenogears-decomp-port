#!/usr/bin/env python3
"""Reconstruct the measured Windows SDL/OpenAL build prefix from public sources."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import subprocess
import tarfile
import tempfile
import zipfile

HEADER_MANIFEST_HASH = 'faeaaf301b300c528e6805afb9bad3eec69c5840b9d0bfbdf64ac6a4293fc8f6'
PUBLIC_PACKAGE_HASH = '8f311b452777060167507b1ad6a59fdb7ceb1ba7352e2785da183ca5205a7b86'
RUNTIMES = {
    'SDL2.dll': ('a2648c887820d882805d00c40c30181a21dbb083f24cfe7ab6cc3e451a55a3b6', '29def8cd8385ffbbf4e7ac6e47359d1213b70c6aed0c282e809ca35bd33c9b37', 842),
    'OpenAL32.dll': ('6aa458bc42bc04683126d40f94c853895f953387587a56cb29d61de0a70c1aee', 'e9fe9cd8e500774102fc4a51d55959eefabd451e7415651e1ef75fd7fa5b15ef', 201),
}

def checked(data, digest, name):
    if hashlib.sha256(data).hexdigest() != digest:
        raise ValueError(f'SHA-256 mismatch for {name}')
    return data

def archive_member(archive, name):
    with archive.extractfile(name) as member:
        return member.read()

def configure_sdl(template, profile, work):
    (work/'SDL_config.h.cmake').write_bytes(checked(template, profile['template_sha256'], 'SDL config template'))
    variables = profile['cmake_variables']
    script = ''.join(f'set({var} {val})\n' for var, val in sorted(variables.items()))
    script += 'configure_file(SDL_config.h.cmake SDL_config.h @ONLY)\n'
    (work/'profile.cmake').write_text(script)
    subprocess.run(['cmake', '-P', 'profile.cmake'],cwd=work,check=True)
    return (work/'SDL_config.h').read_bytes()

def create_import_library(dll, raw, count, target, work, zig, readobj):
    table = subprocess.check_output([readobj, '--coff-exports', dll],text=True)
    entries = re.findall(r'^Export \{\n  Ordinal: (\d+)\n  Name: *(\S+)\n  RVA: (0x[0-9A-Fa-f]+)\n\}$',table,re.M)
    if not (len(entries)==count==table.count('Export {') and
            [int(i) for i,_,_ in entries]==list(range(1,count+1)) and
            len({n for _,n,_ in entries})==count):
        raise ValueError(f'Incomplete export table for {dll.name}')
    nt = struct.unpack_from('<I',raw,0x3c)[0]
    nsections = struct.unpack_from('<H',raw,nt+6)[0]
    optional_size = struct.unpack_from('<H',raw,nt+20)[0]
    sections = []
    for index in range(nsections):
        head = nt+24+optional_size+40*index
        size,start = struct.unpack_from('<II',raw,head+8)
        flags = struct.unpack_from('<I',raw,head+36)[0]
        sections.append((start,start+size,bool(flags & 0x20000000)))
    names = []
    for _,name,rva in entries:
        matches = [code for start,end,code in sections if start<=int(rva,16)<end]
        if len(matches)!=1:
            raise ValueError(f'Unmapped or ambiguous export in {dll.name}')
        names.append(name if matches[0] else name+' DATA')
    definition = work/(dll.name+'.def')
    definition.write_text('LIBRARY '+dll.name+'\nEXPORTS\n'+'\n'.join(sorted(names))+'\n')
    subprocess.run([zig,'dlltool','-m','i386:x86-64','-d',definition,'-l',target],check=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package',type=Path,help='public v0.7.93-preview Windows zip')
    parser.add_argument('crate',type=Path,help='public sdl2-sys 0.38.0 crate')
    parser.add_argument('output',type=Path,help='a new directory for this prefix')
    parser.add_argument('--headers',type=Path,default=Path(__file__).with_name('windows-external-headers.json'))
    parser.add_argument('--zig',default='zig')
    parser.add_argument('--readobj',default='llvm-readobj-21')
    args=parser.parse_args()
    if args.output.exists():
        parser.error('output must be a new directory')
    if subprocess.check_output([args.zig,'version'],text=True).strip()!='0.13.0':
        parser.error('Zig 0.13.0 is required')
    manifest=json.loads(checked(args.headers.read_bytes(), HEADER_MANIFEST_HASH, 'headers manifest'))
    crate=checked(args.crate.read_bytes(), manifest['sdl_archive_sha256'], 'SDL source crate')
    package=checked(args.package.read_bytes(), PUBLIC_PACKAGE_HASH, 'public release ZIP')
    with tempfile.TemporaryDirectory(dir=args.output.parent) as tmp:
        work=Path(tmp)
        prefix=work/'prefix'
        for part in ['include/SDL2','include/AL','bin','lib']:
            (prefix/part).mkdir(parents=True)
        with zipfile.ZipFile(io.BytesIO(package)) as z:
            blob=checked(z.read('sources/openal-soft-1.25.1-source.tar.bz2'),manifest['openal_public_companion_sha256'],'OpenAL source')
            with tarfile.open(fileobj=io.BytesIO(blob)) as al, tarfile.open(fileobj=io.BytesIO(crate)) as sdl:
                for key,profile in manifest['sources'].items():
                    component,name=key.split('/',1)
                    raw=archive_member(sdl if component=='SDL' else al,profile['archive_member'])
                    if key=='SDL/SDL_config.h':
                        raw=configure_sdl(raw,profile,work)
                    name='SDL2/'+name if component=='SDL' else name
                    (prefix/'include'/name).write_bytes(checked(raw,profile['sha256'],key))
            for name,(dll_hash,import_hash,count) in RUNTIMES.items():
                raw=checked(z.read('runtime/'+name),dll_hash,name)
                dll=prefix/'bin'/name
                dll.write_bytes(raw)
                library=prefix/'lib'/('lib'+name+'.a')
                create_import_library(dll,raw,count,library,work,args.zig,args.readobj)
                checked(library.read_bytes(),import_hash,library.name)
        prefix.rename(args.output)
    print(f'Verified {len(manifest["sources"])} headers, 2 DLLs, and 2 native import libraries at {args.output}')

if __name__=='__main__':
    main()
