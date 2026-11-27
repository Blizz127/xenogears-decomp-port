#!/usr/bin/env python3
"""Stage or verify the standalone Linux and Windows packages from one source revision."""
import argparse
import fcntl
import io
import json
import os
from pathlib import Path
import re
import stat
import struct
import subprocess
import sys
import tarfile
import tempfile
import zipfile

sys.dont_write_bytecode = True

from public_release import companion
from public_release.restore_renderer import check, git_blob, name, sha

ROOT = Path(__file__).resolve().parents[2]
ANALYSIS = 'tools/analysis/'
sys.path.insert(0, str(ROOT / ANALYSIS))
from public_digest_packaging import check_packaging
from public_guard_archives import archive_inputs

PROFILE = 'pc_port/tools/public_release/'
COMMANDS = {
    'linux': '```sh\npython3 prepare-data/prepare.py "/path/to/Disc 1.bin" "$HOME/Xenogears Data"\n./xenogears.sh "$HOME/Xenogears Data"\n```',
    'windows': '```powershell\npy -3 .\\prepare-data\\prepare.py "D:\\My Discs\\Disc 1.bin" "$env:LOCALAPPDATA\\Xenogears Data"\npy -3 .\\start-windows.py "$env:LOCALAPPDATA\\Xenogears Data"\n```',
}


def executable(platform, data):
    if platform == 'linux':
        check(len(data)>=64 and data.startswith(b'\x7fELF\x02\x01\x01') and struct.unpack_from('<HH',data,16) in ((2,62),(3,62)), 'Expected x86-64 Linux ELF')
        entry, table = struct.unpack_from('<QQ',data,24)
        stride, count = struct.unpack_from('<HH',data,54)
        check(count and stride>=56 and table+count*stride<=len(data), 'Invalid ELF program headers')
        for i in range(count):
            typ, flags, off, va, _, filesz, memsz, _ = struct.unpack_from('<II6Q',data,table+i*stride)
            if typ==1 and flags&1 and va<=entry<va+filesz and filesz<=memsz and off+filesz<=len(data):
                return
        raise ValueError('Linux entry point is outside executable file data')
    check(len(data)>=64 and data[:2]==b'MZ', 'Expected Windows PE executable')
    pe = struct.unpack_from('<I',data,60)[0]
    check(pe+24<=len(data) and data[pe:pe+6]==b'PE\0\0\x64\x86', 'Expected AMD64 PE')
    sections = struct.unpack_from('<H',data,pe+6)[0]
    size, flags = struct.unpack_from('<HH',data,pe+20)
    hdr = pe+24
    check(size>=112 and flags&2 and not flags&0x2000 and hdr+size+40*sections<=len(data) and data[hdr:hdr+2]==b'\x0b\x02', 'Expected a PE32+ application')
    entry = struct.unpack_from('<I',data,hdr+16)[0]
    for i in range(sections):
        base = hdr+size+i*40
        vsize, va, raw, off = struct.unpack_from('<4I', data,base+8)
        flags = struct.unpack_from('<I',data,base+36)[0]
        if flags & 0x20000000 and flags & 0x20 and va<=entry<va+min(vsize or raw,raw) and off+raw<=len(data):
            return
    raise ValueError('Windows entry point is outside executable code')


def pinned(base,pin):
    path = base/Path(pin['path'])
    data = path.read_bytes()
    check(sha(data)==pin['sha256'] and len(data)==pin['bytes'], 'Changed build product: '+str(path))
    return data


