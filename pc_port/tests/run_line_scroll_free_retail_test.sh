#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
OUT=$(mktemp -d pc_port/build_native/line_scroll_free_retail_test.XXXXXXXX)
SOURCE=${LINE_SCROLL_FREE_SOURCE:-src/slus_006.64/graphics/line_scroll.c}
EXPECT_RED=${LINE_SCROLL_FREE_EXPECT_RED:-0}

python3 - "$SOURCE" "$OUT" "$SOURCE" <<'PY'
from hashlib import sha256
from pathlib import Path
import json
import struct
import sys

out = Path(sys.argv[2])
tested_source = Path(sys.argv[3])
assert tested_source.is_file(), tested_source
disc = Path('disc/SLUS_006.64').read_bytes()
assert sha256(disc).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
leaf_start = 0x8002800c - 0x8000f800
leaf = disc[leaf_start:leaf_start + 0x40]
assert len(leaf) == 0x40
assert sha256(leaf).hexdigest() == '1c7cae4712c0808dc1260fab9217d7465d97f091f86c55ed1ed75d7c9addc3c0'
stop = Path('pc_port/build_native/opening-heap-stop-5-72kk118u/stop-ram.bin').read_bytes()
assert len(stop) == 0x200000
assert sha256(stop).hexdigest() == 'b7771db5a18fcacb16f7f23220967d5c6152cd0104dfc334f46d0924491130f9'
record = stop[0xc3db8:0xc3db8 + 0x20]
assert record[:0x18] == bytes(0x18)
assert struct.unpack_from('<I', record, 0x14)[0] == 0
assert struct.unpack_from('<I', record, 0x18)[0] == 0x800e97dc
(out / 'retail_line_scroll_free_leaf.bin').write_bytes(leaf)
(out / 'snapshot_record.bin').write_bytes(record)
(out / 'pins.json').write_text(json.dumps({
    'disc_sha256': sha256(disc).hexdigest(),
    'tested_source': str(tested_source),
    'tested_source_sha256': sha256(tested_source.read_bytes()).hexdigest(),
    'test_harness_sha256': sha256(Path('pc_port/tests/line_scroll_free_retail_test.c').read_bytes()).hexdigest(),
    'runner_sha256': sha256(Path('pc_port/tests/run_line_scroll_free_retail_test.sh').read_bytes()).hexdigest(),
    'retail_leaf': {'address': '0x8002800c', 'size': 0x40,
                    'sha256': sha256(leaf).hexdigest()},
    'stop_ram_sha256': sha256(stop).hexdigest(),
    'snapshot_guest_record': '0x800c3db8',
    'snapshot_member_offset': '0x14',
    'snapshot_member_value': '0x00000000',
    'snapshot_native_poison_offset': '0x18',
    'snapshot_native_poison_value': '0x800e97dc',
}, indent=2) + '\n')
PY

common=(
    -std=gnu17 -O0 -g -fno-pie -fno-builtin -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -I. -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
    -include assert.h
)

build() {
    local label=$1 source=$2 opt=$3
    local -a flags=(-"$opt")
    if [[ "$opt" == UBSan ]]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra \
        "-DLINE_SCROLL_SOURCE=\"$source\"" \
        -c pc_port/tests/line_scroll_free_retail_test.c \
        -o "$OUT/$label.test.o" >"$OUT/$label.build.log" 2>&1
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -c \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$label.cpu.o" \
        >>"$OUT/$label.build.log" 2>&1
    gcc "${flags[@]}" -no-pie -Wl,--gc-sections \
        "$OUT/$label.test.o" "$OUT/$label.cpu.o" -o "$OUT/$label.test" \
        >>"$OUT/$label.build.log" 2>&1
}

run_expect_red() {
    local label=$1
    set +e
    "$OUT/$label.test" >"$OUT/$label.log" 2>&1
    local status=$?
    set -e
    if [[ "$status" == 0 ]] || ! rg -q 'LINE SCROLL FREE FAIL' "$OUT/$label.log"; then
        cat "$OUT/$label.build.log" "$OUT/$label.log" >&2
        echo "LINE SCROLL FREE expected RED missing: $label status=$status" >&2
        exit 1
    fi
    echo "LINE SCROLL FREE $label RED: current host struct reads +18 instead of retail +14"
}

for opt in O0 O2 UBSan; do
    build "source-$opt" "$SOURCE" "$opt"
    if [[ "$EXPECT_RED" == 1 ]]; then
        run_expect_red "source-$opt"
    else
        "$OUT/source-$opt.test" | tee "$OUT/source-$opt.log"
    fi
done

if [[ "$EXPECT_RED" == 1 ]]; then
    exit 1
fi

python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
out = Path(sys.argv[2])
mutants = {
    'offset18': source.replace('pLineScroll + 0x14', 'pLineScroll + 0x18'),
    'clear_before_free': source.replace(
        '    HeapFree(pWork);\n    guest_pWork = 0;',
        '    guest_pWork = 0;\n    line_scroll_copy((u8 *)pLineScroll + 0x14, &guest_pWork,\n           sizeof(guest_pWork));\n    HeapFree(pWork);'),
    'translate_low': source.replace(
        '    if (segment == 0x80000000u || segment == 0xa0000000u)\n        pWork = PSX_ADDR(guest_pWork);\n    else\n        pWork = (void *)(uintptr_t)guest_pWork;',
        '    (void)segment;\n    pWork = PSX_ADDR(guest_pWork);'),
    'clear_eight': source.replace(
        '    line_scroll_copy((u8 *)pLineScroll + 0x14, &guest_pWork,\n                     sizeof(guest_pWork));',
        '    for (unsigned i = 0; i < 8; ++i)\n        ((u8 *)pLineScroll + 0x14)[i] = 0;'),
}
for name, body in mutants.items():
    assert body != source, name
    (out / (name + '.c')).write_text(body)
PY

for mutant in offset18 clear_before_free translate_low clear_eight; do
    build "mutant-$mutant" "$OUT/$mutant.c" O2
    set +e
    "$OUT/mutant-$mutant.test" >"$OUT/mutant-$mutant.log" 2>&1
    status=$?
    set -e
    if [[ "$status" == 0 ]] || ! rg -q 'LINE SCROLL FREE FAIL' "$OUT/mutant-$mutant.log"; then
        cat "$OUT/mutant-$mutant.build.log" "$OUT/mutant-$mutant.log" >&2
        echo "LINE SCROLL FREE negative control survived: $mutant" >&2
        exit 1
    fi
done

echo "LINE SCROLL FREE negative controls PASS: offset18 clear-before-free translate-low clear-eight"

python3 - "$OUT" "$SOURCE" <<'PYVERIFY'
from pathlib import Path
from hashlib import sha256
import json,sys
pins=json.loads((Path(sys.argv[1])/'pins.json').read_text())
for name,key in [(sys.argv[2],'tested_source_sha256'),('pc_port/tests/line_scroll_free_retail_test.c','test_harness_sha256'),('pc_port/tests/run_line_scroll_free_retail_test.sh','runner_sha256')]:
    assert sha256(Path(name).read_bytes()).hexdigest()==pins[key], 'Source changed during test: '+name
PYVERIFY
echo "LINE SCROLL FREE GREEN: $OUT"
