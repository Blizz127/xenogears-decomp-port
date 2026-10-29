# shellcheck shell=bash
# Shared build helpers for the main-executable (slus_006.64) retail-differential
# tests built on slus_oracle.h.  Callers: cd to repo root, set OUT, then use
#   slus_build_deps <mode>            GTE/compat/adapter objects for a regime
#   slus_build_tu  <mode> <src> <name> [weaken syms...]
#   slus_link      <mode> <exe> <objs...>
# Regimes: O0, O2, UBSan (-O1, shift-base ENABLED: port arms must be UB-free).
SLUS_SHA=dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119
slus_check_disc() {
  local sha
  sha=$(sha256sum disc/SLUS_006.64 | awk '{print $1}')
  [ "$sha" = "$SLUS_SHA" ] || { echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $sha" >&2; exit 1; }
}
SLUS_COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
  -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
SLUS_GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
  -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/tests -Ipc_port/build_native)
slus_flags() { # mode -> SLUS_FL
  if [ "$1" = UBSan ]; then SLUS_FL=(-O1 -g -fsanitize=undefined -fno-sanitize-recover=all); else SLUS_FL=(-"$1"); fi
}
slus_build_deps() {
  local mode=$1 s name
  slus_flags "$mode"
  [ -f "$OUT/$mode.deps.done" ] && return 0
  for s in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
    name=${s##*/}
    g++ -std=c++17 "${SLUS_COMMON[@]}" "${SLUS_FL[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
      -c "pc_port/extern/PsyCross/src/$s" -o "$OUT/$mode.dep.$name.o"
  done
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -Wall -Wextra -Werror \
    -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.dep.adapter.o"
  touch "$OUT/$mode.deps.done"
}
slus_build_compat() { # mode : host libgte helpers (VectorNormalS, OuterProduct0, NormalLightCol)
  local mode=$1
  slus_flags "$mode"
  [ -f "$OUT/$mode.compat.done" ] && return 0
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -fpermissive -w \
    -c pc_port/src/psyq_compat.c -o "$OUT/$mode.dep.compat.o"
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -fpermissive -w -include common.h -include psyq/libgte.h \
    -c pc_port/src/psyq_compat_normal_light_col.c -o "$OUT/$mode.dep.normlight.o"
  touch "$OUT/$mode.compat.done"
}
slus_build_tu() { # mode src obj [weaken...]
  local mode=$1 src=$2 obj=$3; shift 3
  slus_flags "$mode"
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -fpermissive -w -fno-inline -fno-ipa-ra -fno-ipa-cp \
    -c "$src" -o "$obj"
  local s syms
  syms=$(nm "$obj") # capture first: `nm | grep -q` under pipefail can die of SIGPIPE and skip the weaken
  for s in "$@"; do
    if grep -q " T $s\$" <<<"$syms"; then objcopy --weaken-symbol="$s" "$obj"; fi
  done
  for s in $(cut -d'|' -f1 pc_port/port_owned_overrides.txt | grep -v '^#'); do
    if grep -q " T $s\$" <<<"$syms"; then objcopy --weaken-symbol="$s" "$obj"; fi
  done
}
slus_build_test() { # mode test.c obj
  local mode=$1 src=$2 obj=$3
  slus_flags "$mode"
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -w -c "$src" -o "$obj"
}
slus_link() { # mode exe objs...
  local mode=$1 exe=$2; shift 2
  slus_flags "$mode"
  clang++ -no-pie "${SLUS_FL[@]}" -Wl,--gc-sections "$@" "$OUT/$mode.dep."*.o -ldl -lm -o "$exe"
}
# mutate_span file fn find repl out : apply a one-site edit inside fn's body
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
    sys.exit("no body with target for " + fn)
span = t[s:e]
if span.count(find) != 1:
    sys.exit("mutation target hit %d times in %s" % (span.count(find), fn))
open(dst, 'w').write(t[:s] + span.replace(find, repl, 1) + t[e:])
PY
}
