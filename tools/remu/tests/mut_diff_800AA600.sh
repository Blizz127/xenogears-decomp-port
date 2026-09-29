#!/usr/bin/env bash
set -euo pipefail
sed 's/p + 0x24/p + 0x26/' "$1" > "$2"
if cmp -s "$1" "$2"; then
    echo "mut_diff_800AA600: mutation did not apply" >&2
    exit 1
fi
