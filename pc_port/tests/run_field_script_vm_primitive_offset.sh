#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t field-script-vm-primitive.XXXXXX)"
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
        -c src/field/main/misc.c -o "$OUT/misc-$label.o"
    gcc "${BASE[@]}" "${extra[@]}" "${INC[@]}" \
        pc_port/tests/field_script_vm_primitive_offset_test.c "$OUT/misc-$label.o" \
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

if build_and_run mutant_copy -O0 -DFIELD_VM_AUDIT_MUTANT_PRIM_SKIP_ALT_COPY; then
    echo "FIELD_SCRIPT_VM_PRIMITIVE_OFFSET FAIL copy mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_PRIMITIVE_OFFSET FAIL ' "$OUT/mutant_copy.log"

if build_and_run mutant_stride -O0 -DFIELD_VM_AUDIT_MUTANT_PRIM_TRIANGLE_STRIDE_28; then
    echo "FIELD_SCRIPT_VM_PRIMITIVE_OFFSET FAIL stride mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_PRIMITIVE_OFFSET FAIL ' "$OUT/mutant_stride.log"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path

payload = Path("disc/field.bin").read_bytes()
base = 0x8006FAF0
actual = sha256(payload[0x8008B5D4 - base:0x8008B894 - base]).hexdigest()
expected = "d2e72e1f311a1dda614bc10b3f9ab227193a9beb11feea442a4f0f1ddb3b4c80"
assert actual == expected, (actual, expected)
print("FIELD_SCRIPT_VM_PRIMITIVE_OFFSET RETAIL_SLICE PASS")
PY

cat "$OUT/O0.log"
echo "FIELD_SCRIPT_VM_PRIMITIVE_OFFSET MUTANTS PASS"
