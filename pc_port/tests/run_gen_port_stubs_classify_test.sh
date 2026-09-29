#!/usr/bin/env bash
# gen_port_stubs classification — regression test for the firstfile/nextfile bug.
#
# Retail has firstfile (0x80040584) and nextfile (0x80040594) as .text FUNCTIONS,
# but overlay ELFs reference them as absolute (ABS) symbols, so the generator's
# ELF/map classification alone emitted them as zeroed data arrays:
#     unsigned char firstfile[32];
# src/menu/main/misc.c calls them as functions, so the call bound into a zero
# .bss array. The fix honours the splat `type:func` annotation for such names.
#
# The test runs the generator on small self-contained map/symbol_addrs inputs
# (no build/out artifacts) and checks:
#   * type:func names are emitted as function stubs, not data;
#   * a non-func type annotation (type:u8) leaves a symbol as data;
#   * three mutants are rejected:
#       noannotation - drop type:func from the input symbol_addrs
#       nogeneratorfix - drop the type override from the generator itself
#       anytype      - treat any type: annotation as a function
set -euo pipefail
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/gen_stubs_classify.XXXXXXXX")
echo "gen_port_stubs classification artifacts: $out"
gen="$out/gen"
mkdir -p "$gen"

cat > "$gen/map.txt" <<'EOF'
 .text          0x80010000       0x1000
                0x80019524       func_a
 .data          0x80050000       0x100
                0x80040584       firstfile
                0x80040594       nextfile
                0x80050000       data_a
EOF
cat > "$gen/undef.txt" <<'EOF'
firstfile
nextfile
func_a
data_a
EOF
cat > "$gen/sym_addrs.txt" <<'EOF'
firstfile = 0x80040584; // type:func
nextfile = 0x80040594; // type:func
data_a = 0x80050000; // type:u8
EOF
# Mutant input 1: no type:func annotation.
grep -v 'type:func' "$gen/sym_addrs.txt" > "$gen/sym_addrs_noannotation.txt"

# Mutant generators 2 and 3 (textual mutations of the real generator).
python3 - "$out" <<'PY'
import sys
from pathlib import Path
out = Path(sys.argv[1])
src = Path('tools/scripts/gen_port_stubs.py').read_text()
old = 'if info[0] or n.startswith("func_") or sym_types.get(n) == "func":  # is_func'
assert old in src, 'generator type-override line not found'
Path(out / 'gen_nogeneratorfix.py').write_text(
    src.replace(old, 'if info[0] or n.startswith("func_"):  # is_func'))
Path(out / 'gen_anytype.py').write_text(
    src.replace(old, 'if info[0] or n.startswith("func_") or n in sym_types:  # is_func'))
PY

classify() {
    # $1 = generator script, $2 = symbol_addrs, $3 = output c
    python3 "$1" --map "$gen/map.txt" --symbol-addrs "$2" \
        --undefined "$gen/undef.txt" --out "$3" 2>"$3.log"
}

check_classes() {
    # $1 = output c, $2 = expected funcs, $3 = expected data, $4 = label
    python3 - "$1" "$2" "$3" "$4" <<'PY'
import re
import sys

def classes(path):
    data, funcs = set(), set()
    for line in open(path):
        m = re.match(r'unsigned char (\w+)\[', line)
        if m:
            data.add(m.group(1))
        m = re.match(r'long (\w+)\(void\)', line)
        if m:
            funcs.add(m.group(1))
    return funcs, data

got_f, got_d = classes(sys.argv[1])
want_f = set(sys.argv[2].split())
want_d = set(sys.argv[3].split())
label = sys.argv[4]
if got_f != want_f or got_d != want_d:
    print(f'FAIL {label}: funcs={sorted(got_f)} data={sorted(got_d)} '
          f'(want funcs={sorted(want_f)} data={sorted(want_d)})', file=sys.stderr)
    sys.exit(1)
print(f'OK {label}: funcs={sorted(got_f)} data={sorted(got_d)}')
PY
}

echo "== production classification =="
classify tools/scripts/gen_port_stubs.py "$gen/sym_addrs.txt" "$gen/out.c"
grep -q '^long firstfile(void)' "$gen/out.c"
grep -q '^long nextfile(void)' "$gen/out.c"
! grep -q '^unsigned char firstfile\[' "$gen/out.c"
check_classes "$gen/out.c" "firstfile nextfile func_a" "data_a" production

echo "== mutants (each must produce a different, wrong classification) =="
classify tools/scripts/gen_port_stubs.py "$gen/sym_addrs_noannotation.txt" "$gen/m1.c"
if check_classes "$gen/m1.c" "func_a" "firstfile nextfile data_a" noannotation; then
    echo "negative control detected: noannotation (firstfile/nextfile fell back to data)"
else
    echo "NEGATIVE CONTROL noannotation produced an unexpected classification" >&2
    exit 1
fi

classify "$out/gen_nogeneratorfix.py" "$gen/sym_addrs.txt" "$gen/m2.c"
if check_classes "$gen/m2.c" "func_a" "firstfile nextfile data_a" nogeneratorfix; then
    echo "negative control detected: nogeneratorfix (generator ignored type:func)"
else
    echo "NEGATIVE CONTROL nogeneratorfix produced an unexpected classification" >&2
    exit 1
fi

classify "$out/gen_anytype.py" "$gen/sym_addrs.txt" "$gen/m3.c"
if check_classes "$gen/m3.c" "firstfile nextfile func_a data_a" "" anytype; then
    echo "negative control detected: anytype (type:u8 wrongly promoted to a function)"
else
    echo "NEGATIVE CONTROL anytype produced an unexpected classification" >&2
    exit 1
fi

echo "gen_port_stubs classification PASS (3/3 negative controls rejected)"
