#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_bind_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x7534:0x75d0]).hexdigest()=='dcca05b1af95b39ec18df4bb4a29aed5b33da46091e9bbd34f20c607882edc7b'
assert sha256(b[0x75d0:0x76bc]).hexdigest()=='7b59dc488b040e5b2b64a898276871a668e7a1795002fb826e140ac020ddfb9b'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/clip_bind_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -DXENO_TEST_OVERLAY_UNIT -c pc_port/tests/clip_bind_retail_test.c -o "$OUT/$opt.overlay.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.overlay.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in reset_aux queue_cap signed_index aux_offset phase vm_args; do
 case "$mutant" in
  reset_aux) range='^void func_801E3534('; expression='s/clipTable, clipAux);/clipTable, 0);/' ;;
  queue_cap) range='^void func_801E35D0('; expression='s/obj\[0x2B\] < 5/obj[0x2B] < 4/' ;;
  signed_index) range='^void func_801E35D0('; expression='s/index < 0x50/(u32)index < 0x50/' ;;
  aux_offset) range='^void func_801E35D0('; expression='s/0x138u/0x134u/' ;;
  phase) range='^void func_801E35D0('; expression='s/u16 phase = (u16)D_801E863C;/u16 phase = 0;/' ;;
  vm_args) range='^void func_801E35D0('; expression='s/context, -1, 1, 0/context, 0, 1, 0/' ;;
 esac
 sed "/$range/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w -DXENO_TEST_OVERLAY_UNIT "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/clip_bind_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CLIP BIND mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CLIP BIND FAIL' "$OUT/$mutant.log"
done
echo 'CLIP BIND negative controls PASS: reset_aux queue_cap signed_index aux_offset phase vm_args'
# E39F0 has a real owner now: replace the obsolete missing-owner gate with
# its actual unsupported-dispatch fallback. clip_vm_owner_test covers its
# entry/exit ownership; here controlled handlers both decline an instruction.
gcc "${common[@]}" -O2 -fpermissive -w -DXENO_TEST_UNSUPPORTED_VM -c pc_port/tests/clip_bind_retail_test.c -o "$OUT/unsupported.o"
clang -no-pie -Wl,--gc-sections "$OUT/unsupported.o" "$OUT/O2.overlay.o" "$OUT/O2.cpu.o" -o "$OUT/unsupported.test"
python3 - "$OUT" <<'PY'
from pathlib import Path
import re
import signal
import subprocess
import sys

out = Path(sys.argv[1])
result = subprocess.run([str(out / 'unsupported.test')], text=True,
                        capture_output=True, check=False)
(out / 'unsupported.log').write_text(result.stdout + result.stderr)
values = re.fullmatch(r'expected-object=(0x[0-9a-f]+) expected-ip=([0-9a-f]{8})\n',
                      result.stdout)
if result.returncode != -signal.SIGABRT or not values:
    raise SystemExit(f'CLIP BIND FAIL unsupported owner did not SIGABRT: {result.returncode}')
expected = ('[obj-ovly] E39F0 unsupported retail clip opcode '
            f'obj={values[1]} ip={values[2]} opcode=70 limit=85 ticks=1 mode=0\n')
if result.stderr != expected:
    raise SystemExit(f'CLIP BIND FAIL unsupported owner diagnostic: {result.stderr!r}')
print('CLIP BIND unsupported owner PASS: SIGABRT and exact diagnostic; controlled rejecting handlers')
PY
