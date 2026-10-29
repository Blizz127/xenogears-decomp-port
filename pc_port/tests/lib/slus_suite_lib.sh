# shellcheck shell=bash
# slus_suite NAME WHAT TESTC SRC "weak syms" "needs_compat(0/1)" <MUTS...>
# Generic O0/O2/UBSan + per-body mutant runner for main-exe retail differentials.
# MUT entry: "fn|find|repl".  Env NAME_MODES (e.g. SLTEMP2B_MODES=O2) restricts regimes (no mutants then).
# slus_mutate override: the shared oracle lib matches braced C definitions;
# this fallback additionally accepts a one-line macro body MACRO(fn, ...)
# (e.g. GPU_LINK_FUNC OT-link one-liners). The macro line must be unique.
slus_mutate() {
  python3 - "$@" <<'PY'
import re, sys
src, fn, find, repl, dst = sys.argv[1:6]
t = open(src).read()
m = None
for m in re.finditer(r'^[A-Za-z_][^;{}\n]*\b%s\([^;{]*\)\s*\{' % re.escape(fn), t, re.M):
    s = m.start(); e = t.index('\n}\n', s) + 2
    if find in t[s:e]:
        break
else:
    ml = [x for x in re.finditer(r'^[ \t]*[A-Za-z_][A-Za-z_0-9]*\(\s*%s\s*,' % re.escape(fn), t, re.M)]
    if len(ml) != 1:
        sys.exit("no body with target for " + fn)
    s = ml[0].start(); e = t.index('\n', s) + 1
    if find not in t[s:e]:
        sys.exit("no body with target for " + fn)
span = t[s:e]
if span.count(find) != 1:
    sys.exit("mutation target hit %d times in %s" % (span.count(find), fn))
open(dst, 'w').write(t[:s] + span.replace(find, repl, 1) + t[e:])
PY
}
slus_suite() {
  local name=$1 what=$2 testc=$3 src=$4 weak=$5 compat=$6; shift 6
  local up; up=$(echo "$name" | tr a-z A-Z)
  local OUT=pc_port/build_native/main_exe_$name
  local modes_var=MAIN_EXE_${up}_MODES
  local modes=${!modes_var:-O0 O2 UBSan}
  local WEAKA=($weak) mode rc
  mkdir -p "$OUT"
  for mode in $modes; do
    slus_build_deps "$mode"; [ "$compat" = 1 ] && slus_build_compat "$mode"
    slus_build_tu "$mode" "$src" "$OUT/$mode.uut.o" "${WEAKA[@]}"
    slus_build_test "$mode" "$testc" "$OUT/$mode.test.o"
    slus_link "$mode" "$OUT/$mode" "$OUT/$mode.test.o" "$OUT/$mode.uut.o"
    rc=0
    "$OUT/$mode" > "$OUT/$mode.log" 2> "$OUT/$mode.err" || rc=$?
    cat "$OUT/$mode.log"
    if [ "$rc" != 0 ]; then echo "$what regime $mode rc=$rc"; head -5 "$OUT/$mode.err" >&2; return 1; fi
  done
  if [ -z "${!modes_var:-}" ]; then
    cmp "$OUT/O0.log" "$OUT/O2.log"; cmp "$OUT/O0.log" "$OUT/UBSan.log"
    echo "$what O0/O2/UBSan logs identical"
  else return 0; fi
  local n=0 m fn rest find repl r
  for m in "$@"; do
    fn=${m%%|*}; rest=${m#*|}; find=${rest%%|*}; repl=${rest#*|}
    slus_mutate "$src" "$fn" "$find" "$repl" "$OUT/$fn.mut.c" || { echo "MUTANT SITE MISSING $fn" >&2; return 1; }
    slus_build_tu O2 "$OUT/$fn.mut.c" "$OUT/$fn.mut.o" "${WEAKA[@]}"
    slus_link O2 "$OUT/$fn.mut" "$OUT/O2.test.o" "$OUT/$fn.mut.o"
    r=0; "$OUT/$fn.mut" "$fn" > "$OUT/$fn.mut.log" 2>/dev/null || r=$?
    if [ "$r" != 1 ] || ! grep -q "DIVERGE fn=$fn" "$OUT/$fn.mut.log"; then
      cat "$OUT/$fn.mut.log"; echo "$what control failed to detect defect: $fn rc=$r" >&2; return 1
    fi
    echo "$what CONTROL REJECTED $fn ($(grep -m1 DIVERGE "$OUT/$fn.mut.log" | cut -c1-100))"
    n=$((n+1))
  done
  echo "$what ALL PASS (differential O0/O2/UBSan + $n negative controls)"
}
