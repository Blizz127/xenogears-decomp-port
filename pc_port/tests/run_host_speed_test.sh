#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mktemp -d pc_port/build_native/host_speed_test.XXXXXXXX)
echo "HOST SPEED artifacts: $out"
python3 - "$out" <<'PY'
from pathlib import Path
import hashlib,json,sys
out=Path(sys.argv[1])
files=['pc_port/src/psycross_host_speed.inl','pc_port/src/host_speed.h',
       'pc_port/extern/PsyCross/src/PsyX_main.cpp',
       'pc_port/extern/PsyCross/src/psx/LIBCD.C',
       'pc_port/extern/PsyCross/src/audio/PsyX_SPUAL.cpp']
for path in files:
    if not Path(path).exists():
        raise SystemExit('HOST SPEED RED: missing '+path)
def function(text, signature):
    start=text.index(signature); brace=text.index('{',start); depth=0
    for i in range(brace,len(text)):
        depth+=(text[i]=='{')-(text[i]=='}')
        if not depth:return text[start:i+1]+'\n'
main=Path(files[2]).read_text(); cd=Path(files[3]).read_text(); audio=Path(files[4]).read_text()
code='\n'.join([function(main,'int PsyX_Sys_GetVBlankCount()'),
                 function(main,'int intrThreadMain(void* data)'),
                 function(cd,'static void _eCdSpoolerPace(void)'),
                 function(audio,'void PsyX_SPUAL_RefreshHostSpeed()')])
(out/'production.inc').write_text(code)
mutants = {
 'vblank-unscaled': code.replace(') / speed;', ');', 1),
 'sound-unscaled': code.replace('PSYX_SOUND_CNT2_PERIOD / speed', 'PSYX_SOUND_CNT2_PERIOD'),
 'disc-unscaled': code.replace(' * PsyX_GetSpeedMultiplier()', '', 1),
 'query-increments': code.replace('int PsyX_Sys_GetVBlankCount()\n{',
                                  'int PsyX_Sys_GetVBlankCount()\n{\n    g_psxSysCounters[0]++;'),
 'audio-register-corruption': code.replace('SDL_UnlockMutex(g_SpuMutex);',
                                          'g_SpuVoices[0].attr.pitch ^= 1; SDL_UnlockMutex(g_SpuMutex);')
}
for name, text in mutants.items():
    assert text != code, name
    (out/name).mkdir()
    (out/name/'production.inc').write_text(text)
files += ['pc_port/tests/host_speed_test.cpp','pc_port/tests/run_host_speed_test.sh',
          'pc_port/src/psycross_host_toolbar_logic.h']
(out/'provenance.json').write_text(json.dumps({p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in files},indent=2)+'\n')
PY
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    g++ -std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter \
        "${flags[@]}" $(pkg-config --cflags sdl2) -I"$out" \
        pc_port/tests/host_speed_test.cpp $(pkg-config --libs sdl2) \
        -o "$out/$mode"
    "$out/$mode" >"$out/$mode.log" 2>&1
    tail -n 1 "$out/$mode.log"
done
for mutant in vblank-unscaled sound-unscaled disc-unscaled query-increments audio-register-corruption; do
    g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter \
        $(pkg-config --cflags sdl2) -I"$out/$mutant" \
        pc_port/tests/host_speed_test.cpp $(pkg-config --libs sdl2) \
        -o "$out/$mutant/test"
    rc=0
    "$out/$mutant/test" >"$out/$mutant/result.log" 2>&1 || rc=$?
    test "$rc" -ne 0
    grep -q 'Assertion .* failed' "$out/$mutant/result.log"
    echo "HOST SPEED rejected $mutant"
done
