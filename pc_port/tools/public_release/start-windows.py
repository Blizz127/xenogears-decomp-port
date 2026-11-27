# SPDX-License-Identifier: MIT
"""Run the Xenogears Windows port with a prepared US Disc 1 directory."""
import argparse
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data', type=Path, help='directory created by prepare-data/prepare.py')
    args = parser.parse_args()
    data = args.data.resolve()
    files = ('disc1.bin', 'SLUS_006.64', 'field.bin', 'world_map.bin', 'menu.bin',
             'member_change_menu.bin', 'shop_menu.bin')
    missing = [name for name in files if not (data / name).is_file()]
    if missing:
        parser.exit(2, 'Run prepare-data/prepare.py for this data directory first. Missing '
                    + ', '.join(missing) + '\n')
    executable = Path(__file__).resolve().parent / 'runtime' / 'xeno-port.exe'
    if not executable.is_file():
        parser.exit(2, 'Extract the complete Windows ZIP before running this script.\n')
    local = os.environ.get('LOCALAPPDATA')
    if not local:
        parser.exit(2, 'Windows did not provide a Local AppData directory.\n')
    profile = Path(local) / 'Xenogears Port'
    cards, quick = profile / 'memcards', profile / 'quicksaves'
    cards.mkdir(parents=True, exist_ok=True)
    quick.mkdir(exist_ok=True)
    env = dict(os.environ, XENO_CHEATS="0", XENOGEARS_DATA=str(data), XENO_DATA_DIR=str(data),
               XENO_DISC=str(data / 'disc1.bin'), XENO_MEMCARD_DIR=str(cards),
               XENO_QUICKSAVE_PATH=str(quick / 'quick.xgqs'))
    return subprocess.run([str(executable)], cwd=profile, env=env).returncode


if __name__ == '__main__':
    raise SystemExit(main())
