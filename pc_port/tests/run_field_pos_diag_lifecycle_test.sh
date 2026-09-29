#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/field-pos-diag-lifecycle.XXXXXX)
CC=${CC:-clang}
BASE=(-std=gnu17 -include assert.h -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0 -Ipc_port/include_shim -Iinclude
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "$CC" "${BASE[@]}" "${flags[@]}" pc_port/src/field_pos_diag.c \
        pc_port/tests/field_pos_diag_lifecycle_test.c -o "$OUT/$mode"
    "$OUT/$mode" > "$OUT/$mode.log" 2>&1
    test ! -s "$OUT/$mode.log"
done
echo 'FIELD POS DIAG LIFECYCLE O0/O2/UBSan PASS'
python3 - "$OUT/mutant.c" <<'PY'
from pathlib import Path
import sys
source = Path('pc_port/src/field_pos_diag.c').read_text()
guard = '    if (D_800ADB04 == 0)\n        return;'
assert source.count(guard) == 1
Path(sys.argv[1]).write_text(source.replace(guard, ''))
PY
"$CC" "${BASE[@]}" -O0 "$OUT/mutant.c" \
    pc_port/tests/field_pos_diag_lifecycle_test.c -o "$OUT/mutant"
rc=0
"$OUT/mutant" > "$OUT/mutant.log" 2>&1 || rc=$?
test "$rc" = 99
echo 'FIELD POS DIAG TRANSITION GUARD MUTANT DETECTED'
echo "Artifacts: $OUT"
