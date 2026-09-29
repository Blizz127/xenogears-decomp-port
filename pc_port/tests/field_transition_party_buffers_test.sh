#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/field/main/misc5.c"
elf="$root/build/out/field.elf"
bin="$root/pc_port/build_native/xeno-port"
stubs="$root/pc_port/build_native/stubs.c"

retail="$(mips-linux-gnu-objdump -d --start-address=0x800a1cbc \
    --stop-address=0x800a1f08 "$elf")"
for marker in '<func_800A74F8>:' 'li' '<HeapAlloc>' '<HeapPinBlock>' \
              '<ArchiveSetIndex>' '<ArchiveDecodeAlignedSize>' \
              '<func_80029AFC>' '<ArchiveCdDataSync>' '<LZSSDecompress>' \
              '<HeapFree>' '<StoreImage>' '<DrawSync>'; do
    grep -Fq "$marker" <<<"$retail"
done

grep -Fq 'void func_800A74F8(void) {' "$src"
grep -Fq 'g_PartyDataBuffers[1] = HeapAlloc(0x14000, 0);' "$src"
grep -Fq 'g_PartyDataBuffers[2] = HeapAlloc(0x14000, 0);' "$src"
grep -Fq 'if (D_800ADB74 == 2)' "$src"
grep -Fq 'requests[count].archiveIndex = g_GamePartyMemberSkins[slot] + 5;' "$src"
grep -Fq 'func_80029AFC(requests, 0, 0);' "$src"
grep -Fq 'LZSSDecompress(g_PartyStreamDataPointers[slot],' "$src"
grep -Fq 'StoreImage(&rect, g_PartyDataBuffers[1]);' "$src"
grep -Fq 'StoreImage(&rect, g_PartyDataBuffers[2]);' "$src"

if grep -Eq '^(long|unsigned char) func_800A74F8(\[|\()' "$stubs"; then
    echo 'func_800A74F8 still uses generated fallback' >&2
    exit 1
fi
nm -g --defined-only "$bin" | awk '$3 == "func_800A74F8" {found=1} END {exit !found}'

echo 'field transition party buffers: PASS'
