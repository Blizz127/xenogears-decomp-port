#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_blend_opcode_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from tools.scripts.audit_field_clip_vm import BASE, inventory, read_overlay, require_hash
payload = read_overlay('disc/disc1.bin')
inventory(payload)
require_hash(payload[0x801e3fd4-BASE:0x801e4068-BASE],
             '36059f865486809e08c946d3bdaf4b4a5cf45bf469f9c2bac3a370097fbcc290',
             'opcode13 handler')
PY
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie \
        "${flags[@]}" pc_port/tests/clip_blend_opcode_retail_test.c \
        pc_port/src/field_clip_data.c pc_port/src/battle_mips_adapter.c \
        -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
echo 'CLIP BLEND opcode13 O0/O2/UBSan PASS'

# Mutate private source copies, never the shared production file. Every
# requested edit must match once; a stale mutation cannot count as a kill.
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys

out = Path(sys.argv[1])
source = Path('pc_port/src/field_clip_data.c').read_text()
start = source.index('    case 0x13: {\n')
end = source.index('    case ', start + 5)
block = source[start:end]
mutants = {}

def replaced(old, new):
    assert block.count(old) == 1, f'mutation source drift: {old!r}'
    return block.replace(old, new, 1)

def add(name, old, new):
    mutants[name] = replaced(old, new)

add('unhandled', '    case 0x13: {\n', '    case 0x13: {\n        return 0;\n')
mutants['successful_noop'] = '    case 0x13: { break; }\n'
add('selector', 'object, control & 0xff, &flags);', 'object, parameter, &flags);')
add('tag', 'parameter, blend & 0xff, control >> 8);',
    'parameter, blend & 0xff, control & 0xff);')
add('loop', 'parameter, blend & 0xff, control >> 8);',
    'parameter, blend >> 8, control >> 8);')
add('duration', 'root, pose, blend >> 8,', 'root, pose, blend & 0xff,')
add('absolute', 'parameter, blend & 0xff, control >> 8);',
    '(parameter & 1u), blend & 0xff, control >> 8);')
add('signed_tag', 'parameter, blend & 0xff, control >> 8);',
    'parameter, blend & 0xff, (int8_t)(control >> 8));')
add('signed_loop', 'parameter, blend & 0xff, control >> 8);',
    'parameter, (int8_t)blend, control >> 8);')
add('global_inverted', 'if (D_801E85CC != 0)', 'if (D_801E85CC == 0)')
add('global_only_one', 'if (D_801E85CC != 0)', 'if (D_801E85CC == 1)')
root = '        uint8_t* root = (uint8_t*)(uintptr_t)word(object + 4);\n'
add('lookup_flags', root, '        if (flags != 0) break;\n' + root)
mutants['root_before_lookup'] = replaced(root, '').replace(
    '        uint32_t flags;\n', root + '        uint32_t flags;\n', 1)
mutants['global_before_lookup'] = replaced('if (D_801E85CC != 0)',
    'if (saved_global != 0)').replace('    case 0x13: {\n',
    '    case 0x13: {\n        uint32_t saved_global = D_801E85CC;\n', 1)
blend = '        uint16_t blend = take(state);\n        state->operand = blend;\n'
mutants['blend_after_lookup'] = replaced(blend, '').replace(root, blend + root, 1)
add('operand', 'state->operand = blend;', 'state->operand = control;')
add('limit_omitted', '        state->limit = -1;\n', '')
mutants['limit_before_lookup'] = replaced('        state->limit = -1;\n', '').replace(
    '        uint32_t flags;\n', '        state->limit = -1;\n        uint32_t flags;\n', 1)
sentinel = '        (void)func_801E632C(object);\n'
add('sentinel_omitted', sentinel, '')
mutants['sentinel_before_blend'] = replaced(sentinel, '').replace(
    '        if (D_801E85CC != 0)\n', sentinel + '        if (D_801E85CC != 0)\n', 1)
add('sentinel_origin', '(void)func_801E632C(object);', '(void)func_801E632C(state->origin);')
add('pool', 'func_801DF0B4(state->pool, root, pose,', 'func_801DF0B4(NULL, root, pose,')
add('pose', 'func_801DF0B4(state->pool, root, pose,', 'func_801DF0B4(state->pool, root, root,')
for name, mutated in mutants.items():
    assert mutated != block, f'unchanged mutation: {name}'
    (out / f'{name}.c').write_text(source[:start] + mutated + source[end:])
(out / 'mutants.list').write_text(''.join(f'{name}\n' for name in mutants))
PY
while IFS= read -r mutant; do
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie -O2 \
        pc_port/tests/clip_blend_opcode_retail_test.c "$OUT/$mutant.c" \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "CLIP BLEND mutant survived: $mutant" >&2
        exit 1
    fi
    rg -q 'CLIP BLEND FAIL' "$OUT/$mutant.log"
    echo "CLIP BLEND negative control PASS: $mutant"
done < "$OUT/mutants.list"
echo 'CLIP BLEND all 23 production mutation controls PASS'
