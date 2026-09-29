#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${ARCHIVE_BUFFER_OUT:-$(mktemp -d /tmp/xeno-archive-buffer.XXXXXXXX)}
SOURCE=${ARCHIVE_BUFFER_SOURCE:-src/slus_006.64/system/libarchive.c}
mkdir -p "$OUT"
echo "ARCHIVE BUFFER OUTPUT $OUT"
COMMON=(-std=gnu17 -fno-pie -fno-builtin -fno-inline -fno-ipa-ra -DXENO_PC_PORT -DSKIP_ASM
  -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
  -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude
  -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
WEAKEN=()
for name in ArchiveDecodeSize ArchiveCdDataSync ArchiveDecodeSector ArchiveDecodeAlignedSize ArchiveReadFile; do
  WEAKEN+=(--weaken-symbol="$name")
done
for mode in O0 O2 UBSan; do
  FLAGS=(-"$mode")
  if [ "$mode" = UBSan ]; then FLAGS=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
  gcc "${COMMON[@]}" "${FLAGS[@]}" -fpermissive -w -c "$SOURCE" -o "$OUT/$mode.source.o"
  objcopy "${WEAKEN[@]}" "$OUT/$mode.source.o"
  gcc "${COMMON[@]}" "${FLAGS[@]}" -Wall -Wextra -Werror -c pc_port/tests/archive_buffer_pointer_test.c -o "$OUT/$mode.test.o"
  clang -no-pie "${FLAGS[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" -o "$OUT/$mode"
  "$OUT/$mode" | tee "$OUT/$mode.log"
done
# Retain the actual production body and recreate only the old pointer type.
python3 - "$SOURCE" "$OUT/old-pointer.c" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
signature = 's32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags)'
assert source.count(signature) == 1
Path(sys.argv[2]).write_text(source.replace(signature, signature.replace('void* pBuffer', 's32 pBuffer'), 1))
PY
gcc "${COMMON[@]}" -O2 -fpermissive -w -c "$OUT/old-pointer.c" -o "$OUT/old-pointer.o"
objcopy "${WEAKEN[@]}" "$OUT/old-pointer.o"
clang -no-pie -Wl,--gc-sections "$OUT/old-pointer.o" "$OUT/O2.test.o" -o "$OUT/old-pointer"
rc=0
"$OUT/old-pointer" > "$OUT/old-pointer.log" 2>&1 || rc=$?
if [ "$rc" = 0 ] || ! rg -q 'native destination pointer truncated' "$OUT/old-pointer.log"; then
  cat "$OUT/old-pointer.log" >&2
  echo 'ARCHIVE BUFFER old pointer negative control failed' >&2
  exit 1
fi
echo 'ARCHIVE BUFFER old pointer negative control rejected'
