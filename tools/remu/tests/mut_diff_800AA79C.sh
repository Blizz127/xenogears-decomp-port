#!/usr/bin/env bash
set -euo pipefail
sed 's/+ 0x34/+ 0x35/g' "$1" > "$2"
if cmp -s "$1" "$2"; then
    echo "mut_diff_800AA79C: mutation did not apply" >&2
    exit 1
fi
