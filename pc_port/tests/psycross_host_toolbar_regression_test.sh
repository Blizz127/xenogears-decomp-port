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
# r6.3: the clickable strip is retired. No reserved pixels, no mouse
# handling; the hooks only carry the dev-menu overlay.
grep -q 'extern "C" int PsyX_HostToolbarReservedHeight(void) { return 0; }' "$toolbar"
grep -q 'static int PsyX_HostToolbarContentHeight(int windowHeight) { return windowHeight; }' "$toolbar"
grep -q 'PsyX_PortDevMenuDraw();' "$toolbar"
if grep -Eq 'PC_PORT_HOST_TOOLBAR_HEIGHT|PC_PORT_TOOLBAR_|SDL_MOUSEBUTTON|SDL_SetWindowMinimumSize' "$toolbar"; then
    echo 'host toolbar strip remnants in psycross_host_toolbar.inl' >&2
    exit 1
fi

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

echo 'PsyCross host overlay (toolbar strip retired) regression: PASS'
