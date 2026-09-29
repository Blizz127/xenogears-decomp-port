#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

SOURCE=${SHOP_BYTE_LOOKUP_SOURCE:-src/shop_menu/main/misc7.c}
case "$SOURCE" in /*) ;; *) SOURCE="$PWD/$SOURCE" ;; esac
test -f "$SOURCE"
OUT=${SHOP_BYTE_LOOKUP_OUT:-$(mktemp -d /tmp/xeno-shop-byte-lookup.XXXXXXXX)}
mkdir -p "$OUT"
echo "SHOP BYTE LOOKUP OUTPUT $OUT"
echo "SHOP BYTE LOOKUP SOURCE $SOURCE"

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys
source = Path(sys.argv[1]).resolve(); out = Path(sys.argv[2])
retail = Path('disc/shop_menu.bin').read_bytes()
assert len(retail) == 0xD800
assert sha256(retail).hexdigest() == '7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf'
slice_ = retail[0x98D8:0x991C]
assert len(slice_) == 68
assert sha256(slice_).hexdigest() == '62321bfbdb3756149ced5268ea0e7022afa1688253efb270110f1a15d3d8740c'
test_source = Path('pc_port/tests/shop_byte_lookup_retail_test.c').resolve()
runner = Path('pc_port/tests/run_shop_byte_lookup_retail_test.sh').resolve()
(out / 'pins.json').write_text(json.dumps({
    'scope': 'actual shop misc.c func_801CE8D8 against retail 68-byte slice',
    'retail_module': {'size': len(retail), 'sha256': sha256(retail).hexdigest()},
    'retail_function': {'entry': '0x801CE8D8', 'end': '0x801CE91C', 'size': len(slice_),
                        'sha256': sha256(slice_).hexdigest()},
    'source_pins': {str(source): sha256(source.read_bytes()).hexdigest(),
                    str(test_source): sha256(test_source.read_bytes()).hexdigest(),
                    str(runner): sha256(runner.read_bytes()).hexdigest()},
}, indent=2) + '\n')
PY

COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode"); link_flags=()
    if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); link_flags=(-fsanitize=undefined); fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c "$SOURCE" -o "$OUT/$mode.source.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/shop_byte_lookup_retail_test.c -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    clang -no-pie "${flags[@]}" "${link_flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
    "$OUT/$mode" disc/shop_menu.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log" >&2; exit 1; }
    cat "$OUT/$mode.log"
done

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys
source_path = Path(sys.argv[1]).resolve(); out = Path(sys.argv[2]); source = source_path.read_text()
start = source.index('u8 func_801CE8D8(')
marker = '\n\nINCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801CE91C);'
end = source.find(marker, start)
if end < 0:
    end = len(source)
body = source[start:end]
mutants = {
    'wrong-target-mask': ('target &= 0xFF;', 'target &= 0x0F;'),
    'missing-count-guard': ('if (count > 0) {', 'if (count >= 0) {'),
    'truncated-end-pointer': ('intptr_t end;', 's32 end;'),
    'wrong-return-byte': ('result = *pValues;', 'result = *pKeys;'),
}
manifest = {}
for name, (old, new) in mutants.items():
    assert body.count(old) == 1, (name, body.count(old))
    mutant = source[:start] + body.replace(old, new, 1) + source[end:]
    path = out / (name + '.c'); path.write_text(mutant)
    manifest[name] = {'path': str(path), 'sha256': sha256(mutant.encode()).hexdigest()}
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
for mutant in wrong-target-mask missing-count-guard truncated-end-pointer wrong-return-byte; do
    gcc "${COMMON[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.source.o"
    clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant"
    rc=0; "$OUT/$mutant" disc/shop_menu.bin > "$OUT/$mutant.log" 2>&1 || rc=$?
    if [ "$rc" = 0 ] || ! rg -q '^SHOP BYTE LOOKUP FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        echo "SHOP BYTE LOOKUP negative control survived: $mutant" >&2
        exit 1
    fi
    echo "SHOP BYTE LOOKUP negative control rejected: $mutant"
done
python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys
out = Path(sys.argv[1]); pins = json.loads((out / 'pins.json').read_text())
for path, wanted in pins['source_pins'].items(): assert sha256(Path(path).read_bytes()).hexdigest() == wanted, path
print('SHOP BYTE LOOKUP source pins unchanged')
PY
echo 'SHOP BYTE LOOKUP test matrix complete'
