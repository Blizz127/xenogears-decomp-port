#!/usr/bin/env python3
"""Restore the renderer compiler inputs named in the companion's manifest."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tarfile
import tempfile


PROJECT_HEADERS = {
    'pc_port/extern/PsyCross/include/psx/' + filename: 'pc_port/renderer_headers/psx/' + filename
    for filename in ('libgpu.h', 'libspu.h', 'libcd.h')
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def check(condition, message):
    if not condition:
        raise ValueError(message)


def name(value):
    path = PurePosixPath(value)
    check(bool(value) and path.as_posix() == value and not path.is_absolute()
          and '..' not in path.parts and '\\' not in value and ':' not in value,
          f'Invalid member path {value!r}')
    return value


def contents(data):
    result = {}
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as archive:
        for item in archive:
            key = name(item.name)
            check(item.isfile() and key not in result, 'Unexpected renderer member ' + key)
            result[key] = archive.extractfile(item).read()
    return result


def git_blob(repo, revision, path):
    return subprocess.check_output(['git','--no-replace-objects','-C',str(repo),'cat-file','blob',revision+':'+name(path)])


def snapshot(root):
    return {p.relative_to(root).as_posix(): p.read_bytes() for p in root.rglob('*') if p.is_file()}


def historical_sources(data, upstream, digest):
    check(sha(data) == digest, 'Renderer support archive digest mismatch')
    files = contents(data)
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)/'recipe'
        root.mkdir()
        for k, v in files.items():
            dest = root/k
            dest.parent.mkdir(parents=True,exist_ok=True)
            dest.write_bytes(v)
        output = Path(tmp)/'restored'
        subprocess.run([sys.executable,'-I','-B',str(root/'restore.py'),str(upstream.resolve()),str(output)],
                       check=True,stdout=subprocess.PIPE)
        return snapshot(output)


def reconstruct(files, upstream):
    manifest = json.loads(files['manifest.json'])
    schema = manifest['schema']
    check(schema in (1, 2), 'Unknown source companion schema')
    sources = historical_sources(files['recipe.tar.gz'], upstream, manifest['recipe_sha256'])
    if schema == 2:
        check(not (PROJECT_HEADERS.keys() & sources.keys()), 'Source recipe contains superseded renderer inputs')
        for key, path in PROJECT_HEADERS.items():
            row = manifest['compiler_inputs'][key]
            check(row.get('origin') == 'source' and row.get('path') == path,
                  'Renderer override needs its committed project origin: ' + key)
    output = {}
    for key, row in manifest['compiler_inputs'].items():
        name(key)
        if row['origin'] == 'published':
            data = sources[key]
        elif row['origin'] == 'source':
            data = files['inputs/'+key]
        elif row['origin'] == 'upstream':
            data = git_blob(upstream, manifest['upstream_commit'], row['path'])
        else:
            raise ValueError('Unknown source origin for '+key)
        check(sha(data) == row['sha256'], 'Compiler input changed: '+key)
        output[key] = data
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream', type=Path, help='Local OpenDriver2/PsyCross Git checkout with the pinned commit')
    parser.add_argument('destination', type=Path, help='New directory for verified sources')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    files = snapshot(root)
    output = reconstruct(files, args.upstream)
    args.destination.mkdir()
    for key, data in output.items():
        dest = args.destination/key
        dest.parent.mkdir(exist_ok=True,parents=True)
        dest.write_bytes(data)
    print('Restored', len(output), 'verified compiler inputs')


if __name__ == '__main__':
    main()
