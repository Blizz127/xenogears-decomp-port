#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t field-load-final-actor-init.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx)
BASE=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY \
      -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
      -ffunction-sections -fdata-sections -fno-pie -include assert.h -w)

compile_field_load() {
    local label="$1"
    shift
    gcc "${BASE[@]}" "$@" "${INC[@]}" \
        -c src/field/main/misc3.c -o "$OUT/misc3-$label.o"
    objdump -dr "$OUT/misc3-$label.o" >"$OUT/misc3-$label.dump"
    sed -n '/<FieldLoad>:/,/^$/p' "$OUT/misc3-$label.dump" \
        >"$OUT/FieldLoad-$label.dump"
}

require_call() {
    local dump="$1"
    local target="$2"
    if ! grep -Eq "R_[A-Za-z0-9_]+[[:space:]]+$target([+-]|$)" "$dump"; then
        echo "FIELD_LOAD_FINAL_ACTOR_INIT FAIL missing production call: $target" >&2
        return 1
    fi
}

reject_call() {
    local dump="$1"
    local target="$2"
    if grep -Eq "R_[A-Za-z0-9_]+[[:space:]]+$target([+-]|$)" "$dump"; then
        echo "FIELD_LOAD_FINAL_ACTOR_INIT FAIL omission mutant retained call: $target" >&2
        return 1
    fi
}

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from struct import unpack_from

payload = Path("disc/field.bin").read_bytes()
assert len(payload) == 260862
assert sha256(payload).hexdigest() == (
    "38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
)

load_base = 0x8006FAF0
start = 0x80070CC8 - load_base
end = 0x80071A64 - load_base
assert end - start == 0xD9C
assert sha256(payload[start:end]).hexdigest() == (
    "b07e933f5c019476dd472261b76e1200aae50eb4bae8627cad769a3f8f75ad2c"
)

def jal_target(address):
    word = unpack_from("<I", payload, address - load_base)[0]
    assert word >> 26 == 3
    return ((address + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)

assert jal_target(0x80071A04) == 0x800223B0
assert jal_target(0x80071A1C) == 0x80021FE0
print("FIELD_LOAD_FINAL_ACTOR_INIT RETAIL_SLICE PASS")
PY

for opt in O0 O2; do
    compile_field_load "$opt" "-$opt"
    require_call "$OUT/FieldLoad-$opt.dump" func_80021FE0
    require_call "$OUT/FieldLoad-$opt.dump" func_800223B0
done

compile_field_load mutant -O2 -DFIELD_AUDIT_MUTANT_SKIP_FINAL_ACTOR_ANIM
reject_call "$OUT/FieldLoad-mutant.dump" func_80021FE0
reject_call "$OUT/FieldLoad-mutant.dump" func_800223B0

echo "FIELD_LOAD_FINAL_ACTOR_INIT PASS production=O0,O2 omission_mutant=detected"
