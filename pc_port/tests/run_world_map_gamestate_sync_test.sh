#!/usr/bin/env bash
# Test the tuple consumed by the installed arrival screen.
set -euo pipefail
cd "$(dirname "$0")/../.."
build=$(mktemp -d "${TMPDIR:-/tmp}/xeno-arrival-tuple.XXXXXXXX")
trap 'rm -rf "$build"' EXIT
ulimit -c 0
python3 - "$build/arrival.inc" <<'PY'
from pathlib import Path
import sys

source = Path("pc_port/src/game_overrides.c").read_text()
functions = []
for name in ("PcPort_WorldMapPlaceholderMain", "PcPort_InitGameStates"):
    signature = f"void {name}(void)\n{{"
    assert source.count(signature) == 1
    start = source.index(signature)
    end = source.index("\n}", start) + 2
    functions.append(source[start:end])
Path(sys.argv[1]).write_text("\n\n".join(functions) + "\n")
PY
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -I"$build" \
    pc_port/tests/world_map_gamestate_sync_test.c -o "$build/arrival"
timeout 20 "$build/arrival" 2>"$build/tuples.log"
python3 - "$build/tuples.log" <<'PY'
from pathlib import Path
import sys

lines = Path(sys.argv[1]).read_text().splitlines()
prefix = "[xeno-port][worldmap-placeholder] enter "
assert lines == [
    prefix + "selector=0x0403 world_index=3 entrance=0x0007 transition_arg2=0x002a heading=0x0009",
    prefix + "selector=0xc420 world_index=32 entrance=0x0123 transition_arg2=0xbeef heading=0x2345",
], lines
PY
echo "State 3 consumed both arrival tuples and submitted two card frames per input to mock presentation"
