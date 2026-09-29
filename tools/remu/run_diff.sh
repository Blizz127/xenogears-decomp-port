#!/usr/bin/env bash
# remu differential runner: builds the TU (O0/O2/UBSan, INCLUDE_ASM neutralized)
# and diffs its C bodies against retail bytes executed by remu.
# Usage: tools/remu/run_diff.sh <testname> <tu-src> <tu-stub-deps...>
#   testname: tools/remu/tests/<testname>.c (must define main + TU externs)
#   tu-src:   src/<overlay>/<tu>.c
# Example: tools/remu/run_diff.sh diff_800AF400 src/battle/main73.c
set -uo pipefail
cd "$(dirname "$0")/../.."
T="$1"; TU="$2"
SHIM="tools/remu/tests/remu_shim.h"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/xeno-remu-diff.XXXXXXXX")"
echo "REMU DIFF OUTPUT $OUT"
CC=${REMU_CC:-clang}
# Section-GC (like the repo's own test scripts): only the tested function and
# its deps survive the link; the rest of the TU needs no host stubs.
# -Wno-implicit-function-declaration: the retail compiler accepts implicit
# decls; clang must too (the later real declaration then governs).
# Optional 3rd+ args: extra -D flags a TU needs to compile on host.
# Tested bodies must not depend on those macros (verify by inspection).
# SKIP RULE: if the TU does not host-compile for PRE-EXISTING reasons
# unrelated to your function (64-bit layout _Static_asserts, implicit-decl
# conflicts elsewhere in the TU), do NOT hack the TU or headers: SKIP the
# function and report the TU as host-incompatible (it needs the matching
# build for verification).
COMMON=(-std=gnu17 -include "$SHIM" -Iinclude -Itools/remu -fno-pie -fno-builtin -DXENO_PC_PORT -Wno-everything -Wno-implicit-function-declaration -ffunction-sections -fdata-sections "${@:3}")
LINK=(-no-pie -Wl,--gc-sections)
pass=1
for mode in O0 O2 UBSan; do
  flags=(-"$mode")
  if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
  $CC "${COMMON[@]}" "${flags[@]}" -c "$TU" -o "$OUT/tu.$mode.o" || pass=0
  $CC "${COMMON[@]}" "${flags[@]}" -c "tools/remu/tests/$T.c" -o "$OUT/t.$mode.o" || pass=0
  $CC "${LINK[@]}" "${flags[@]}" "$OUT/tu.$mode.o" "$OUT/t.$mode.o" tools/remu/remu.c \
      -o "$OUT/$mode" || pass=0
  "$OUT/$mode" > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log"; pass=0; }
  cat "$OUT/$mode.log"
done
# Mutant control: per-test mutator script tools/remu/tests/mut_<T>.sh
# (takes TU MUTOUT); the test MUST reject the mutant. Absent mutator = warn.
MUT="$OUT/mutant.c"
if [ -f "tools/remu/tests/mut_$T.sh" ]; then
  if ! bash "tools/remu/tests/mut_$T.sh" "$TU" "$MUT"; then
    echo "MUTANT PREPARATION FAILED: mutator"
    echo "REMU DIFF $T FAILED"
    exit 1
  fi
else
  echo "WARNING: no mutator tools/remu/tests/mut_$T.sh; skipping mutant control"
  [ "$pass" = 1 ] && echo "REMU DIFF $T ALL GREEN (no mutant)" || { echo "REMU DIFF $T FAILED"; exit 1; }
  exit 0
fi
if ! $CC "${COMMON[@]}" -O2 -c "$MUT" -o "$OUT/mut.o"; then
  echo "MUTANT PREPARATION FAILED: compilation"
  pass=0
elif ! $CC "${LINK[@]}" -O2 "$OUT/mut.o" "$OUT/t.O2.o" tools/remu/remu.c -o "$OUT/mut"; then
  echo "MUTANT PREPARATION FAILED: linking"
  pass=0
else
  rc=0
  "$OUT/mut" > "$OUT/mut.log" 2>&1 || rc=$?
  if [ "$rc" = 0 ]; then
    echo "MUTANT NOT REJECTED"
    cat "$OUT/mut.log"
    pass=0
  else
    echo "MUTANT REJECTED (rc=$rc)"
  fi
fi
[ "$pass" = 1 ] && echo "REMU DIFF $T ALL GREEN" || { echo "REMU DIFF $T FAILED"; exit 1; }
