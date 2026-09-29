#!/usr/bin/env bash
# The field overlay's ten 0x8028xxxx callees are the PC-HDD developer debug
# overlay (archive 4/0xAD at 0x80280000), unreachable in retail CD-ROM mode.
# Proves it from disc/field.bin + disc/SLUS_006.64 + disc/disc1.bin (with
# negative controls), then checks the port does not paper over the calls with
# generated no-op stubs. Pure static analysis: no compiler regimes to skip.
set -euo pipefail
cd "$(dirname "$0")/../.."

python3 pc_port/tests/field_debug_overlay_unreachable.py

STUBS=pc_port/build_native/stubs.c
if [ ! -f "$STUBS" ]; then
    echo "FAIL: $STUBS absent -- run pc_port/build_port.sh first (port check NOT RUN)"
    exit 1
fi
leaked=$(grep -oE '"func_8028[0-9A-F]{4}"' "$STUBS" | sort -u | tr '\n' ' ' || true)
if [ -n "$leaked" ]; then
    echo "FAIL: generated no-op stubs still stand in for debug-overlay entries: $leaked"
    exit 1
fi
defined=$(grep -cE '^void func_8028[0-9A-F]{4}\(.*FieldDebugOverlayUnreachable' pc_port/src/game_overrides.c)
if [ "$defined" -ne 10 ]; then
    echo "FAIL: expected 10 fail-closed debug-overlay entries in game_overrides.c, found $defined"
    exit 1
fi
echo "PASS port: 10 debug-overlay entries fail closed; none is a generated stub"