def verify_members(sdk, proof):
    """Bind the producer's per-member dependency union to the exact linked .a."""
    check(sdk.startswith(b'!<arch>\n'), 'Expected a complete static renderer archive')
    with tempfile.TemporaryDirectory() as tmp:
        archive = Path(tmp)/'renderer.a'
        archive.write_bytes(sdk)
        actual = subprocess.check_output(['ar','t',str(archive)], text=True).splitlines()
        members = proof['members']
        hashes = proof['member_sha256']
        units = proof['units']
        check(isinstance(members,list) and members and len(actual)==len(set(actual)) and
              len(members)==len(set(members)) and set(actual)==set(members)==set(hashes)==set(units),
              'Renderer archive and compiler member rosters disagree')
        union = set()
        for member in actual:
            check(name(member)==Path(member).name and member.endswith(('.o','.obj')), 'Invalid object member name')
            data = subprocess.check_output(['ar','p',str(archive),member])
            check(data and sha(data)==hashes[member], 'Renderer object digest mismatch: '+member)
            deps=units[member]
            check(isinstance(deps,list) and deps and len(deps)==len(set(deps)), 'Invalid dependency list: '+member)
            union.update(deps)
        check(union==set(proof['compiler_inputs']), 'Renderer dependency union differs from the measured inputs')


def read_support(archive,spec,platform):
    raw = archive.read_bytes()
    check(sha(raw)==spec['archive_sha256'] and len(raw)==spec['archive_bytes'],platform+' public support archive digest mismatch')
    # Match the complete published archive and read only the approved support members.
    expected = dict(spec['members'])
    expected[spec['renderer']]=spec['renderer_sha256']
    result = {}
    if platform=='linux':
        prefix = spec['archive'].removesuffix('.tar.gz')+'/'
        with tarfile.open(fileobj=io.BytesIO(raw),mode='r:gz') as t:
            for key in expected:
                member=t.getmember(prefix+name(key))
                check(member.isfile(), 'Support member is not a regular file: '+key)
                result[key]=t.extractfile(member).read()
    else:
        with zipfile.ZipFile(io.BytesIO(raw)) as z:
            for key in expected:
                result[key]=z.read(name(key))
    check(all(sha(result[k])==v for k,v in expected.items()),platform+' support member digest mismatch')
    seed = result.pop(spec['renderer'])
    return result, seed


def check_renderer_distribution(path, renderer):
    for member, data in archive_inputs(path, renderer):
        findings = check_packaging(member, data)
        if findings:
            raise ValueError('Excluded renderer distribution content in ' + member + ': ' + ', '.join(findings))


