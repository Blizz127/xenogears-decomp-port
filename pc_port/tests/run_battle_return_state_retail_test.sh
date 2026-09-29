#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
SCRATCH_ROOT=/tmp/xeno-battle-return-test-20260906
mkdir -p "$SCRATCH_ROOT"
OUT=$(mktemp -d "$SCRATCH_ROOT/run.XXXXXXXX")
SOURCE=${BATTLE_RETURN_STATE_SOURCE:-src/slus_006.64/system/temp3.c}

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys

source = Path(sys.argv[1])
out = Path(sys.argv[2])
assert source.is_file(), source
disc = Path('disc/SLUS_006.64').read_bytes()
disc_sha = sha256(disc).hexdigest()
assert disc_sha == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
start = 0x8001b6c4 - 0x8000f800
retail = disc[start:start + 0x180]
retail_sha = sha256(retail).hexdigest()
assert retail_sha == '05a824d3eac6a3fe154d9ae725c8ab846615bdd0a8624eb80c48cb01a756b625'

text = source.read_text()
needle = 'void func_8001B6C4(void)'
begin = text.index('#ifdef XENO_PC_PORT', text.index('/* func_8001B6C4 addresses'))
# Select the XENO_PC_PORT arms while extracting so mutually exclusive braces do
# not confuse the bounded function scanner.
selected = []
enabled = True
stack = []
for line in text[begin:].splitlines(keepends=True):
    stripped = line.strip()
    if stripped == '#ifdef XENO_PC_PORT':
        stack.append(enabled); enabled = enabled and True; continue
    if stripped == '#else' and stack:
        enabled = stack[-1] and not enabled; continue
    if stripped == '#endif' and stack:
        enabled = stack.pop(); continue
    if enabled: selected.append(line)
text = ''.join(selected)
begin = text.index(needle)
prefix = text[:begin]
brace = text.index('{', begin)
depth = 0
end = None
for i in range(brace, len(text)):
    if text[i] == '{': depth += 1
    elif text[i] == '}':
        depth -= 1
        if depth == 0:
            end = i + 1
            break
assert end is not None
(out / 'production_body.c').write_text(prefix + text[begin:end] + '\n')
(out / 'retail_8001b6c4_8001b844.bin').write_bytes(retail)
(out / 'pins.json').write_text(json.dumps({
    'disc': 'disc/SLUS_006.64', 'disc_sha256': disc_sha,
    'retail_range': '0x8001B6C4..0x8001B844',
    'retail_size': len(retail), 'retail_sha256': retail_sha,
    'source': str(source), 'source_sha256': sha256(source.read_bytes()).hexdigest(),
    'extracted_body_sha256': sha256((out / 'production_body.c').read_bytes()).hexdigest(),
    'harness_sha256': sha256(Path('pc_port/tests/battle_return_state_retail_test.c').read_bytes()).hexdigest(),
    'runner_sha256': sha256(Path('pc_port/tests/run_battle_return_state_retail_test.sh').read_bytes()).hexdigest(),
    'boundary_spies': {
        'ArchiveCdDataSync': '0x80028A60', 'ArchiveSetIndex': '0x80028470',
        'func_8003747C': '0x8003747C', 'FontLoadFont': '0x800374E8',
        'func_8001B844': '0x8001B844', 'func_80070F40': '0x80070F40',
        'GamePartySignalReinitialize': '0x8001AC94',
        'ChangeGameState': '0x8001996C', 'MainLoop': '0x80019ACC'},
    'scope_limit': 'D_8005917C[0] fixed -1; debug FontLoadFont ABI is separate',
}, indent=2) + '\n')
PY

common=(
    -std=gnu17 -g -fno-pie -fno-builtin -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -Ipc_port/src -Ipc_port/include_shim -Iinclude
    -Wall -Wextra -Werror -Wno-int-conversion
)

