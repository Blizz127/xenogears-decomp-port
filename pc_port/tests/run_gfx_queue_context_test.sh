#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
python3 - "$out/override.c" <<'PY'
from pathlib import Path
import sys
source = Path('pc_port/src/game_overrides.c').read_text()
start = source.index('void func_80025180(void* pData)\n{')
end = source.index('\n}', start) + 2
prefix = '''#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
extern u32 g_GfxCurWorkBuffer;
extern s32 g_GfxCurContext;
extern u32 D_80059300[];
extern u32 D_80059304;
'''
Path(sys.argv[1]).write_text(prefix + source[start:end] + '\n')
PY
gcc -std=c11 -Wall -Wextra -Werror -fno-pie -no-pie \
    "$out/override.c" pc_port/tests/gfx_queue_context_test.c -o "$out/test"
timeout -k 2s 10s "$out/test"
