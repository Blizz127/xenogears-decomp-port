"""Generate a renderer source companion from the actual build dependency report."""
import gzip
import io
import json
from pathlib import Path
import re
import tarfile
import zlib

from .restore_renderer import PROJECT_HEADERS, check, contents, git_blob, historical_sources, name, project_includes, sha

VENDOR = 'pc_port/extern/PsyCross/'
UPSTREAM = 'e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6'


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True)+'\n').encode()


def tar_bytes(files, prefix='', executable=()):
    stream = io.BytesIO()
    with gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0, compresslevel=6) as compressed:
        with tarfile.open(fileobj=compressed,mode='w',format=tarfile.GNU_FORMAT) as tar:
            for key, data in sorted(files.items()):
                info = tarfile.TarInfo(prefix+name(key))
                info.size = len(data)
                info.mode = 0o755 if key in executable else 0o644
                tar.addfile(info, io.BytesIO(data))
    return stream.getvalue()


def checksums(files):
    return ''.join(f'{sha(data)}  {key}\n' for key,data in sorted(files.items())).encode()


def check_public_text(data):
    """Apply the published profile's two historical text exclusions by digest."""
    fingerprints = (
        (19, 132701054, 'b164c253c80a5db9304a93f0c8da89bbfcd81b9edc53329d8ce5d00b7c1c8157'),
        (7, 2129856702, 'c5c767165ca51f2522eed0ee579cffe84b6b16971fe6a1d545b66f57532c70ad'),
    )
    for token in re.findall(rb'[a-z0-9_.-]{7,}', data.lower()):
        for width, checksum, digest in fingerprints:
            for index in range(len(token) - width + 1):
                window = token[index:index+width]
                if zlib.crc32(window) == checksum and sha(window) == digest:
                    raise ValueError('Source text contains an excluded integration identifier')



def retained_recipe(seed, old, platform):
    """Keep the authenticated donor's unchanged deltas and notices in their original form."""
    files = contents(seed)
    check(files.pop('SHA256SUMS') == checksums(files), 'Donor recipe checksums differ')
    check(files['restored-inputs.sha256'] == checksums(old), 'Donor source manifest differs')
    manifest = json.loads(files['manifest.json'])
    changes = json.loads(files['changes.json'])
    check(manifest['platform'] == platform and platform in ('linux', 'windows'), 'Wrong donor platform')
    originals = []
    for key, path in PROJECT_HEADERS.items():
        relative = key[len(VENDOR):]
        entry = relative if platform == 'linux' else 'PsyCross/' + relative
        data = files.pop(path)
        originals.append(data)
        check(data and data == old[key] and manifest['full_header_replacements'].pop(key) == path,
              'Wrong donor header source: ' + key)
        details = manifest['sdk_inputs'].pop(entry)
        edit = changes.pop(entry)
        path_key, hash_key = ('additions', 'additions_sha256') if platform == 'linux' else ('added_file', 'added_sha256')
        check(details['upstream_status'] == 'project-replacement' and details['upstream_sha256'] is None
              and details['effective_sha256'] == sha(data)
              and edit[path_key] == path and edit[hash_key] == sha(data)
              and edit['spans'] == [['addition', 0, len(data)]], 'Incomplete donor replacement record: ' + key)
    includes = project_includes(platform)
    for path in includes:
        data = files.pop(path)
        check(data and data == old[path] and manifest['project_includes'].pop(path) == sha(data),
              'Incorrect donor project include: ' + path)
        originals.append(data)
    files['manifest.json'] = json_bytes(manifest)
    files['changes.json'] = json_bytes(changes)
    files['restored-inputs.sha256'] = checksums({k:v for k,v in old.items() if k not in PROJECT_HEADERS and k not in includes})
    files['README.md'] = (
        '# Retained renderer sources\n\n'
        'These compiler inputs are unchanged from the donor identified in the outer manifest. '
        'Use the enclosing restore.py to recover the complete current build. '
        'Upstream and project license notices are included under PsyCross, notices and pc_port.\n'
    ).encode()
    check(not any(header in data for header in originals for data in files.values()),
          'Superseded donor header has a second location in the source recipe')
    files['SHA256SUMS'] = checksums(files)
    return tar_bytes(files, executable=('restore.py',))