def package(platform,args,spec,tree):
    receipt_path=getattr(args,platform+'_build')
    build=json.loads(receipt_path.read_bytes())
    check((build.get('source') or build.get('game_source'))==args.commit and (build.get('tree') or build.get('source_tree'))==tree,
          'Build source identity differs from selection: '+platform)
    binary=build.get('binary') or build['ELF' if platform=='linux' else 'pe']
    image=pinned(receipt_path.parent,binary)
    executable(platform,image)
    check(sha(image)!=spec['previous_binary_sha256'],'Refusing the previously published game executable')
    sdk=pinned(receipt_path.parent,build.get('sdk') or build['renderer_sdk'])
    proof_pin=build.get('renderer_inputs') or build['sdk_compiler_inputs']
    proof=json.loads(pinned(receipt_path.parent,proof_pin))
    verify_members(sdk, proof)
    support,seed=read_support(getattr(args,platform+'_support'),spec,platform)
    renderer=companion.build(ROOT,args.commit,tree,platform,sha(image),sha(sdk),proof,seed,spec,args.psycross_upstream)
    check_renderer_distribution(spec['renderer'], renderer)
    support[spec['renderer']]=renderer
    support[spec['game']]=image
    for member, path in spec['tracked'].items():
        data = git_blob(ROOT,args.commit,path)
        companion.check_public_text(member.encode() + b'\n' + data)
        support[member] = data
    if platform=='linux':
        comp=json.loads(support['licenses/components.json'])
        comp['appliesTo']=args.version
        comp['psycrossRendererCorrespondingSource'].update(sha256=sha(renderer),sdk_archive_sha256=sha(sdk))
        support['licenses/components.json']=companion.json_bytes(comp)
    linux=platform=='linux'
    template=git_blob(ROOT,args.commit,PROFILE+'readme.md').decode()
    values=dict(platform='Linux x86-64' if linux else 'Windows x64',version=args.version,commit=args.commit,
                binary=spec['game'],binary_hash=sha(image),commands=COMMANDS[platform],
                save_dir='`$XDG_DATA_HOME/xenogears-port` (usually `~/.local/share/xenogears-port`)' if linux else r'`%LOCALAPPDATA%\Xenogears Port`',
                notices='Third-party terms and sources are in `THIRD_PARTY.md` and `licenses/`.' if linux else 'Third-party terms and sources are in `runtime/THIRD_PARTY.md`, `runtime/licenses/` and `sources/`.')
    support['README.md']=template.format(**values).encode()
    support['SHA256SUMS']=companion.checksums(support)
    title='xenogears-'+args.version+('-linux-x86_64' if linux else '-windows-x64')
    if linux:
        return title+'.tar.gz',companion.tar_bytes(support,title+'/',{'xenogears.sh',spec['game'],'prepare-data/prepare.py'})
    buf=io.BytesIO()
    with zipfile.ZipFile(buf,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for key,data in sorted(support.items()):
            info=zipfile.ZipInfo(name(key),(1980,1,1,0,0,0))
            info.create_system=3
            info.external_attr=(stat.S_IFREG|0o644)<<16
            archive.writestr(info,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=6)
    return title+'.zip',buf.getvalue()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=['stage','verify'])
    parser.add_argument('--commit',required=True)
    parser.add_argument('--version',required=True)
    for platform in ('linux','windows'):
        parser.add_argument('--'+platform+'-build',required=True,type=Path,help='New same-revision build receipt')
        parser.add_argument('--'+platform+'-support',required=True,type=Path,help='Published v0.7.93-preview platform archive')
    parser.add_argument('--psycross-upstream',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    try:
        check(re.fullmatch(r'[a-f0-9]{40}',args.commit),'Choose a full Git source commit')
        check(re.fullmatch(r'v[0-9]+(?:\.[0-9]+){2}(?:-[A-Za-z0-9]+(?:[.-][A-Za-z0-9]+)*)?',args.version),'Invalid package version')
        tree=subprocess.check_output(['git','--no-replace-objects','-C',ROOT,'rev-parse','--verify',args.commit+'^{tree}'],text=True).strip()
        for path in ['pc_port/tools/package_public.py',PROFILE+'companion.py',PROFILE+'restore_renderer.py',
                     ANALYSIS+'public_digest_packaging.py',ANALYSIS+'public_guard_archives.py']:
            check((ROOT/path).read_bytes()==git_blob(ROOT,args.commit,path),'Packaging code differs from source revision: '+path)
        specs=json.loads(git_blob(ROOT,args.commit,PROFILE+'support.json'))
        files=dict(package(p,args,specs[p],tree) for p in ('linux','windows'))
        files['SHA256SUMS']=companion.checksums(files)
        if args.command=='verify':
            check(args.out.is_dir() and not args.out.is_symlink() and {p.name for p in args.out.iterdir()}==set(files),'Expected exactly two packages and SHA256SUMS')
            for path,data in files.items():
                target=args.out/path
                check(not target.is_symlink() and target.is_file() and target.read_bytes()==data,'Archive readback differs: '+path)
        else:
            check(not os.path.lexists(args.out),'Choose a new staging directory')
            with tempfile.TemporaryDirectory(dir=args.out.parent.resolve()) as tmp:
                stage=Path(tmp)/'release';stage.mkdir()
                for path,data in files.items(): (stage/path).write_bytes(data)
                directory_fd = os.open(args.out.parent, os.O_RDONLY | os.O_DIRECTORY)
                try:
                    fcntl.flock(directory_fd, fcntl.LOCK_EX)
                    check(not os.path.lexists(args.out), 'Staging destination already exists')
                    stage.rename(args.out)
                finally:
                    os.close(directory_fd)
        for path,data in sorted(files.items()): print(sha(data),path)
    except (OSError,ValueError,KeyError,tarfile.TarError,zipfile.BadZipFile,subprocess.CalledProcessError) as exc:
        parser.exit(1,'package_public: '+str(exc)+'\n')


if __name__=='__main__':
    main()
