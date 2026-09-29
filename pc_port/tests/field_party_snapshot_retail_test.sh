#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/field/scripts/virtual_machine.c"
elf="$root/build/out/field.elf"
bin="$root/pc_port/build_native/xeno-port"
stubs="$root/pc_port/build_native/stubs.c"

retail="$(mips-linux-gnu-objdump -d --start-address=0x800a83a8 \
    --stop-address=0x800a8414 "$elf")"
grep -Fq '<func_800ACE24>:' <<<"$retail"
grep -Fq '8881(v0)' <<<"$retail"
grep -Fq '<func_800AD978>' <<<"$retail"

grep -Fq 'void func_800ACE24(void) {' "$src"
grep -Fq 'for (i = 0; i < 3; i++)' "$src"
grep -Fq 'D_8006BE2C[i] =' "$src"
grep -Fq '((u8*)g_pGameState)[0x22B1 + i]' "$src"
grep -Fq 'func_800AD978(0);' "$src"

if grep -Eq '^(long|unsigned char) func_800ACE24(\[|\()' "$stubs"; then
    echo 'func_800ACE24 still uses generated fallback' >&2
    exit 1
fi
nm -g --defined-only "$bin" | awk '$3 == "func_800ACE24" {found=1} END {exit !found}'

echo 'field party snapshot refresh: PASS'
