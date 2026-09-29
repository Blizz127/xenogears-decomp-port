#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MAIN="$ROOT/pc_port/extern/PsyCross/src/PsyX_main.cpp"
LIBCD="$ROOT/pc_port/extern/PsyCross/src/psx/LIBCD.C"
PATCH="$ROOT/pc_port/patches/psycross_shutdown_cd_join.patch"

shutdown_body="$(sed -n '/void PsyX_Shutdown()/,/^}/p' "$MAIN")"
cd_shutdown_body="$(sed -n '/void PsyX_CD_Shutdown(void)/,/^}/p' "$LIBCD")"

if ! grep -Fq 'PsyX_CD_Shutdown();' <<<"$shutdown_body"; then
    echo 'FAIL: PsyX_Shutdown does not quiesce the CD spooler' >&2
    exit 1
fi
if ! grep -Fq '_eCdControlF_Pause();' <<<"$cd_shutdown_body" ||
   ! grep -Fq '_eCdSpoolerJoin();' <<<"$cd_shutdown_body"; then
    echo 'FAIL: CD shutdown does not request stop and join its worker' >&2
    exit 1
fi

join_line="$(grep -n 'PsyX_CD_Shutdown();' <<<"$shutdown_body" | cut -d: -f1)"
sdl_line="$(grep -n 'SDL_Quit();' <<<"$shutdown_body" | cut -d: -f1)"
if [ -z "$join_line" ] || [ -z "$sdl_line" ] ||
   [ "$join_line" -ge "$sdl_line" ]; then
    echo 'FAIL: CD worker is not joined before SDL teardown' >&2
    exit 1
fi

renderer_line="$(grep -n 'GR_Shutdown();' <<<"$shutdown_body" | cut -d: -f1)"
window_line="$(grep -n 'SDL_DestroyWindow(g_window);' <<<"$shutdown_body" | cut -d: -f1)"
if [ -z "$renderer_line" ] || [ -z "$window_line" ] ||
   [ "$renderer_line" -ge "$window_line" ]; then
    echo 'FAIL: OpenGL renderer is destroyed after its SDL window/context' >&2
    exit 1
fi

grep -Fq '_xeno_cd_shutdown_join' "$PATCH"
grep -Fq 'PsyX_CD_Shutdown();' "$PATCH"
grep -Fq 'GR_Shutdown();' "$PATCH"

echo 'PsyCross CD shutdown ordering: PASS'
