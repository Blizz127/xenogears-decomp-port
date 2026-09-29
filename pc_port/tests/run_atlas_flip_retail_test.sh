#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/atlas_flip_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json
p=Path('pc_port/build_native/atlas_flip_retail_test');b=Path('disc/SLUS_006.64').read_bytes();assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
s=Path('src/slus_006.64/system/temp1f.c').read_text();a=s.index('s32 func_800263E4(');z=s.index('\n}\n',a)+3
(p/'body.c').write_text(s[a:z]);(p/'pins.json').write_text(json.dumps({'retail':sha256(b[0x16be4:0x16f5c]).hexdigest(),'body':sha256(s[a:z].encode()).hexdigest()},indent=2))
PY
sed -n -e '/^u_short GetClut(/,/^}/p' -e '/^u_short GetTPage(/,/^}/p' -e '/^void SetSemiTrans(/,/^}/p' -e '/^void SetPolyFT4(/,/^}/p' -e '/^void SetShadeTex(/,/^}/p' pc_port/extern/PsyCross/src/psx/LIBGPU.C > "$OUT/gpu.c"
COMMON=(-DXENO_PC_PORT -include stdint.h -include assert.h -std=gnu17 -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -D_LANGUAGE_C -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan;do
 flags=(-"$opt");if [ "$opt" = UBSan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${COMMON[@]}" "${flags[@]}" -include common.h -include psyq/libgpu.h -c "$OUT/body.c" -o "$OUT/$opt.body.o"
 gcc "${COMMON[@]}" "${flags[@]}" -include common.h -include psyq/libgpu.h -c "$OUT/gpu.c" -o "$OUT/$opt.gpu.o"
 clang -std=c17 -Wall -Wextra -Werror -no-pie "${flags[@]}" -Ipc_port/src pc_port/tests/atlas_flip_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.gpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
python3 - <<'PY'
from pathlib import Path
p=Path('pc_port/build_native/atlas_flip_retail_test');s=(p/'body.c').read_text()
for name,old,new in [('buffer','buffer * 40','buffer * 80'),('flip-byte','(u8)flipX','flipX'),('vertical-flip','if ((u8)flipY)','if (0)'),('negative-round','product += 0xFFF','product += 0'),('uv-direction','poly->x3 < poly->x0','poly->x3 > poly->x0'),('uv-clamp','u = 0; --uw;','u = 0;')]:
 assert old in s,name
 (p/(name+'.c')).write_text(s.replace(old,new))
PY
for mutant in buffer flip-byte vertical-flip negative-round uv-direction uv-clamp;do
 gcc "${COMMON[@]}" -O2 -include common.h -include psyq/libgpu.h -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -Wall -Wextra -Werror -no-pie -O2 -Ipc_port/src pc_port/tests/atlas_flip_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" "$OUT/O2.gpu.o" -o "$OUT/$mutant.test"
 rc=0; "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^ATLAS FLIP FAIL' "$OUT/$mutant.log";then cat "$OUT/$mutant.log";exit 1;fi
 echo "ATLAS FLIP MUTANT REJECTED $mutant"
done
