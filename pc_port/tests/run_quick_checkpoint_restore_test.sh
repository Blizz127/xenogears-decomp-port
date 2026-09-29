#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
python3 - "${XENO_CHECKPOINT_TEST_SOURCE:-pc_port/src/quick_checkpoint.c}" "$out" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text();a=s.index('void PcPort_QuickCheckpointRestorePlayer(void)');b=s.index('\n}',a)+2
(Path(sys.argv[2])/'checkpoint_restore.inc').write_text(s[a:b])
PY
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
 "${CC:-cc}" -std=gnu17 -Wall -Wextra "${flags[@]}" -I"$out" pc_port/tests/quick_checkpoint_restore_test.c -o "$out/test"
 "$out/test"
done
