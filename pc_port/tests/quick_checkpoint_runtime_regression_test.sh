#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
runtime="$repo_root/pc_port/src/quick_checkpoint.c"
field="$repo_root/src/field/main/main.c"
psyx="$repo_root/pc_port/extern/PsyCross/src/PsyX_main.cpp"
build="$repo_root/pc_port/build_port.sh"

test -f "$runtime"
grep -q 'pc_port/src/quick_checkpoint_file.c' "$build"
grep -q 'pc_port/src/quick_checkpoint.c' "$build"
grep -q 'PcPort_QuickCheckpointSetFieldActive(1)' "$field"
grep -q 'PcPort_QuickCheckpointPoll()' "$field"
grep -q 'PcPort_QuickCheckpointCommitLoad()' "$field"
grep -q 'PcPort_QuickCheckpointRestorePlayer()' "$field"
grep -q 'case SDL_SCANCODE_F11:' "$psyx"
grep -q 'PcPort_QuickCheckpointRequestSave' "$psyx"
grep -q 'case SDL_SCANCODE_F12:' "$psyx"
grep -q 'PcPort_QuickCheckpointRequestLoad' "$psyx"
grep -q 'PcPort_QuickRequestTakeIfSafe' "$runtime"
grep -q 'PcPort_QuickCheckpointGetUiState' "$runtime"
if grep -q 'request rejected: wait for free field control' "$runtime"; then
    echo 'unsafe checkpoint requests are still dropped' >&2
    exit 1
fi

# Host keys (psycross_host_hotkeys.patch): F11/F12 checkpoint, F6 is the
# fast-forward toggle, PsyCross's F3-F5 debug toggles are unbound.  The
# checkpoint keys never enter the active-low PlayStation pad mapping.
grep -A3 -q 'case SDL_SCANCODE_F6:' "$psyx"
if grep -Eq 'case SDL_SCANCODE_F[345]:' "$psyx"; then
    echo 'PsyCross F3-F5 debug toggles are bound in the default build' >&2
    exit 1
fi
if grep -Eq 'SDL_SCANCODE_F1[12]' \
    "$repo_root/pc_port/extern/PsyCross/src/pad/PsyX_pad.cpp"; then
    echo 'checkpoint hotkey leaked into PlayStation pad mapping' >&2
    exit 1
fi

echo 'quick checkpoint runtime integration: PASS'
