#!/bin/bash
# m2c_drive.sh OV VRAM SEG...
#
# Draft every INCLUDE_ASM function of the given segments with m2c_try.py,
# insert the standalone matches with m2c_insert.py, `make build`, and keep
# iterating until build/out/OV.bin is identical to disc/OV.bin: functions
# that fail to compile, or that elf_cmp.py shows mismatching in their TU
# (callee/extern types differ from the standalone draft), go to a skip list
# ($XENO_ASSIST_CACHE/skip_OV.txt) and stay INCLUDE_ASM.  src/OV is restored
# from a snapshot before each attempt, so only the final identical state
# remains in the tree.  Run `make rom-check` before committing.
set -u
OV=$1; VRAM=$2; shift 2; SEGS="$*"
HERE=$(cd "$(dirname "$0")" && pwd); REPO=$(cd "$HERE/../../.." && pwd); cd "$REPO"
CACHE=${XENO_ASSIST_CACHE:-$HOME/.cache/xeno/decomp_assist}; mkdir -p "$CACHE"
SK=$CACHE/skip_$OV.txt; touch "$SK"
build() { if [ -n "${XENO_BUILD_WRAP:-}" ]; then $XENO_BUILD_WRAP "$1"; else bash -lc "$1"; fi; }
[ -n "${SKIP_TRY:-}" ] || for s in $SEGS; do nice -n 10 python3 "$HERE/m2c_try.py" "$OV" "$s" >/dev/null 2>&1; done
SNAP=$CACHE/snap_$OV; rm -rf "$SNAP"; cp -r "src/$OV" "$SNAP"
for it in 1 2 3 4 5 6 7 8; do
  rm -rf "src/$OV"; cp -r "$SNAP" "src/$OV"
  for s in $SEGS; do
    names=$(python3 -c "
import json,sys
r=json.load(open('$CACHE/m2c/$OV/$s/results.json')); sk=set(open('$SK').read().split())
print(' '.join(k for k,v in r.items() if v=='MATCH' and k not in sk))")
    [ -n "$names" ] && python3 "$HERE/m2c_insert.py" "$OV" "$s" $names
  done
  build 'nice -n 10 make build' > "$CACHE/drive.log" 2>&1
  errs=$(grep "In function" "$CACHE/drive.log" | grep -oE "\`\w+'" | tr -d "\`'" | sort -u)
  if [ -n "$errs" ]; then echo "iter $it: compile errors in" $errs; echo "$errs" >> "$SK"; continue; fi
  if [ ! -f "build/out/$OV.bin" ]; then echo "iter $it: build failed outside a function:"; grep -E "\.i:[0-9]+:|undefined reference|multiple definition" "$CACHE/drive.log" | head -5; break; fi
  if cmp -s "build/out/$OV.bin" "disc/$OV.bin"; then echo "iter $it: IDENTICAL"; exit 0; fi
  bad=$(python3 "$HERE/elf_cmp.py" "$OV" "$VRAM" | awk '{print $1}')
  [ -z "$bad" ] && { echo "iter $it: binary differs but no function flagged (relocation target?)"; break; }
  echo "iter $it: mismatch in" $bad; echo "$bad" >> "$SK"
done
echo "gave up; restoring snapshot"; rm -rf "src/$OV"; cp -r "$SNAP" "src/$OV"; exit 1