build_and_run() {
    local label=$1 opt=$2 body=${3:-$OUT/production_body.c} cc=gcc
    local -a flags=(-"$opt")
    if [[ "$opt" == UBSan ]]; then
        cc=clang
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "$cc" "${common[@]}" "-DBATTLE_RETURN_BODY=\"$body\"" "${flags[@]}" -c \
        pc_port/tests/battle_return_state_retail_test.c -o "$OUT/$label.test.o" \
        >"$OUT/$label.build.log" 2>&1 || return $?
    "$cc" "${common[@]}" "-DBATTLE_RETURN_BODY=\"$body\"" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c \
        -o "$OUT/$label.adapter.o" >>"$OUT/$label.build.log" 2>&1 || return $?
    "$cc" "${flags[@]}" -no-pie -Wl,--gc-sections \
        "$OUT/$label.test.o" "$OUT/$label.adapter.o" -o "$OUT/$label.test" \
        >>"$OUT/$label.build.log" 2>&1 || return $?
    "$OUT/$label.test" >"$OUT/$label.log" 2>&1 || return $?
}

source_status=0
for opt in O0 O2 UBSan; do
    if ! build_and_run "source-$opt" "$opt"; then
        source_status=1
        cat "$OUT/source-$opt.build.log" >&2 || true
        cat "$OUT/source-$opt.log" >&2 || true
    else
        cat "$OUT/source-$opt.log"
    fi
done

# A source that passes is challenged with semantic mutations. Each must be
# rejected by the same full retail oracle; this proves the checks are live.
if [[ "$source_status" == 0 ]]; then
    python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
body = (out / 'production_body.c').read_text()
mutations = {
    'host-result-split': ('state = *(u8*)PSX_ADDR(0x800C48EAu);', 'state = D_800C48EA;'),
    'host-guard-split': ('*(u8*)PSX_ADDR(0x800D3338u) == 0', 'D_800D3338 == 0'),
    'mask-wide': ('D_8006F94E & 0x7FF', 'D_8006F94E & 0xFFFF'),
    'boundary-inclusive': ('tmp < 0x400', 'tmp <= 0x400'),
    'guard-state-original': ('next = 6;', 'next = D_800C48EA;'),
    'unknown-change-call': ('goto battle_return;',
                            'if (state == 0xff) { goto battle_return; } next = state;'),
    'reset-map0': ('D_8006F94E = 0x1EA;', 'D_8006F94E = 0x1EB;'),
    'return-flag-unconditional': ('battle_return:\n    if (T3_ABS7C == 0)',
                                  'battle_return:\n    if (1)'),
    'drop-reinitialize': ('GamePartySignalReinitialize();', '/* mutation: omitted */'),
}
for name, (old, new) in mutations.items():
    assert old in body, (name, old)
    mutated = body.replace(old, new, 1)
    assert mutated != body
    (out / f'mutant-{name}.c').write_text(mutated)
PY
    for mutant in host-result-split host-guard-split mask-wide boundary-inclusive \
                   guard-state-original unknown-change-call reset-map0 \
                   return-flag-unconditional drop-reinitialize; do
        set +e
        build_and_run "mutant-$mutant" O2 "$OUT/mutant-$mutant.c"
        status=$?
        set -e
        if [[ "$status" == 0 ]] || ! rg -q 'BATTLE RETURN STATE FAIL' "$OUT/mutant-$mutant.log"; then
            cat "$OUT/mutant-$mutant.build.log" "$OUT/mutant-$mutant.log" >&2
            echo "BATTLE RETURN STATE FAIL negative control survived: $mutant" >&2
            exit 1
        fi
    done
    echo "BATTLE RETURN STATE negative controls PASS: 9 semantic mutations rejected"
fi

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys
source, out = Path(sys.argv[1]), Path(sys.argv[2])
pins = json.loads((out / 'pins.json').read_text())
checks = [(source, 'source_sha256'),
          (Path('pc_port/tests/battle_return_state_retail_test.c'), 'harness_sha256'),
          (Path('pc_port/tests/run_battle_return_state_retail_test.sh'), 'runner_sha256')]
for path, key in checks:
    assert sha256(path.read_bytes()).hexdigest() == pins[key], f'changed during run: {path}'
assert sha256((out / 'production_body.c').read_bytes()).hexdigest() == pins['extracted_body_sha256'], \
       'extracted production body changed during run'
PY

if [[ "$source_status" != 0 ]]; then
    echo "BATTLE RETURN STATE RED source=$SOURCE evidence=$OUT" >&2
    exit 1
fi
echo "BATTLE RETURN STATE PASS source=$SOURCE evidence=$OUT"
