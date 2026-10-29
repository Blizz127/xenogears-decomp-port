#!/usr/bin/env bash
# Unit proof for XENO_FIELD_STEP_PAD_INPUT (see field_step_pad_input_test.c).
# Source guards pin the production call order the proof relies on; the
# FieldPollControllers drain and the game->physical remap are lifted verbatim
# from production; a mutant that clocks steps per vblank must fail.
# The test TU adds -Wno-address only because it defines the remap arrays the
# lifted helper NULL-checks (they are #pragma weak in production).
# Not runtime evidence.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/field_step_pad_input_test.XXXXXXXX)
export OUT
echo "FIELD STEP PAD artifacts: $OUT"

python3 - <<'PY'
import os, re
from pathlib import Path
out = Path(os.environ['OUT'])

# Guard 1: vblank order in func_8003634C -- prepare hook, then ControllerPoll,
# then ControllerPushState.
c2 = Path('src/slus_006.64/system/controller2.c').read_text()
body = c2[c2.index('void func_8003634C(void) {'):]
body = body[:body.index('\n}\n')]
order = [body.index(x) for x in ('PcPort_PrepareControllerPoll();', 'ControllerPoll();',
                                 'ControllerPushState();')]
assert order == sorted(order), 'func_8003634C order changed'

# Guard 2: the apply runs inside PcPort_PrepareControllerPoll, after the real
# pad read and the other pad-buffer injector, before anything else.
compat = Path('pc_port/src/psyq_compat.c').read_text()
prep = compat[compat.index('void PcPort_PrepareControllerPoll(void)\n{'):]
prep = prep[:prep.index('\n}\n')]
order = [prep.index(x) for x in ('PsyX_UpdateInput();', 'PcPort_PadTestInputInject();',
                                 'PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical,',
                                 'PcPort_PadOnControl();')]
assert order == sorted(order), 'PrepareControllerPoll order changed'
assert compat.count('PcPort_FieldStepPadApply(') == 2  # declaration + call

# Guard 3: the drain clock -- FieldPollControllers calls the merge exactly
# once, after the drain and both masks, and the merge notes the drain first.
misc2 = Path('src/field/main/misc2.c').read_text()
start = misc2.index('#else\nvoid FieldPollControllers(void) {') + len('#else\n')
end = misc2.index('    /* Headless field-test:', start)
lifted = misc2[start:end]
for needed in ('while (ControllerPopState() != 0)', 'g_FieldControl.controllerBtnMask',
               'D_800AFE9C &= D_800ADB00;',
               'PcPort_FieldTestInputMerge(&D_800AFE9C, &D_800C3900, &D_800C2694);'):
    assert lifted.count(needed) == 1, needed
assert lifted.index('D_800AFE9C &= D_800ADB00;') < lifted.index('PcPort_FieldTestInputMerge(')
assert misc2.count('PcPort_FieldTestInputMerge(&') == 1
ti = Path('pc_port/src/test_input.c').read_text()
merge = ti[ti.index('void PcPort_FieldTestInputMerge('):]
assert merge.index('PcPort_FieldStepPadNoteDrain();') < merge.index('if (s_hold_frames')

# Lift the remap helper verbatim.
m = re.search(r'static unsigned short PcPort_PadTestGameToPhysical\(unsigned short game\)\n\{.*?\n\}\n',
              compat, re.S)
assert m, 'PcPort_PadTestGameToPhysical not found'
(out / 'field_step_pad_lifted.h').write_text(
    '/* Lifted verbatim by run_field_step_pad_input_test.sh; do not edit. */\n'
    + m.group(0) + '\n' + lifted + '#endif\n}\n')

# Mutant: the step clock advances on every vblank instead of every drain.
needle = '    s_step_pad_vblanks++;\n    return 0;\n'
assert ti.count(needle) == 1
(out / 'mutant_test_input.c').write_text(
    ti.replace(needle, '    s_step_pad_vblanks++;\n    s_step_pad_step++;\n    return 0;\n'))
print('FIELD STEP PAD source guards PASS (vblank order, apply seam, drain clock)')
PY

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -ffunction-sections -fdata-sections
      -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT")
run_modes() {
    local bin=$1
    for layout in identity usa; do
        for p in 0 1 2; do "$bin" "main-$layout" "$p"; done
    done
    for mode in masks contrast overflow map unset suspend handover; do "$bin" "$mode"; done
    for which in field pad schedule map handover-mismatch handover-nomap; do "$bin" refuse "$which"; done
}
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    [ "$opt" = UBSan ] && flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -w -c src/slus_006.64/system/controller.c -o "$OUT/$opt.controller.o"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/test_input.c -o "$OUT/$opt.test_input.o"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -Wall -Wextra -Werror -Wno-unused-function -Wno-address \
        pc_port/tests/field_step_pad_input_test.c "$OUT/$opt.controller.o" "$OUT/$opt.test_input.o" \
        -o "$OUT/$opt.test"
    run_modes "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1 || { cat "$OUT/$opt.log" >&2; echo "FIELD STEP PAD RED $opt" >&2; exit 1; }
    echo "FIELD STEP PAD $opt PASS ($(grep -c PASS "$OUT/$opt.log") checks)"
done

"${CC:-gcc}" "${BASE[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/mutant_test_input.c" -o "$OUT/mutant.test_input.o"
"${CC:-gcc}" "${BASE[@]}" -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-address \
    pc_port/tests/field_step_pad_input_test.c "$OUT/O2.controller.o" "$OUT/mutant.test_input.o" -o "$OUT/mutant.test"
if "$OUT/mutant.test" main-usa 1 > "$OUT/mutant.log" 2>&1; then
    echo "FIELD STEP PAD FAIL mutant (per-vblank clock) survived" >&2
    exit 1
fi
grep -q 'FIELD STEP PAD FAIL' "$OUT/mutant.log"
echo "FIELD STEP PAD negative control PASS: per-vblank clock mutant fails"
