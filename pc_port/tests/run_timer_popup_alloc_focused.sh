#!/usr/bin/env bash
# Two popup-sized cases from the existing retail timer-task differential.
set -euo pipefail
if [[ ${XENO_TIMER_POPUP_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_TIMER_POPUP_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/.."/..
ulimit -c 0
ulimit -v 12582912
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-timer-popup.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import sys
image=Path('disc/SLUS_006.64').read_bytes()
part=image[0x8001cc18-0x8000f800:0x8001cc18-0x8000f800+0x25c]
assert sha256(part).hexdigest()=='73e431046377a0795e3627cc44af06c84574a20c3b25032ecd6df2015519697d'
lookup=image[0x8001d0a4-0x8000f800:0x8001d0a4-0x8000f800+0x68]
assert sha256(lookup).hexdigest()=='4d2e3d0559a80d25efb325eadf0c64edacc7fabd24e3147a3ade1cfd7c0f1090'
s=Path('pc_port/tests/timer_work_list_retail_test.c').read_text()
assert s.count('int main(void)\n')==1
s=s.split('int main(void)\n',1)[0]+'''int main(void) {
 assert(TimerWorkListAllocateTask && TimerWorkListDeleteTask);
 assert((uintptr_t)g_PsxRam + sizeof(g_PsxRam) < UINT32_MAX);
 FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
 assert(!fseek(f,0x8001cc18u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8001cc18u),1,0x25c,f)==0x25c);
 assert(!fseek(f,0x8001d0a4u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8001d0a4u),1,0x68,f)==0x68);fclose(f);
 assert(compare_case(0,0,7,0,1,0,12,0));
 assert(compare_case(0,0x55,7,1,1,0,12,0));
 put(OWNER+0x10u,7u);
 for(unsigned n=0;n<2;n++) {
  u32 node=NODE+n*0x40u;
  put(node,WL_U32(PSX_ADDR(OWNER)));put(node+8u,0x800bcfacu);
  put(node+0x14u,7u);put(node+0x18u,n?0:WL_U32(PSX_ADDR(NODE+0x40u)));
 }
 g_TimerWorkList=PSX_ADDR(NODE);put(0x8005958cu,WL_U32(g_TimerWorkList));
 u32 owner=WL_U32(PSX_ADDR(OWNER));
 assert(retail(0x8001d0a4u,owner,0x800bcfacu)==WL_U32(PSX_ADDR(NODE)));
 assert(func_8001D0A4(PSX_ADDR(OWNER),(void *)(uintptr_t)0x800bcfacu)==PSX_ADDR(NODE));
 put(NODE+8u,0x800bcbb4u);put(NODE+0x48u,0x800bcbb4u);
 assert(retail(0x8001d0a4u,owner,0x800bcfacu)==0);
 assert(func_8001D0A4(PSX_ADDR(OWNER),(void *)(uintptr_t)0x800bcfacu)==NULL);
 puts("PASS synthetic popup-sized timer allocation and lookup comparisons");
 return 0;
}
'''
Path(sys.argv[1],'focused.c').write_text(s)
PY
COMMON=(-std=gnu17 -O1 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
gcc "${COMMON[@]}" -w -c "$OUT/focused.c" -o "$OUT/test.o"
gcc "${COMMON[@]}" -w -c pc_port/src/work_list_port.c -o "$OUT/native.o"
gcc "${COMMON[@]}" -w -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"
gcc -no-pie -Wl,--gc-sections "$OUT/test.o" "$OUT/native.o" "$OUT/cpu.o" -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test"
