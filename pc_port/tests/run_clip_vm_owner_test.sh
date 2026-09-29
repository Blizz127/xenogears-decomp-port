#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_vm_owner_test
mkdir -p "$OUT"
python3 - <<'PY'
from tools.scripts.audit_field_clip_vm import inventory, read_overlay
inventory(read_overlay('disc/disc1.bin'))
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/clip_vm_owner_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in exit_owner origin mode_mask; do
 python3 - "$mutant" "$OUT/$mutant.c" <<'PY'
from pathlib import Path
import sys
changes = {
    'exit_owner': ('*(u32*)(state.object + 0x10) = state.stream;',
                   '*(u32*)(obj + 0x10) = state.stream;'),
    'origin': ('state.origin = obj;', 'state.origin = NULL;'),
    'mode_mask': ('D_801E85CC = (u32)value & 1u;',
                  'D_801E85CC = (u32)value & 2u;'),
}
source = Path('pc_port/src/field_object_overlay.c').read_text()
before, after = changes[sys.argv[1]]
assert source.count(before) == 1
Path(sys.argv[2]).write_text(source.replace(before, after))
PY
 gcc "${common[@]}" -O2 -fpermissive -w \
  "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
  -c pc_port/tests/clip_vm_owner_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CLIP VM OWNER mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CLIP VM OWNER FAIL|state->origin' "$OUT/$mutant.log"
done
echo 'CLIP VM OWNER negative controls PASS: exit destination, entry identity, blend-mode mask'
