#!/usr/bin/env bash
set -euo pipefail
if [[ ${XENO_SOUND_CLEAR_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 30s env XENO_SOUND_CLEAR_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
ulimit -c 0
ulimit -v 12582912
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-sound-clear.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
sed -n '/^void SoundClearVoiceDataPointers(/,/^}/p' src/slus_006.64/system/sound.c > "$OUT/sound_clear_body.inc"
gcc -std=gnu17 -O0 -Wall -Wextra -Werror -I"$OUT" pc_port/tests/sound_channel_clear_test.c -o "$OUT/test"
"$OUT/test"
