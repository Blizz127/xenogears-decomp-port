#!/usr/bin/env bash
# Generic M2 menu retail differential runner (pc_port/tests/lib/m2_retail_diff.h).
#   run_menu_m2_retail.sh <name>
# builds pc_port/tests/menu_m2_<name>_retail_test.c against the XENO_PC_PORT
# arm of src/menu/main/misc.c at O0/O2/UBSan (shift-base ON), runs each, then
# applies every mutant in pc_port/tests/menu_m2_<name>.mutants (one per line:
# name<TAB>function<TAB>old<TAB>new, "\n" escapes) to that function's body
# only; every mutant must FAIL the differential.
# Symbols the test defines (spies, globals) are weakened in the misc.c object.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
NAME=$1
TEST=pc_port/tests/menu_m2_${NAME}_retail_test.c
MUT=pc_port/tests/menu_m2_${NAME}.mutants
OUT=pc_port/build_native/menu_m2_${NAME}
mkdir -p "$OUT/probe-tmp"
export TMPDIR="$PWD/$OUT/probe-tmp"
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
LINK_CC=${LINK_CC:-clang}
python3 - <<'PYGEN'
from pathlib import Path
import hashlib
b = Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest() == \
    '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d', \
    'menu.bin SHA pin'
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -fno-strict-aliasing -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
weaken() { # $1 owner object, $2 test object
  local args=() s
  for s in $(nm --defined-only "$2" | awk '$2 ~ /^[TDBRC]$/ {print $3}'); do args+=(--weaken-symbol="$s"); done
  objcopy "${args[@]}" "$1"
}
for mode in O0 O2 UBSan; do
  flags=(-"$mode"); if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fsanitize=shift-base -fno-sanitize-recover=all); fi
  # -fPIC: intra-TU calls/data go through the symbol, so the test spies win;
  # -fno-builtin: library calls (bzero, ...) stay calls, as in retail.
  gcc "${BASE[@]}" "${flags[@]}" -fPIC -fno-builtin -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
  gcc "${BASE[@]}" "${flags[@]}" -c "$TEST" -o "$OUT/$mode.test.o"
  weaken "$OUT/$mode.owner.o" "$OUT/$mode.test.o"
  gcc "${BASE[@]}" -O0 -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
  "$LINK_CC" -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.test.o" "$OUT/$mode.owner.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
  echo "[$mode]"; "$OUT/$mode"
done
[[ -f "$MUT" ]] || { echo "no mutant file $MUT"; exit 1; }
python3 - "$MUT" "$OUT" <<'PYGEN'
import re, sys
from pathlib import Path
spec, out = sys.argv[1], Path(sys.argv[2])
src = Path('src/menu/main/misc.c').read_text()
names = []
for line in Path(spec).read_text().splitlines():
    if not line.strip() or line.startswith('#'):
        continue
    name, fn, old, new = line.split('\t')
    old, new = old.replace('\\n', '\n'), new.replace('\\n', '\n')
    # The XENO_PC_PORT definition of fn: last definition line in the file.
    defs = [m.start() for m in re.finditer(r'\n[A-Za-z][^\n;]*\b' + fn + r'\([^;{]*\{', src)]
    assert defs, (name, fn)
    a = defs[-1]
    b = src.index('\n}\n', a)
    body = src[a:b]
    assert body.count(old) == 1, (name, fn, old, body.count(old))
    (out / (name + '.c')).write_text(src[:a] + body.replace(old, new, 1) + src[b:])
    names.append(name)
(out / 'mutants.list').write_text('\n'.join(names) + '\n')
PYGEN
n=0
while read -r mutant; do
  gcc "${BASE[@]}" -O2 -fPIC -fno-builtin -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
  weaken "$OUT/$mutant.o" "$OUT/O2.test.o"
  "$LINK_CC" -no-pie -O2 -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/$mutant.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
  if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then echo "FAIL survived $mutant"; exit 1; fi
  grep -q ' FAIL ' "$OUT/$mutant.log" || { echo "FAIL $mutant crashed without a differential verdict"; tail -3 "$OUT/$mutant.log"; exit 1; }
  echo "control rejected: $mutant ($(grep -m1 ' FAIL ' "$OUT/$mutant.log"))"
  n=$((n+1))
done < "$OUT/mutants.list"
echo "M2 $NAME negative controls PASS count=$n"
