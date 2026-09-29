#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

CC_BIN="${CC:-cc}"
COMMON=(
  -std=c17 -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/src/world_map_capture.c"
  "$ROOT/pc_port/tests/w34b72_world_capture_prod_test.c"
)

"$CC_BIN" -O0 "${COMMON[@]}" -o "$TMP/w34b72-o0"
"$TMP/w34b72-o0"

"$CC_BIN" -O2 "${COMMON[@]}" -o "$TMP/w34b72-o2"
"$TMP/w34b72-o2"

"$CC_BIN" -O2 -fsanitize=undefined -fno-sanitize-recover=all \
  "${COMMON[@]}" -o "$TMP/w34b72-ubsan"
"$TMP/w34b72-ubsan"

echo "W34B72 CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"
