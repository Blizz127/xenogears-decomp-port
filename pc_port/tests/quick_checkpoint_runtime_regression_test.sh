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
grep -q 'SDL_SCANCODE_F7' "$psyx"
grep -q 'PcPort_QuickCheckpointRequestSave' "$psyx"
grep -q 'SDL_SCANCODE_F8' "$psyx"
grep -q 'PcPort_QuickCheckpointRequestLoad' "$psyx"
grep -q 'PcPort_QuickRequestTakeIfSafe' "$runtime"
grep -q 'PcPort_QuickCheckpointGetUiState' "$runtime"
if grep -q 'request rejected: wait for free field control' "$runtime"; then
    echo 'unsafe checkpoint requests are still dropped' >&2
    exit 1
fi

# Existing PsyCross F5/F6 debug bindings remain intact; checkpoint keys are
# host-only and never enter the active-low PlayStation pad mapping.
grep -A3 -q 'SDL_SCANCODE_F5' "$psyx"
grep -A3 -q 'SDL_SCANCODE_F6' "$psyx"
if rg -q 'SDL_SCANCODE_F[78]' \
    "$repo_root/pc_port/extern/PsyCross/src/pad/PsyX_pad.cpp"; then
    echo 'checkpoint hotkey leaked into PlayStation pad mapping' >&2
    exit 1
fi

echo 'quick checkpoint runtime integration: PASS'
