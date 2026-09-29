#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/timed_effect_integration_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x4a00:0x5258]).hexdigest()=='4ede271950970cf43e61e2864660eaa9a27bbd4145f191c81330dab80d6ec32c'
assert sha256(b[0x9d44:0xa32c]).hexdigest()=='760f9f457085e59b63a6ae781297309641b8a4e429cc7ced348c1cda0f6c4c19'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/timed_effect_integration_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/src/field_clip_control.c -o "$OUT/$opt.clip_control.o"
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/src/field_clip_data.c -o "$OUT/$opt.clip_data.o"
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/src/field_clip_prelude.c -o "$OUT/$opt.prelude.o"
 for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
  name=${source##*/}
  g++ -std=c++17 -fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections \
   -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
   "${flags[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
   -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
 done
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" "$OUT/$opt.clip_control.o" "$OUT/$opt.clip_data.o" "$OUT/$opt.prelude.o" "$OUT/$opt.LIBGTE.C.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in carry flags cleanup; do
 case "$mutant" in
  carry) source_file=pc_port/src/field_timed_commands.c; define=TIMED_SOURCE; expression='s/callback, retailObjectAddress/callback, 0/' ;;
  flags) source_file=pc_port/src/field_timed_commands.c; define=TIMED_SOURCE; expression='s/p\[0x12\] | 0x700/p[0x12] | 0x300/' ;;
  cleanup) source_file=pc_port/src/field_object_overlay.c; define=OBJECT_OVERLAY_SOURCE; expression='/^void func_801E165C(/,/^}/s/== 0) return/!= 0) return/' ;;
 esac
 sed "$expression" "$source_file" > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-D$define=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/timed_effect_integration_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" "$OUT/O2.clip_control.o" "$OUT/O2.clip_data.o" "$OUT/O2.prelude.o" "$OUT/O2.LIBGTE.C.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "TIMED EFFECT mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'TIMED EFFECT FAIL' "$OUT/$mutant.log"
done
echo 'TIMED EFFECT negative controls PASS: caller carry, allocation flags, cleanup gate'
