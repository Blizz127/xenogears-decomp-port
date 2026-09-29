#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t field-object-cleanup.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx)
BASE=(-std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C -fno-pie -no-pie \
      -ffunction-sections -fdata-sections -Wl,--gc-sections \
      -include assert.h -Wall -Wextra -Wno-incompatible-pointer-types)

for opt in O0 O2; do
    gcc "${BASE[@]}" "-${opt}" "${INC[@]}" \
        pc_port/tests/field_object_cleanup_adapter_test.c -o "$OUT/test-$opt"
    "$OUT/test-$opt" >"$OUT/$opt.log" 2>&1
done
cmp "$OUT/O0.log" "$OUT/O2.log"

clang "${BASE[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all \
    "${INC[@]}" pc_port/tests/field_object_cleanup_adapter_test.c \
    -o "$OUT/test-ubsan"
"$OUT/test-ubsan" >"$OUT/ubsan.log" 2>&1
cmp "$OUT/O0.log" "$OUT/ubsan.log"

for mutant in \
    XENO_TEST_MUTATE_FIELD_OBJECT_SKIP_MODEL_CLEANUP \
    XENO_TEST_MUTATE_FIELD_OBJECT_SKIP_SLOT_CLEAR \
    XENO_TEST_MUTATE_FIELD_OBJECT_SKIP_DESTROY_ALL_SLOT; do
    gcc "${BASE[@]}" -O2 -D"$mutant" "${INC[@]}" \
        pc_port/tests/field_object_cleanup_adapter_test.c \
        -o "$OUT/mutant-$mutant"
    if "$OUT/mutant-$mutant" >"$OUT/mutant-$mutant.log" 2>&1; then
        echo "FIELD_OBJECT_CLEANUP_ADAPTER mutant survived: $mutant" >&2
        exit 1
    fi
done

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path

disc_path = Path("disc/disc1.bin")
h = sha256()
with disc_path.open("rb") as stream:
    for chunk in iter(lambda: stream.read(1024 * 1024), b""):
        h.update(chunk)
assert h.hexdigest() == "39c547a9afc6da15d847ef81a2c6cea1a6516bdfa562cf13b0999b04e8598bda"

# ArchiveSetIndex(4, 0) makes local 0x6B9 global entry 0x860. Its retail
# index record is sector 231361, size 0xC6B4; CD-ROM mode-2 user data starts
# 24 bytes into each 2352-byte raw sector.
payload = bytearray()
with disc_path.open("rb") as stream:
    for sector in range(231361, 231361 + 25):
        stream.seek(sector * 2352 + 24)
        payload.extend(stream.read(2048))
assert sha256(payload).hexdigest() == "14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523"

load_base = 0x801DC000
start = 0x801E8030 - load_base
end = 0x801E8330 - load_base
assert sha256(payload[start:end]).hexdigest() == "e15a7ea45c5adf850542501ced1817d2f6bda71e2b355251dd2381cb3c3d95c7"
start = 0x801E7FD4 - load_base
end = 0x801E8030 - load_base
assert sha256(payload[start:end]).hexdigest() == "0fc1564d18b625500a97b6545bd868c46d1dd8aa93937d392b5e60fbbc2d1218"
print("FIELD_OBJECT_CLEANUP_ADAPTER RETAIL_SLICE PASS")
PY

cat "$OUT/O0.log"
echo "FIELD_OBJECT_CLEANUP_ADAPTER MUTANTS PASS"
