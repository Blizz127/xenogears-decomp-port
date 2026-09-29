#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
python3 - <<'PY'
import pathlib,re,hashlib
retail=pathlib.Path('disc/field.bin').read_bytes()
for name in ['func_80092424','func_800924D4']:
    rows=re.findall(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',pathlib.Path('asm/field/matchings/main/misc11/'+name+'.s').read_text())
    assert rows
    for a,b in rows:
        off=int(a,16)-0x8006faf0
        assert retail[off:off+4]==bytes.fromhex(b)
    print(name,len(rows)*4,hashlib.sha256(b''.join(bytes.fromhex(b) for a,b in rows)).hexdigest())
PY
for opt in '-O0' '-O2' '-O2 -fsanitize=undefined -fno-sanitize-recover=all'; do
    gcc -include assert.h -std=gnu17 -fno-pie -no-pie -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -w -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src $opt pc_port/tests/field_collision_flags_test.c "${COLLISION_FLAGS_SOURCE:-src/field/main/misc11.c}" pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$build/test"
    "$build/test"
done
