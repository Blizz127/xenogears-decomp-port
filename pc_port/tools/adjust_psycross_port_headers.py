"""Install the project's GPU, SPU and CD headers in the selected PsyCross checkout."""
import argparse
import hashlib
import os
from pathlib import Path
import stat
import subprocess
from tempfile import NamedTemporaryFile


PROJECT = Path(__file__).resolve().parents[2]
PORT_HEADERS = PROJECT / 'pc_port' / 'renderer_headers' / 'psx'

# Pinned PsyCross e56e4cde headers and the GPU/SPU forms produced or accepted
# by previous port builds, including the complete pre-minimization headers.
KNOWN_INPUTS = {
    'libgpu.h': {
        '40143e465a87c6b5695a18a8613f5b32517b5763199e69a79e9aac47131af227',
        'f6039ad95766eabbd1737e2b5fdbfcd735db67cc16a702091deab21872ce06f1',
        '5d16e4691bb7892dce6bbb7c0d2051914549858228430d72960977c006620b73',
        'e52adaf1bf18ecf3d24f6b8005c01114926771c7f1878cc7f345c84a10542af1',
        '97dc22c5d2830eed91cf0ca74f608d67d4cc80c7d620b2dffbb963772214626b',
    },
    'libspu.h': {
        '0d1ecf587ff0c1e2433628daf21ea47cb9b9f21d64fab6cb89a0898f067f60ee',
        'e47002308bf810a30f7fad050e3e811afbf62e21232bf007ffbcd1bc82b9731e',
        'fff7b21c6a96cee58c8c49986774bfa5e2970411323da2be30bfb8563700ada3',
        'b60f16b299bd98636b91c6735f92efed856c4ba7eb501a85585da8ecaa1b8bbe',
        '575071730a75c104559243ead0f8c047f9feaf241e5edcf17d015d4fa5f66e7d',
    },
    'libcd.h': {
        'e5aeb1891ec58896f87bab7c816428165988a3ba8e2f6c52ca7c732ca8a9dff7',
        '4fbc51c5504713d4c57e3370bc099d1e429b32e5737ecb6b9bc00c733c3e2adb',
    },
}


# GitHub source archives lack Git metadata. Their canonical header bytes must
# match the released inputs; source checkouts follow their own selected commit.
EXPORT_HEADERS = {
    'libgpu.h': 'b8e16a88083526764fbf8d25eff2b900620f33a8ab4ca1bf167a88e58b13348d',
    'libspu.h': '48546469fa1371fcb01bef935fe1da759347f74131c0c07cd0592b014001d0b6',
    'libcd.h': 'e5aeb1891ec58896f87bab7c816428165988a3ba8e2f6c52ca7c732ca8a9dff7',
}


def project_revision():
    if not (PROJECT / '.git').exists():
        return None
    return subprocess.check_output(
        ['git', '--no-replace-objects', '-C', PROJECT, 'rev-parse', '--verify', 'HEAD^{commit}'],
        text=True,
    ).strip()


def project_bytes(source, revision):
    data = source.read_bytes()
    if revision is not None:
        relative = source.relative_to(PROJECT).as_posix()
        committed = subprocess.check_output(
            ['git', '--no-replace-objects', '-C', PROJECT, 'cat-file', 'blob', revision + ':' + relative]
        )
        if data != committed:
            raise ValueError(f'Project renderer header differs from the selected Git commit: {source}')
    elif hashlib.sha256(data).hexdigest() != EXPORT_HEADERS[source.name]:
        raise ValueError(f'Unexpected source archive renderer header: {source}')
    return data


def install(sdk):
    destination = sdk.resolve(strict=True) / 'include' / 'psx'
    revision = project_revision()
    pending = []
    for name, accepted in KNOWN_INPUTS.items():
        source = PORT_HEADERS / name
        target = destination / name
        for path in (source, target):
            if path.resolve(strict=True) != path or not path.is_file():
                raise ValueError(f'Expected a regular renderer header: {path}')
        owned = project_bytes(source, revision)
        old = target.read_bytes()
        if old == owned:
            continue
        if hashlib.sha256(old).hexdigest() not in accepted:
            raise ValueError(f'Unrecognized local renderer header; preserve it before replacing {target}')
        pending.append((target, owned))

    for target, data in pending:
        with NamedTemporaryFile(dir=target.parent, delete=False) as tmp:
            temp = Path(tmp.name)
            try:
                tmp.write(data)
                tmp.flush()
                os.fchmod(tmp.fileno(), stat.S_IMODE(target.stat().st_mode))
                temp.replace(target)
            finally:
                temp.unlink(missing_ok=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('psycross', type=Path)
    args = parser.parse_args()
    try:
        install(args.psycross)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f'renderer headers: {exc}\n')
