#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/slus_006.64/system/temp3.c"
elf="$root/build/out/slus_006.64.elf"
bin="$root/pc_port/build_native/xeno-port"
stubs="$root/pc_port/build_native/stubs.c"

retail="$(mips-linux-gnu-objdump -d --start-address=0x80035d50 \
    --stop-address=0x80035d58 "$elf")"
grep -Fq 'jr' <<<"$retail"
grep -Fq 'nop' <<<"$retail"
test "$(grep -Ec '^80035d5[04]:' <<<"$retail")" -eq 2

grep -Fq 'void func_800379D0(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f) {' "$src"
for arg in a b c d e f; do
    grep -Fq "(void)$arg;" "$src"
done

if grep -Eq '^(long|unsigned char) func_800379D0(\[|\()' "$stubs"; then
    echo 'retail-empty func_800379D0 still uses generated fallback' >&2
    exit 1
fi
nm -g --defined-only "$bin" | awk '$3 == "func_800379D0" {found=1} END {exit !found}'

echo 'retail-empty func_800379D0: PASS'
