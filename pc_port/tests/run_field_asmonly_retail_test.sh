#!/usr/bin/env bash
# Full production misc2 TU vs an independent oracle for the two camera-sector
# scan helpers that were ASM_ONLY before 2026-09-06, plus retail byte pins read
# from the shipped field overlay.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
SOURCE=${FIELD_ASMONLY_SOURCE:-src/field/main/misc2.c}
OUT=${FIELD_ASMONLY_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-field-asmonly.XXXXXXXX")}
mkdir -p "$OUT"
echo "FIELD ASMONLY OUTPUT $OUT"

python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json, sys
source = Path(sys.argv[1]); out = Path(sys.argv[2])
disc = Path('disc/field.bin'); raw = disc.read_bytes()
assert sha256(raw).hexdigest() == '38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
# The two pinned functions, and the sector table they index.
for name, off, length, checksum in [('func_8007234C', 0x285C, 76, 0x0E04),
                                    ('func_80072398', 0x28A8, 76, 0x1014)]:
    assert sum(raw[off:off + length]) & 0xFFFFFFFF == checksum, name
assert raw[0x3E12C:0x3E134] == bytes.fromhex('10204080 01020408'.replace(' ', ''))
paths = [source, disc,
         Path('pc_port/tests/field_asmonly_retail_test.c'),
         Path('pc_port/tests/run_field_asmonly_retail_test.sh'),
         Path('include/common.h')]
(out / 'pins.json').write_text(json.dumps({
    'source_pins': {str(p.resolve()): sha256(p.read_bytes()).hexdigest() for p in paths},
    'retail_functions': {
        'func_8007234C': {'entry': '8007234C', 'raw_offset': '285C', 'bytes': 76},
        'func_80072398': {'entry': '80072398', 'raw_offset': '28A8', 'bytes': 76},
    },
    'scope': 'full production misc2 TU; pure integer helpers, no SDK or '
             'rendering surface; oracle indexes the sector table by iteration '
             'number instead of mutating a cursor',
    'clang_compatibility': 'test-only forward declarations of unrelated later '
                           'full-TU definitions avoid conflicting implicit '
                           'declarations; no tested body is replaced',
}, indent=2) + '\n')
(out / 'native-prototypes.h').write_text('''/* Test-only forward declarations of existing later full-TU definitions. */
#include "common.h"
#include "psyq/libgte.h"
void FieldMatrixCopy(MATRIX *, MATRIX *);
void FieldMatrixCopyTranslation(MATRIX *, MATRIX *);
void FieldMatrixCopyTransform(MATRIX *, MATRIX *);
void func_80281B00(void *);
''')
PY

COMMON=(-include assert.h -include "$OUT/native-prototypes.h" -std=gnu17 -fno-pie
        -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM
        -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections
        -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

for mode in O0 O2 UBSan; do
  flags=(-"$mode")
  if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
  clang "${COMMON[@]}" "${flags[@]}" -Wno-everything -c "$SOURCE" -o "$OUT/$mode.source.o"
  clang "${COMMON[@]}" "${flags[@]}" -c pc_port/tests/field_asmonly_retail_test.c -o "$OUT/$mode.test.o"
  clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" -o "$OUT/$mode"
  "$OUT/$mode" disc/field.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log"; exit 1; }
  cat "$OUT/$mode.log"
done

# Deliberately-defective variants of the production bodies. Each must be caught.
python3 - "$SOURCE" "$OUT" <<'PY'
import sys, json
from pathlib import Path
s = Path(sys.argv[1]).read_text(); out = Path(sys.argv[2])
a = s.index('/* Count how many consecutive camera sectors')
b = s.index('void func_800723E4', a)
body = s[a:b]
controls = {}
for name, old, new in [
        ('forward-steps-backward', '        index++;\n', '        index--;\n'),
        ('backward-steps-forward', '        index--;\n', '        index++;\n'),
        ('short-lap', 'while (i < 8) {', 'while (i < 7) {'),
        ('inverted-test', 'if (!(mask & bit)) {', 'if ((mask & bit)) {'),
        ('narrow-wrap', 'D_800ADC1C[index & 7]', 'D_800ADC1C[index & 3]'),
        ('full-lap-returns-count', '    return 0;\n}', '    return found;\n}')]:
    assert body.count(old) >= 1, (name, body.count(old))
    path = out / (name + '.c')
    path.write_text(s[:a] + body.replace(old, new, 1) + s[b:])
    controls[name] = str(path)
(out / 'controls.json').write_text(json.dumps(controls, indent=2) + '\n')
PY

for control in forward-steps-backward backward-steps-forward short-lap \
               inverted-test narrow-wrap full-lap-returns-count; do
  clang "${COMMON[@]}" -O2 -Wno-everything -c "$OUT/$control.c" -o "$OUT/$control.source.o"
  clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" -o "$OUT/$control"
  rc=0
  "$OUT/$control" disc/field.bin > "$OUT/$control.log" 2>&1 || rc=$?
  if [ "$rc" != 1 ] || ! grep -q '^FIELD ASMONLY FAIL ' "$OUT/$control.log"; then
    cat "$OUT/$control.log"
    echo "FIELD ASMONLY control failed to detect defect: $control rc=$rc" >&2
    exit 1
  fi
  echo "FIELD ASMONLY CONTROL REJECTED $control"
done

python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json, sys
pins = json.loads((Path(sys.argv[1]) / 'pins.json').read_text())
for path, wanted in pins['source_pins'].items():
    assert sha256(Path(path).read_bytes()).hexdigest() == wanted, path
print('FIELD ASMONLY PINS VERIFIED')
PY
