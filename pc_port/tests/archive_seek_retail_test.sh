#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/pc_port/src/archive_port.c"
retail_src="$root/src/slus_006.64/system/archive.c"
stubs="$root/pc_port/build_native/stubs.c"

for fn in ArchiveCdSeekOrPause ArchiveCdSeekToFile; do
    grep -Fq "void $fn(int entryIndex)" "$retail_src"
    grep -Fq "void $fn(int entryIndex)" "$src"
    if grep -Eq "^(long|unsigned char) $fn(\[|\()" "$stubs"; then
        echo "$fn still uses generated fallback" >&2
        exit 1
    fi
done

grep -Fq 'ArchiveDecodeSector(entryIndex)' "$src"
grep -Fq 'CdIntToPos(sector, &g_ArchiveCdCurLocation)' "$src"
grep -Fq 'CdControlB(CdlSetloc' "$src"
grep -Fq 'CdControlB(CdlPause' "$src"
grep -Fq 'g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;' "$src"

echo 'archive seek retail adapter: PASS'
