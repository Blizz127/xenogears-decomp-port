#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${CAMERA_INIT_OUT:-$(mktemp -d pc_port/build_native/camera-init.XXXXXXXX)}
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
import hashlib,sys
p=Path(sys.argv[1]);b=Path('disc/field.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
assert hashlib.sha256(b[0x2a5c:0x2bf8]).hexdigest()=='35815dc632d53d4c0346abf2e3a920fe1846f314ecd17d4ad1fbf66e3cbfb0a7'
s=Path('src/field/main/misc2.c').read_text();a=s.index('void func_8007254C(void) {');z=s.index('\n}',a)+2;(p/'body.c').write_text(s[a:z]+'\n')
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt");if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang -std=gnu17 -fno-strict-aliasing "${flags[@]}" -Ipc_port/src "-DCAMERA_INIT_BODY=\"$PWD/$OUT/body.c\"" pc_port/tests/field_camera_init_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt"
 "$OUT/$opt" disc/field.bin
done
printf 'Artifacts: %s\n' "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys,json,hashlib
p=Path(sys.argv[1]);s=(p/'body.c').read_text();controls=[('matrix','func_80070594(&g_Scene.camRotationMatrix);'),('mode','g_FieldCameraMode = 0;'),('flags','g_CamMovementFlags = 0;'),('at-timer','g_CamAtMovementDuration = 0;'),('eye-timer','g_CamEyeMovementDuration = 0;')]+[(f'scene-{x}',f'*(s16*)((u8*)&g_Scene + 0x{x}) = 0;') for x in ('70','80','8C')]
for name,line in controls:
 assert s.count(line)==1,(name,line)
 (p/(name+'.c')).write_text(s.replace(line,'/* negative control */',1))
paths=['src/field/main/misc2.c','pc_port/tests/field_camera_init_retail_test.c','pc_port/tests/run_field_camera_init_retail_test.sh']
(p/'pins.json').write_text(json.dumps({x:hashlib.sha256(Path(x).read_bytes()).hexdigest() for x in paths},indent=2)+'\n')
PY
for mutant in matrix mode flags at-timer eye-timer scene-70 scene-80 scene-8C; do
 clang -std=gnu17 -fno-strict-aliasing -O2 -Ipc_port/src "-DCAMERA_INIT_BODY=\"$PWD/$OUT/$mutant.c\"" pc_port/tests/field_camera_init_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/mutant"
 if "$OUT/mutant" disc/field.bin > "$OUT/$mutant.log" 2>&1; then echo "Unexpected mutant pass: $mutant";exit 1;fi
 echo "Negative control rejected: $mutant"
done
