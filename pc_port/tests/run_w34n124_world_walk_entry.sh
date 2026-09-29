#!/usr/bin/env bash
# W34N124: natural on-foot world walk and Lahan Village entry.
#
# Runs the accepted Lahan exit-1 route into the world session, walks Fei with
# the d-pad nibble 0x2000 for 30 world frames onto the Lahan Village trigger
# (list-0 record 1: map 1, entrance 9), then presses circle (0x0020) once.
# Verifies from the log that:
#   1. slot 1 (wm_8008A72C) moved the guest position between call 1 and 31;
#   2. the trigger query selected record id 1 while standing on it;
#   3. the world session exited naturally (D554 -> 0, D7CC == 0);
#   4. the terminal lane handed FieldMain map 1.
# Requires a host display (default :10) and the normal xeno-port build.
#
# Usage: run_w34n124_world_walk_entry.sh [lahan|mountain|blackmoon]
#   lahan    (default) 30 frames of 0x2000, circle at frame 40:
#            record 1 (id 1) -> map 1 entrance 9.
#   mountain 120 frames of 0x2000 (Fei stops against the mountain wall inside
#            record 0), circle at frame 130: record 0 (id 2) -> map 15
#            entrance 2 (the Mountain Path world entrance).
#   blackmoon takes that same world route and checks whether the field scripts
#            continue from map 15 into map 16 (Blackmoon Forest).
set -uo pipefail

target=${1:-lahan}
case "$target" in
  lahan)
    schedule='0:0x2000,30:0,40:0x20,41:0'
    sample_call=31
    expect_id=1
    expect_map=1
    ;;
  mountain)
    schedule='0:0x2000,120:0,130:0x20,131:0'
    sample_call=121
    expect_id=2
    expect_map=15
    ;;
  blackmoon)
    schedule='0:0x2000,120:0,130:0x20,131:0'
    sample_call=121
    expect_id=2
    expect_map=15
    ;;
  *)
    echo "unknown target: $target" >&2
    exit 2
    ;;
esac

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
log=${W34N124_LOG:-$(mktemp /tmp/w34n124-$target.XXXXXX.log)}
display=${DISPLAY:-:10}

cd "$repo_dir"
XENO_WORLD_TEST_INPUT=$schedule \
DISPLAY=$display timeout -s INT "${W34N124_TIMEOUT:-420}" gdb -batch \
  -x pc_port/tools/world_harness/w34n124_walk_entry.gdb \
  --args pc_port/build_native/xeno-port > "$log" 2>&1
echo "W34N124 log=$log"

fail=0
check() {
  if "$@"; then
    printf 'W34N124 PASS %s\n' "$name"
  else
    printf 'W34N124 FAIL %s\n' "$name"
    fail=1
  fi
}

name="slot-1 call 1 recorded"
check grep -aq 'W34N124 call=1 slot=1 ' "$log"
pos1=$(grep -a 'W34N124 call=1 slot=1 ' "$log" | sed -n 's/.*pos=(\([^)]*\)).*/\1/p' | head -1)
pos31=$(grep -a "W34N124 call=$sample_call slot=1 " "$log" | sed -n 's/.*pos=(\([^)]*\)).*/\1/p' | head -1)
name="guest position moved between call 1 and $sample_call ($pos1 -> $pos31)"
check test -n "$pos1" -a -n "$pos31" -a "$pos1" != "$pos31"
anim2=$(grep -a 'W34N124 call=2 slot=1 ' "$log" | sed -n 's/.* anim=\([-0-9]*\) .*/\1/p' | head -1)
pose2=$(grep -a 'W34N124 call=2 slot=1 ' "$log" | sed -n 's/.* pose=\([-0-9]*\) .*/\1/p' | head -1)
anim_sample=$(grep -a "W34N124 call=$sample_call slot=1 " "$log" | sed -n 's/.* anim=\([-0-9]*\) .*/\1/p' | head -1)
pose_sample=$(grep -a "W34N124 call=$sample_call slot=1 " "$log" | sed -n 's/.* pose=\([-0-9]*\) .*/\1/p' | head -1)
name="moving player selects walk animation 1 ($anim2, $anim_sample)"
check test "$anim2" = 1 -a "$anim_sample" = 1
name="moving player advances rendered pose ($pose2 -> $pose_sample)"
check test -n "$pose2" -a -n "$pose_sample" -a "$pose2" != "$pose_sample"
name="target trigger (record id $expect_id) selected at call $sample_call"
check grep -aq "W34N124 call=$sample_call slot=1 .*BD24=$expect_id " "$log"
name="natural world exit with D7CC=0"
check grep -aq '\[worldmap-open-loop\] natural state exit frames=[0-9]* D7CC=0' "$log"
name="terminal lane entered FieldMain map $expect_map"
check grep -aq "\[FieldMain\] g_pGameState=0x[0-9a-f]* g_GameSceneMapNum=$expect_map\$" "$log"
if [ "$target" = blackmoon ]; then
    name="Blackmoon Forest map 16 reached from Mountain Path"
    check grep -aq '\[FieldMain\] g_pGameState=0x[0-9a-f]* g_GameSceneMapNum=16$' "$log"
    if ! grep -aq '\[FieldMain\] g_pGameState=0x[0-9a-f]* g_GameSceneMapNum=16$' "$log"; then
        last_map=$(grep -a '\[FieldMain\] g_pGameState=' "$log" | tail -1 | sed 's/.*g_GameSceneMapNum=//')
        echo "W34N124 INFO blackmoon.stop_map=${last_map:-unobserved}"
    fi
fi
name="no worldmap stub reached"
check test "$(grep -ac '\[worldmap-stub\]' "$log")" = 0

if [ "$fail" = 0 ]; then
  echo "W34N124 WORLD WALK ENTRY PASS target=$target"
else
  echo "W34N124 WORLD WALK ENTRY FAIL target=$target" >&2
  exit 1
fi
