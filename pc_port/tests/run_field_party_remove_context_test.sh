#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_party_remove_context
mkdir -p "$OUT"
python3 - <<'PY'
import hashlib,re
from pathlib import Path
b=Path('disc/field.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
s=b[0x8008c180-0x8006faf0:0x8008c334-0x8006faf0]
assert hashlib.sha256(s).hexdigest()=='7c3ddcc8e2df40fe264b7f455c4786b4598014017b04f839dd02490595b0fdba'
a=Path('asm/field/matchings/main/misc/func_8008C180.s').read_text()
assert b''.join(bytes.fromhex(w) for w in re.findall(r'/\* [0-9A-F]+ [0-9A-F]+ ([0-9A-F]{8}) \*/',a))==s
PY
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT
      -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
      -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${BASE[@]}" "${flags[@]}" -fpermissive -w -c src/field/main/misc.c -o "$OUT/$mode.owner.o"
 gcc "${BASE[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/field_party_remove_context_test.c -o "$OUT/$mode.test.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.test.o" "$OUT/$mode.owner.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
python3 - <<'PY'
from pathlib import Path
s=Path('src/field/main/misc.c').read_text();start=s.index('void func_8008C180(');end=s.index('\nextern s32 D_800ADBC4;',start)
body=s[start:end]
changes={
 'no_teardown':('        func_80080A74(slot);',''),
 'wrong_context':('        g_FieldScriptVMCurActor = savedActor;','        g_FieldScriptVMCurActor = (ActorData*)savedFieldActor;'),
 'wrong_ip_owner':('        target->scriptInstructionPointer = savedIP;','        savedActor->scriptInstructionPointer = savedIP;'),
 'no_slot_reset':('        D_8005A444[partyId] = 0xFF;',''),
 'no_target_context':('        g_FieldScriptVMCurActor = (ActorData*)(uintptr_t)fieldActor->pActorData;',''),
}
for name,(old,new) in changes.items():
 assert body.count(old)==1
 Path('pc_port/build_native/field_party_remove_context/'+name+'.c').write_text(s[:start]+body.replace(old,new)+s[end:])
PY
for mutant in no_teardown wrong_context wrong_ip_owner no_slot_reset no_target_context; do
 gcc "${BASE[@]}" -O0 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/O0.test.o" "$OUT/$mutant.o" -o "$OUT/$mutant"
 if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then
  echo "FAIL negative control survived: $mutant" >&2; exit 1
 fi
 grep -q '^FAIL ' "$OUT/$mutant.log"
done
echo 'PARTY REMOVE CONTEXT negative controls PASS count=5'

gcc "${BASE[@]}" -O0 -Ipc_port/src pc_port/tests/field_party_remove_retail_test.c pc_port/src/battle_mips_adapter.c -no-pie -o "$OUT/retail"
"$OUT/retail" | tee "$OUT/retail.log"
cmp "$OUT/O0.log" "$OUT/retail.log"
