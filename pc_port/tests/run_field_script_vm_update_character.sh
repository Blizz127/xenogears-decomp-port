#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t field-script-vm-update-character.XXXXXX)"
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
        -c src/field/main/misc6.c -o "$OUT/misc6-$label.o"
    gcc "${BASE[@]}" "${extra[@]}" "${INC[@]}" \
        pc_port/tests/field_script_vm_update_character_test.c \
        "$OUT/misc6-$label.o" -Wl,--gc-sections -no-pie -o "$OUT/test-$label"
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

if build_and_run mutant_material -O0 \
    -DFIELD_VM_AUDIT_MUTANT_UPDATE_CHARACTER_ALLOW_MATERIAL; then
    echo "FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL material mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL ' "$OUT/mutant_material.log"

if build_and_run mutant_result -O0 \
    -DFIELD_VM_AUDIT_MUTANT_UPDATE_CHARACTER_INVERT_RESULT; then
    echo "FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL result mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL ' "$OUT/mutant_result.log"

if build_and_run mutant_cooldown -O0 \
    -DFIELD_VM_AUDIT_MUTANT_UPDATE_CHARACTER_SKIP_COOLDOWN; then
    echo "FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL cooldown mutant survived" >&2
    exit 1
fi
grep -q '^FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL ' "$OUT/mutant_cooldown.log"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path

payload = Path("disc/field.bin").read_bytes()
base = 0x8006FAF0
start, end = 0x8009F5F4, 0x8009FA00
expected = "e39b751f5a51295d92ab7af43b8e34e4d93509e144a46694e27669ca4c6565b8"
actual = sha256(payload[start - base:end - base]).hexdigest()
assert actual == expected, (actual, expected)
print("FIELD_SCRIPT_VM_UPDATE_CHARACTER RETAIL_SLICE PASS")
PY

cat "$OUT/O0.log"
echo "FIELD_SCRIPT_VM_UPDATE_CHARACTER MUTANTS PASS"
