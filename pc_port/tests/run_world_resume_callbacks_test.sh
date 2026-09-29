#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
python3 - <<'PY'
import re,pathlib,hashlib
s=pathlib.Path('asm/world_map/120C.s').read_text();d=pathlib.Path('disc/world_map.bin').read_bytes()
for start,end in [(0x80072784,0x8007290c),(0x800976fc,0x80097718)]:
 rows=[(int(a,16),bytes.fromhex(b)) for a,b in re.findall(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s) if start<=int(a,16)<end]
 assert [a for a,b in rows]==list(range(start,end,4))
 assert all(d[a-0x8006faf0:a-0x8006faf0+4]==b for a,b in rows)
 print(hex(start),len(rows)*4,hashlib.sha256(b''.join(b for a,b in rows)).hexdigest())
PY
for opt in '-O0' '-O2' '-O2 -fsanitize=undefined -fno-sanitize-recover=all'; do
 gcc -std=gnu17 -DXENO_PC_PORT -Iinclude -Ipc_port/src -ffunction-sections -fdata-sections $opt pc_port/tests/world_resume_callbacks_test.c pc_port/src/world_map_convergence.c pc_port/src/psx_memory.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$build/test"
 "$build/test"
done
