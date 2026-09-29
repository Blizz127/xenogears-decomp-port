#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/field/scripts/virtual_machine.c"
elf="$root/build/out/field.elf"
stubs="$root/pc_port/build_native/stubs.c"

retail="$(mips-linux-gnu-objdump -d --start-address=0x800a8efc \
    --stop-address=0x800a8fec "$elf")"
grep -Fq '<func_800AD978>:' <<<"$retail"
grep -Fq '<func_800AD4D4>' <<<"$retail"
grep -Fq '<func_800ACFD0>' <<<"$retail"

grep -Fq 'void func_800AD978(s32 mode) {' "$src"
grep -Fq 'if (!D_800B2268)' "$src"
grep -Fq 'D_800AFD1C = D_8006F990[i];' "$src"
grep -Fq 'present != (mode != 0)' "$src"

if grep -Eq '^(long|unsigned char) func_800AD978(\[|\()' "$stubs"; then
    echo 'func_800AD978 still uses generated fallback' >&2
    exit 1
fi

echo 'field party refresh retail: PASS'
