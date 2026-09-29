#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
binary=$(mktemp /tmp/xeno-host-toolbar-test.XXXXXX)
trap 'rm -f "$binary"' EXIT

cc -std=c99 -Wall -Wextra -Werror \
    "$repo_root/pc_port/tests/host_toolbar_logic_test.c" \
    -o "$binary"
"$binary"
