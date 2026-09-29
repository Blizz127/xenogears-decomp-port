#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
main="$repo_root/pc_port/extern/PsyCross/src/PsyX_main.cpp"
build="$repo_root/pc_port/build_port.sh"
patch="$repo_root/pc_port/patches/psycross_video_recording.patch"
recording="$repo_root/pc_port/src/psycross_video_recording.inl"

test -f "$patch"
grep -q '_xeno_video_recording' "$main"
grep -q 'SDL_SCANCODE_F9' "$main"
grep -q 'PsyX_RecordPresentedFrame();' "$main"
grep -q 'PsyX_StopRecording();' "$main"
grep -q 'psycross_video_recording.patch' "$build"
grep -q 'XENO_RECORDING_AUDIO_SOURCE' "$recording"
grep -q 'get-default-sink' "$recording"
grep -q '"-f", "pulse"' "$recording"
grep -q '"-c:a", "aac"' "$recording"
grep -q '"-ac", "2"' "$recording"
grep -q '"-map", "0:v:0", "-map", "1:a:0"' "$recording"
grep -q '"-use_wallclock_as_timestamps", "1"' "$recording"
grep -q 'CLOCK_MONOTONIC' "$recording"
grep -q 'g_xenoRecordingNextFrameNs' "$recording"
python3 - "$recording" <<'PY'
from pathlib import Path
import sys

text = Path(sys.argv[1]).read_text()
assert text.count('"-use_wallclock_as_timestamps", "1"') == 1, \
    'Pulse input must retain native timestamps; only raw video gets wallclock timestamps'
PY
if grep -q '"-an"' "$recording"; then
    echo 'recording explicitly disables audio' >&2
    exit 1
fi

# Both presentation routes must capture the completed backbuffer before swap.
python3 - "$main" <<'PY'
from pathlib import Path
import sys

text = Path(sys.argv[1]).read_text()
for name, signature in (
    ("PsyX_PresentDisplayFromVRAM", "PsyX_PresentDisplayFromVRAM(void)"),
    ("PsyX_EndScene", "PsyX_EndScene()"),
):
    start = text.index(signature)
    end = text.index("\n}", start)
    body = text[start:end]
    assert body.index("PsyX_RecordPresentedFrame();") < body.index("GR_SwapWindow();"), name
PY

# F9 is host-only and must never become a virtual PlayStation button.
if grep -q 'SDL_SCANCODE_F9' "$repo_root/pc_port/extern/PsyCross/src/pad/PsyX_pad.cpp"; then
    echo 'F9 leaked into PlayStation pad mapping' >&2
    exit 1
fi

echo 'PsyCross video recording regression: PASS'
