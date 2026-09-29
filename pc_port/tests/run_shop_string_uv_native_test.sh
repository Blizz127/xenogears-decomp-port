#!/usr/bin/env bash
# Native semantic differential for shop func_801C5A7C (string UV/tpage).
# Production C vs independent reference model + 6 distinguished mutants,
# plus retail-slice and source-shape pins. GCC UBSan is unavailable in this
# environment (missing libubsan runtime); UBSan runs under clang instead.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

SOURCE=${SHOP_STRING_UV_SOURCE:-src/shop_menu/main/misc.c}
case "$SOURCE" in /*) ;; *) SOURCE="$PWD/$SOURCE" ;; esac
test -f "$SOURCE"
OUT=${SHOP_STRING_UV_OUT:-$(mktemp -d /tmp/xeno-shop-string-uv.XXXXXXXX)}
mkdir -p "$OUT"
echo "SHOP STRING UV OUTPUT $OUT"
echo "SHOP STRING UV SOURCE $SOURCE"

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys
source = Path(sys.argv[1]).resolve(); out = Path(sys.argv[2])
retail = Path('disc/shop_menu.bin').read_bytes()
assert len(retail) == 0xD800, len(retail)
assert sha256(retail).hexdigest() == '7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf'
slice_ = retail[0xA7C:0xA7C + 576]
assert len(slice_) == 576
assert sha256(slice_).hexdigest() == 'd71c52a1f841c5be08425bba883a28d3f2ac47acccc4ee1062c84316f1bd3e09'
text = source.read_text()
assert 'u16 blend;' in text, 'blend must stay u16 for the retail or-operand order'
assert 'u = (half & 1) * 128;' in text, 'mid-block u copy must stay for the retail move'
test_source = Path('pc_port/tests/shop_string_uv_native_test.c').resolve()
runner = Path('pc_port/tests/run_shop_string_uv_native_test.sh').resolve()
(out / 'pins.json').write_text(json.dumps({
    'scope': 'actual shop misc.c func_801C5A7C vs native reference + mutants',
    'retail_module': {'size': len(retail), 'sha256': sha256(retail).hexdigest()},
    'retail_function': {'entry': '0x801C5A7C', 'end': '0x801C5CBC',
                        'size': len(slice_),
                        'sha256': sha256(slice_).hexdigest()},
    'source_pins': {str(source): sha256(source.read_bytes()).hexdigest(),
                    str(test_source): sha256(test_source.read_bytes()).hexdigest(),
                    str(runner): sha256(runner.read_bytes()).hexdigest()},
}, indent=2) + '\n')
print('PINS retail-slice + source-shape OK')
PY

COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    TCC=gcc; flags=(-"$mode"); link_flags=(); LLD=clang
    if [ "$mode" = UBSan ]; then
        # GCC tolerates the TU's legacy int/pointer conversions
        # (-fpermissive); clang provides the UBSan runtime that GCC's
        # missing libubsan cannot. Mixed GCC/Clang UBSan is the
        # project-accepted convention for this environment.
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
        link_flags=(-fsanitize=undefined)
    fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c "$SOURCE" -o "$OUT/$mode.source.o"
    "$TCC" "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/shop_string_uv_native_test.c -o "$OUT/$mode.test.o"
    "$LLD" -no-pie "${flags[@]}" "${link_flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" -o "$OUT/$mode"
    "$OUT/$mode" > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log" >&2; exit 1; }
    cat "$OUT/$mode.log"
done
echo "SHOP STRING UV NATIVE: PASS O0/O2/UBSan(clang)"
