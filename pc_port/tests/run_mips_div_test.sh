#!/usr/bin/env bash
# Checks include/common.h's XenoMipsDiv against R3000 DIV semantics.
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
sed -n '/^static inline int XenoMipsDiv/,/^}/p' "$ROOT/include/common.h" > "$T/mips_div_helper.inc"
cc -std=c11 -O2 -I"$T" "$ROOT/pc_port/tests/mips_div_test.c" -o "$T/t"
"$T/t"
