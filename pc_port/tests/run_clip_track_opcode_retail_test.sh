#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_track_opcode_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from tools.scripts.audit_field_clip_vm import inventory, read_overlay
inventory(read_overlay('disc/disc1.bin'))
print('CLIP TRACK retail overlay/VM/dispatch SHA pins PASS')
PY
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie \
        "${flags[@]}" pc_port/tests/clip_track_opcode_retail_test.c \
        pc_port/src/field_clip_data.c pc_port/src/battle_mips_adapter.c \
        -o "$OUT/$opt.test"
    "$OUT/$opt.test"
    python3 - "$OUT/$opt.test" <<'PY'
import re
import signal
import subprocess
import sys

binary = sys.argv[1]
for slot in (10, 255):
    result = subprocess.run([binary, '--reject-slot', str(slot)],
                            text=True, capture_output=True, check=False)
    ip = re.fullmatch(r'expected-ip=([0-9a-f]{8})\n', result.stdout)
    if result.returncode != -signal.SIGABRT or not ip:
        raise SystemExit(f'CLIP TRACK GUARD FAIL slot={slot}: '
                         f'expected SIGABRT/IP, got {result.returncode}: {result.stderr!r}')
    expected = ('[obj-ovly] retail clip opcode 1F slot outside native registry: '
                f'{slot} ip={ip.group(1)}\n')
    if result.stderr != expected:
        raise SystemExit(f'CLIP TRACK GUARD FAIL slot={slot}: '
                         f'diagnostic mismatch {result.stderr!r}')
print('CLIP TRACK native guard PASS: slots 10/255 reject with SIGABRT and exact diagnostic; no retail parity claim')
PY
done
echo 'CLIP TRACK 1D/1F/23/25-direct/26/27 O0/O2/UBSAN PASS'

# Mutate isolated build copies. Require each exact anchor once, a successful
# build, and a differential assertion failure: a crash is not a passing control.
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys

out = Path(sys.argv[1])
source = Path('pc_port/src/field_clip_data.c').read_text()
mutants = {
    'handled_only': ('1d', 'uint16_t w0 = take(state);',
                     'return 1;\n        uint16_t w0 = take(state);'),
    'limit': ('1d', 'state->limit = -1;', 'state->limit = 0;'),
    'operand': ('1d', 'state->operand = w1;', 'state->operand = w0;'),
    'working_state': ('1d', 'state->limit = -1;',
                      'state->limit = -1; state->ticks = 0;'),
    'flags_mask': ('1d', 'w0 & 0xff', 'w0 & 0x7f'),
    'mode_sign': ('1d', 'w0 >> 8', '(int8_t)(w0 >> 8)'),
    'payload_sign': ('1d', 'values[i] = (int16_t)take(state)',
                     'values[i] = (uint16_t)take(state)'),
    'node_stride': ('1d', '(uint32_t)parameter * 124u',
                    '(uint32_t)parameter * 120u'),
    'entry_object': ('1f', 'func_801E6830(state->origin, parameter,',
                     'func_801E6830(object, parameter,'),
    'slot_low_bits': ('1f', '&state->operand) & 0xffu', '&state->operand) & 7u'),
    'null_slot': ('1f', 'D_801E8670[slot] != 0', 'D_801E8670[slot] != 0 || slot == 3'),
    'consumed_stream': ('23', 'int32_t index = (int16_t)take(state);',
                        'int32_t index = (int16_t)take(state); state->stream -= 2u;'),
    'index_sign': ('23', '(int16_t)take(state)', '(uint16_t)take(state)'),
    'direct_target': ('25', 'target[0x5c] = object[0x20];',
                      '((uint8_t*)(uintptr_t)word(target))[0x5c] = object[0x20];'),
    'store_order': ('25',
                    'put_half(target + 0x5e, selector >> 8);\n            target[0x5c] = object[0x20];',
                    'target[0x5c] = object[0x20];\n            put_half(target + 0x5e, selector >> 8);'),
    'resolver_read_order': ('25',
                            '(void)func_801E6830(object, selector & 0xff, &state->operand);\n'
                            '        uint16_t value_x = take(state);\n'
                            '        uint16_t value_y = take(state);\n'
                            '        uint16_t value_z = take(state);',
                            'uint16_t value_x = take(state);\n'
                            '        uint16_t value_y = take(state);\n'
                            '        uint16_t value_z = take(state);\n'
                            '        (void)func_801E6830(object, selector & 0xff, &state->operand);'),
    'selector_half': ('25', 'put_half(target + 0x5e, selector >> 8)',
                      'put_half(target + 0x5e, selector & 0xff)'),
    'attach_flag': ('25', 'parameter & 2u', 'parameter & 1u'),
    'high_mask': ('25', 'slot < 8', 'slot < 9'),
    'detach_target': ('26', 'if (target) target[0x5c] = 0xff;',
                      'if (target) target[0x5d] = 0xff;'),
    'active_object': ('26', 'func_801E6830(object, parameter,',
                      'func_801E6830(state->origin, parameter,'),
    'scale_sign': ('27', '(int16_t)half(object + 0x1c)',
                   '(uint16_t)half(object + 0x1c)'),
    'dependency_choice': ('27', 'object[0x37] != 0', 'object[0x37] == 0'),
}
for name, (opcode, old, new) in mutants.items():
    start = source.index(f'    case 0x{opcode}: {{')
    end = source.index('\n    case ', start + 1)
    body = source[start:end]
    if body.count(old) != 1:
        raise SystemExit(f'CLIP TRACK mutation anchor drift: {name}')
    mutated = source[:start] + body.replace(old, new, 1) + source[end:]
    (out / f'{name}.c').write_text(mutated)
(out / 'mutants.list').write_text(''.join(f'{name}\n' for name in mutants))
PY
while IFS= read -r mutant; do
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie -O2 \
        pc_port/tests/clip_track_opcode_retail_test.c "$OUT/$mutant.c" \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        echo "CLIP TRACK mutant survived: $mutant" >&2
        exit 1
    fi
    if [ "$status" -ne 1 ]; then
        echo "CLIP TRACK mutant did not fail through an assertion: $mutant status=$status" >&2
        exit 1
    fi
    rg -q '^CLIP TRACK FAIL' "$OUT/$mutant.log"
done < "$OUT/mutants.list"
echo 'CLIP TRACK negative controls PASS: 23 handler/state/stream/mask/sign/slot/order/dependency regressions rejected'
