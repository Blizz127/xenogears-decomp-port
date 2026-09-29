#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
binary="${TMPDIR:-/tmp}/quick_checkpoint_file_test.$$"
trap 'rm -f "$binary"' EXIT

cc -std=gnu17 -Wall -Wextra -Werror \
    -I"$repo_root/pc_port/src" \
    "$repo_root/pc_port/tests/quick_checkpoint_file_test.c" \
    "$repo_root/pc_port/src/quick_checkpoint_file.c" \
    -o "$binary"
"$binary"
