#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
python3 - "${XENO_MENU_TEST_SOURCE:-src/menu/main/misc.c}" "$out" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
a=s.index('void func_801C55A0(void) {')
b=s.index('\n}',a)+2
(Path(sys.argv[2])/'file_menu_loop.inc').write_text(s[a:b])
PY
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=gnu17 -DXENO_PC_PORT -Wall -Wextra -Werror "${flags[@]}" \
        -I"$out" pc_port/tests/file_menu_guard_test.c -o "$out/test"
    "$out/test"
done
