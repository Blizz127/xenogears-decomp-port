#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
main="$repo_root/pc_port/extern/PsyCross/src/PsyX_main.cpp"
build="$repo_root/pc_port/build_port.sh"
patch="$repo_root/pc_port/patches/psycross_host_toolbar.patch"
toolbar="$repo_root/pc_port/src/psycross_host_toolbar.inl"

test -f "$patch"
test -f "$toolbar"
grep -q '_xeno_host_toolbar' "$main"
grep -q 'PsyX_HostToolbarInitialise();' "$main"
grep -q 'PsyX_HostToolbarHandleEvent(&event)' "$main"
grep -q 'PsyX_HostToolbarDraw();' "$main"
grep -q 'PsyX_HostToolbarShutdown();' "$main"
grep -q 'psycross_host_toolbar.patch' "$build"
grep -q 'PC_PORT_TOOLBAR_QUICK_SAVE' "$toolbar"
grep -q 'PC_PORT_TOOLBAR_QUICK_LOAD' "$toolbar"
grep -q 'PC_PORT_TOOLBAR_RECORD' "$toolbar"
grep -q 'PcPort_QuickCheckpointGetUiState' "$toolbar"
grep -q 'PC_PORT_QUICK_UI_SAVE_PENDING' "$toolbar"
grep -q 'PC_PORT_QUICK_UI_LOAD_PENDING' "$toolbar"
grep -q 'event->motion.y -= PC_PORT_HOST_TOOLBAR_HEIGHT' "$toolbar"

# Host chrome is drawn after framebuffer recording and before presentation, so
# recordings remain retail-clean while the controls remain visible on screen.
python3 - "$main" <<'PY'
from pathlib import Path
import sys

text = Path(sys.argv[1]).read_text()
for signature in ("PsyX_PresentDisplayFromVRAM(void)", "PsyX_EndScene()"):
    start = text.index(signature)
    end = text.index("\n}", start)
    body = text[start:end]
    assert body.index("PsyX_RecordPresentedFrame();") < body.index("PsyX_HostToolbarDraw();")
    assert body.index("PsyX_HostToolbarDraw();") < body.index("GR_SwapWindow();")
PY

echo 'PsyCross host toolbar regression: PASS'
