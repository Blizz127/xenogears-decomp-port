#!/usr/bin/env bash
set -euo pipefail
sed 's/0x2A/0x2B/' "$1" > "$2"
if cmp -s "$1" "$2"; then
    echo "mut_diff_800AA760: mutation did not apply" >&2
    exit 1
fi
