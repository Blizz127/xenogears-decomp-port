#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
python3 - <<'PY'
import re, pathlib, hashlib
text=pathlib.Path('asm/battle/15898.s').read_text()
rows=[(int(a,16),bytes.fromhex(b)) for a,b in re.findall(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',text) if 0x80085618<=int(a,16)<0x80085ac4]
retail=pathlib.Path('disc/battle.bin').read_bytes()
assert rows and all(b==retail[a-0x8006faf0:a-0x8006faf0+4] for a,b in rows)
assert [a for a,b in rows]==list(range(0x80085618,0x80085ac4,4))
print('Retail damage bytes verified:',len(rows)*4,hashlib.sha256(b''.join(b for a,b in rows)).hexdigest())
PY
for opt in '-O0' '-O2' '-O2 -fsanitize=undefined -fno-sanitize-recover=all'; do
    gcc -std=c11 -Wall -Wextra -Werror $opt -Ipc_port/src pc_port/tests/god_mode_retail_test.c pc_port/src/god_mode.c pc_port/src/battle_mips_adapter.c -o "$build/test"
    "$build/test"
done
bash pc_port/tests/run_host_toolbar_logic_test.sh
