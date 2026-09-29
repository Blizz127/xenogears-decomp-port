#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t field-script-vm-core.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx)
BASE=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM \
      -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections \
      -fdata-sections -fno-pie -include assert.h -w)

build_and_run() {
    local label="$1"
    shift
    local extra=("$@")
    gcc "${BASE[@]}" "${extra[@]}" "${INC[@]}" -c src/field/main/misc.c \
        -o "$OUT/misc-$label.o"
    gcc "${BASE[@]}" "${extra[@]}" \
        -DFieldGetVec2Magnitude=FieldGetVec2Magnitude_misc7_unused \
        "${INC[@]}" -c src/field/main/misc7.c \
        -o "$OUT/misc7-$label.o"
    gcc "${BASE[@]}" "${extra[@]}" "${INC[@]}" \
        pc_port/tests/field_script_vm_core_handlers_test.c \
        "$OUT/misc-$label.o" "$OUT/misc7-$label.o" \
        -Wl,--gc-sections -no-pie -o "$OUT/test-$label"
    "$OUT/test-$label" >"$OUT/$label.log" 2>&1
}

build_and_run O0 -O0
build_and_run O2 -O2
UBSAN_ARCHIVE="/home/linuxbrew/.linuxbrew/Cellar/llvm/22.1.8/lib/clang/22/lib/linux/libclang_rt.ubsan_standalone-x86_64.a"
if [[ -f "$UBSAN_ARCHIVE" ]]; then
    # Bazzite's immutable host currently has GCC's libubsan linker script but
    # not its target.  Clang's ABI-compatible standalone archive keeps this
    # focused test runnable there.  Ordinary dev containers take the standard
    # GCC runtime path below.
    ln -s "$UBSAN_ARCHIVE" "$OUT/libubsan.a"
    build_and_run UBSAN -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        -L"$OUT" -static-libubsan
else
    build_and_run UBSAN -O1 -fsanitize=undefined -fno-sanitize-recover=all
fi
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSAN.log"

if build_and_run mutant_leader -O0 -DFIELD_VM_AUDIT_MUTANT_LEADER_CURRENT_ACTOR; then
    echo "FIELD_SCRIPT_VM_CORE_HANDLERS FAIL leader mutant survived" >&2
    exit 1
fi
grep -q 'leader must use player actor character id' "$OUT/mutant_leader.log"

if build_and_run mutant_distance -O0 -DFIELD_VM_AUDIT_MUTANT_DISTANCE_ZERO; then
    echo "FIELD_SCRIPT_VM_CORE_HANDLERS FAIL distance mutant survived" >&2
    exit 1
fi
grep -q 'distance result' "$OUT/mutant_distance.log"

cat "$OUT/O0.log"
echo "FIELD_SCRIPT_VM_CORE_HANDLERS MUTANTS PASS"
