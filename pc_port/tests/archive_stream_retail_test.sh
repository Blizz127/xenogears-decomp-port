#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/pc_port/src/archive_port.c"

# Retail ArchiveReadFile arms a stream and returns while the CD/ring pump is
# outstanding.  The old host substitute rejected both retail stream modes;
# keep that regression from returning.
if grep -Fq 'Streaming reads (field BG/audio' "$src"; then
    echo 'archive stream retail structure: FAIL (stream modes still rejected)' >&2
    exit 1
fi
grep -Fq 'func_80028B14' "$src"
grep -Fq 'CdRead(1' "$src"
grep -Fq 'CdReadSync(1' "$src"
grep -Fq 'ARCHIVE_STREAM_FILE_NOT_LOADED' "$src"
# 0x200 is a different retail mode (2340-byte ADPCM sectors); it must remain
# fail-closed until PsyCross exposes that payload boundary.
grep -Fq 'if (flags & 0x200)' "$src"
echo 'archive stream retail structure: PASS'
