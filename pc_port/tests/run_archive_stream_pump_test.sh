#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

python3 - "$root" "$out" <<'PYEXTRACT'
import sys
from pathlib import Path
s=(Path(sys.argv[1])/'src/slus_006.64/system/libarchive.c').read_text()
a=s.index('int ArchiveDataSync(void) {')
b=s.index('\n}',a)+2
body=s[a:b]
a=s.index('void ArchiveCdDataSync(int mode) {')
b=s.index('\n}',a)+2
(Path(sys.argv[2])/'archive_sync_under_test.inc').write_text(body+'\n'+s[a:b])
PYEXTRACT

gcc ${XENO_TEST_CFLAGS:-} -I"$out" -std=gnu17 -fpermissive -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h \
    -ffunction-sections -fdata-sections \
    -I"$root/pc_port/include_shim" -I"$root/include" \
    -I"$root/pc_port/extern/PsyCross/include" \
    -I"$root/pc_port/extern/PsyCross/include/psx" \
    "$root/pc_port/tests/archive_stream_pump_test.c" \
    -Wl,--gc-sections -no-pie -o "$out/test"

timeout 10 "$out/test"
echo 'archive stream pump: PASS'
