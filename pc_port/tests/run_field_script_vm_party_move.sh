#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t field-script-vm-party.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx)
BASE=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY \
      -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
      -ffunction-sections -fdata-sections -fno-pie -include assert.h -w)

build_and_run() {
    local label="$1"
    shift
    local extra=("$@")
    gcc "${BASE[@]}" "${extra[@]}" "${INC[@]}" \
        -c src/field/main/misc7.c -o "$OUT/misc7-$label.o"
    gcc "${BASE[@]}" "${extra[@]}" "${INC[@]}" \
        pc_port/tests/field_script_vm_party_move_test.c "$OUT/misc7-$label.o" \
        -Wl,--gc-sections -no-pie -o "$OUT/test-$label"
    "$OUT/test-$label" >"$OUT/$label.log" 2>&1
}

build_and_run O0 -O0
build_and_run O2 -O2

UBSAN_ARCHIVE="/home/linuxbrew/.linuxbrew/Cellar/llvm/22.1.8/lib/clang/22/lib/linux/libclang_rt.ubsan_standalone-x86_64.a"
if [[ -f "$UBSAN_ARCHIVE" ]]; then
    ln -s "$UBSAN_ARCHIVE" "$OUT/libubsan.a"
    build_and_run UBSAN -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        -L"$OUT" -static-libubsan
else
    build_and_run UBSAN -O1 -fsanitize=undefined -fno-sanitize-recover=all
fi
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSAN.log"

if build_and_run mutant_move -O0 -DFIELD_VM_AUDIT_MUTANT_PARTY_MOVE_ALWAYS_DONE; then
    echo "FIELD_SCRIPT_VM_PARTY_MOVE FAIL movement mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_PARTY_MOVE FAIL ' "$OUT/mutant_move.log"

if build_and_run mutant_ip -O0 -DFIELD_VM_AUDIT_MUTANT_PARTY_GROUP_IP_1; then
    echo "FIELD_SCRIPT_VM_PARTY_MOVE FAIL group-IP mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_PARTY_MOVE FAIL ' "$OUT/mutant_ip.log"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path

payload = Path("disc/field.bin").read_bytes()
base = 0x8006FAF0
expected = {
    (0x8009AEE0, 0x8009B15C): "2dac5d12ff717ca279c923da11fd7989f7f61d2925777c1ae2c1c958ee98024f",
    (0x8009B210, 0x8009B338): "fd48db8a080481b68e09dcb521a8b1e41d8a128083e4fd9ca2e1824e6ca5166b",
    (0x8009B398, 0x8009B664): "a3306818a1faa2ed818028e51fc8aaeed476a16bf50666f40b2b3dfd92101686",
}
for (start, end), digest in expected.items():
    actual = sha256(payload[start - base:end - base]).hexdigest()
    assert actual == digest, (hex(start), actual, digest)
print("FIELD_SCRIPT_VM_PARTY_MOVE RETAIL_SLICES PASS")
PY

cat "$OUT/O0.log"
echo "FIELD_SCRIPT_VM_PARTY_MOVE MUTANTS PASS"
