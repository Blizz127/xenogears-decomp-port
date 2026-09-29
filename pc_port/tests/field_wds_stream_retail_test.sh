#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/field/main/misc8.c"

# func_80085560 forwards its allocation mode to the retail eight-slot stream
# allocation.  The one-argument host transcription left the allocator's
# second ABI argument undefined.
grep -Fq 'ArchiveAllocStreamFile(8, a1)' "$src"

# The streamed slot pointer, not the WDS chunk counter, owns the section-table
# entry released by func_8002945C.
count="$(grep -Fc 'func_8002945C(pSrc);' "$src")"
test "$count" -eq 2

# Once the sector pump is live, the old whole-file host staging path would
# double-load the bank and bypass retail's chunk callback boundary.
if grep -Eq 'PcPort_PendingBankFile|SoundLoadWdsFileHostStaged' "$src"; then
    echo 'field WDS stream retail contract: FAIL (host staging remains)' >&2
    exit 1
fi

echo 'field WDS stream retail contract: PASS'
