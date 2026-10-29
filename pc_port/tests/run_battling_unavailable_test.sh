#!/usr/bin/env bash
# battling_unavailable -- TEMPORARY STOPGAP certificate: entering the unported
# Battling minigame (game state 4) must return to the field with a notice
# instead of crashing on its unbound g_MainGameStates[4] entry.  Delete with
# pc_port/src/battling_unavailable.c when the native Battling port binds it.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
SOURCE=$(realpath "${BATTLING_UNAVAILABLE_SOURCE:-pc_port/src/battling_unavailable.c}")
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

# MainLoop must run the guard before it reads the state's table entry.
python3 - <<'PY'
from pathlib import Path
src = Path('src/slus_006.64/main/main_loop.c').read_text()
body = src.split('void MainLoop(int errorCode) {', 1)[1]
guard = body.find('PcPort_BattlingUnavailableGuard(')
use = body.find('pGameState = &g_MainGameStates[g_CurGameState];')
assert guard != -1, 'MainLoop no longer calls the state-4 guard'
assert use != -1 and guard < use, 'guard runs after the state table is read'
pre = body[:guard]
assert pre.rfind('#ifdef XENO_PC_PORT') > pre.rfind('#endif'), \
    'guard is outside XENO_PC_PORT'
print('BATTLING GUARD MainLoop call site: before table read, XENO_PC_PORT only')
PY

build_and_run() {
    local mode="$1" src="$2"; shift 2
    "${CC:-clang}" -std=gnu17 -Wall -Wextra -Werror "$@" -Ipc_port/src \
        pc_port/tests/battling_unavailable_test.c "$src" -o "$out/$mode" || return 2
    "$out/$mode" > "$out/$mode.log" 2>&1
}

for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if ! build_and_run "$mode" "$SOURCE" "${flags[@]}"; then
        cat "$out/$mode.log"; echo "BATTLING GUARD REGIME FAILURE $mode" >&2; exit 1
    fi
    grep '^BATTLING GUARD PASS ' "$out/$mode.log" | sed "s/^/  $mode: /"
done

# Negative controls: each mutant of the guard must be rejected.
python3 - "$out" "$SOURCE" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1]); s = Path(sys.argv[2]).read_text()
muts = {
    'no-guard': ('if (state != PC_PORT_GAME_STATE_BATTLING || entryBound)\n        return 0;',
                 'return 0;'),
    'ignores-bound': ('|| entryBound)', ')'),
    'no-notice': ('    PcPort_QuickCheckpointNoticeSet(PcPort_BattlingUnavailableNotice);\n', ''),
    'fake-result': ('D_80050622[0] = 0;', 'D_80050622[0] = 0x81;'),
    'wrong-return': ('ChangeGameState(PC_PORT_GAME_STATE_FIELD);', 'ChangeGameState(0);'),
}
for name, (old, new) in muts.items():
    assert s.count(old) == 1, (name, s.count(old))
    (out / (name + '.c')).write_text(s.replace(old, new, 1))
PY
for m in no-guard ignores-bound no-notice fake-result wrong-return; do
    rc=0; build_and_run "mut-$m" "$out/$m.c" -O0 -Wno-unused-parameter || rc=$?
    if [ "$rc" -ne 1 ] || ! grep -q 'BATTLING GUARD FAIL' "$out/mut-$m.log"; then
        echo "BATTLING GUARD MUTANT NOT DETECTED (or did not build): $m rc=$rc" >&2; exit 1
    fi
    echo "BATTLING GUARD mutant rejected: $m"
done
echo 'BATTLING GUARD PASS (3 regimes, 5/5 mutants rejected)'