def build(repo, commit, tree, platform, image_hash, sdk_hash, proof, seed, spec, upstream):
    check(all(proof.get(k) == expected for k,expected in [('source_commit',commit),('source_tree',tree),('platform',platform),
                ('binary_sha256',image_hash),('sdk_sha256',sdk_hash)]), 'Renderer proof belongs to another build')
    measured = proof['compiler_inputs']
    check(isinstance(measured,dict) and any(k.startswith(VENDOR) for k in measured), 'No measured renderer inputs')
    seed_sha = spec['renderer_sha256']
    old = historical_sources(seed, upstream, seed_sha)
    recipe = retained_recipe(seed, old, platform)
    files = {'recipe.tar.gz':recipe, 'restore.py':git_blob(repo,commit,'pc_port/tools/public_release/restore_renderer.py')}
    files['README.md'] = (f'# Renderer compiler sources for {platform}\n\nGame revision `{commit}`, executable SHA-256 `{image_hash}`, '
                          f'linked renderer archive SHA-256 `{sdk_hash}`.\n\nCheck out commit `{UPSTREAM}` of '
                          'https://github.com/OpenDriver2/PsyCross and run `python3 restore.py /path/to/PsyCross /new/output`. '
                          'The retained source recipe includes the upstream license and notices. '
                          'The manifest identifies the origin and SHA-256 of every measured compiler input.\n').encode()
    manifest = dict(schema=3,platform=platform,source_commit=commit,source_tree=tree,binary_sha256=image_hash,
                    sdk_sha256=sdk_hash,upstream_commit=UPSTREAM,donor_recipe_sha256=seed_sha,recipe_sha256=sha(recipe),compiler_inputs={})
    mapped = proof.get('source_files',{})
    check(isinstance(mapped,dict) and set(mapped)<=set(measured), 'Unmeasured source mapping')
    check(all(key in measured and mapped.get(key) == path for key, path in PROJECT_HEADERS.items()),
          'Renderer proof omits the committed source of a replaced header')
    for path in project_includes(platform):
        check(path in measured and mapped.get(path) == path,
              'Missing committed renderer project include: ' + path)
    for key, expected in sorted(measured.items()):
        name(key)
        check(bool(re.fullmatch('[0-9a-f]{64}',expected)), 'Invalid compiler input digest: '+key)
        if key not in PROJECT_HEADERS and key.startswith(VENDOR) and key in old and sha(old[key]) == expected:
            row = {'origin':'published'}
        elif key.startswith(VENDOR) and key not in mapped:
            relative = key[len(VENDOR):]
            data = git_blob(upstream,UPSTREAM,relative)
            check(sha(data) == expected, 'Changed vendor input needs its committed source mapping: '+key)
            row = {'origin':'upstream','path':relative}
        else:
            path = name(mapped.get(key,key))
            check(path.startswith(('pc_port/','include/','src/')) and Path(path).suffix in ('.h','.hpp','.c','.C','.cc','.cpp','.inc','.inl','.S'),
                  'Expected a project compiler source: '+path)
            data = git_blob(repo,commit,path)
            check(sha(data) == expected, 'Committed source differs from compiler dependency: '+key)
            row = {'origin':'source','path':path}
            files['inputs/'+key] = data
        manifest['compiler_inputs'][key] = dict(row,sha256=expected)
    files['manifest.json'] = json_bytes(manifest)
    for key, data in files.items():
        if key != 'recipe.tar.gz':
            check_public_text(key.encode() + b'\n' + data)
    files['SHA256SUMS'] = checksums(files)
    return tar_bytes(files)
