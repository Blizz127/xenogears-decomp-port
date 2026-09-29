#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/menu/main/misc.c"
body="$(sed -n '/^void func_801D1B20(void)/,/^}/p' "$src")"

expected=(
    func_801D3B00 func_801D11F0 func_801CE3C8 func_801CE338
    func_801D02D8 func_801D01D0 func_801CE540 func_801CE660
    func_801CEB5C func_801CEBB4 func_801CE464 func_801D13F8
    func_801D1AAC func_801D14FC func_801D1640 func_801D17C4
    func_801D1914 func_801D1464 func_801D14B0 func_801D0C78
    func_801CEC40 func_801CF308
)

last=0
for fn in "${expected[@]}"; do
    line="$(grep -n -m1 "    $fn();" <<<"$body" | cut -d: -f1)"
    test -n "$line"
    if (( line <= last )); then
        echo "retail root-submit order mismatch at $fn" >&2
        exit 1
    fi
    last="$line"
done

if grep -Eq 'XENO_PC_PORT|root submit' <<<"$body"; then
    echo 'host-only root-submit shortcut remains' >&2
    exit 1
fi

ce660="$(sed -n '/^void func_801CE660(void)/,/^}/p' "$src")"
# The definition may carry the retail body behind an inner #else; these
# checks are about the port body, so keep only its #ifdef XENO_PC_PORT arm.
if grep -q '^#ifdef XENO_PC_PORT' <<<"$ce660"; then
    ce660="$(sed -n '/^#ifdef XENO_PC_PORT/,/^#else/p' <<<"$ce660")"
fi
test "$(grep -c 'func_801CE198(' <<<"$ce660")" -eq 12
grep -Fq 'data[0x2AED]' <<<"$ce660"
grep -Fq 'data[0x2AEE]' <<<"$ce660"
grep -Fq '(POLY_FT4*)(data + 0x1C70)' <<<"$ce660"

echo 'menu root submit retail order: PASS'
