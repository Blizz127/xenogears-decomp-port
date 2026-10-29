"""Pinned exact-signature actor interfaces and actual native heap-size body."""
import hashlib,json,os,resource,subprocess
from pathlib import Path
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
out=Path(os.environ['BATTLE_ACTOR_ABI_OUT']).resolve();out.mkdir(exist_ok=False)
base=Path('scratchpad/native-resource-init-20261018');audit=json.loads((base/'final-audit.json').read_text())['pins']
support=Path('scratchpad/native-effect-reset-20261018/test-4')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in private.items():assert sha(p)==d,p
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h','include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','src/slus_006.64/system/memory.c','pc_port/tests/battle_actor_create_host_test.c','pc_port/tests/battle_actor_service_abi_test.c','pc_port/tests/run_battle_actor_service_abi_test.py']
pins={p:sha(p) for p in paths}
for p in paths:
 dest=out/'source-snapshot'/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(Path(p).read_bytes())
bridge=support/'battle_bridge_map.inc';assert sha(bridge)==audit[str(bridge)];(out/'battle_bridge_map.inc').write_bytes(bridge.read_bytes())
source=Path('src/slus_006.64/system/memory.c').read_text();a=source.index('void* HeapGetNextBlockHeader(');b=source.index('\n}',a)+2
(out/'heap-query.c').write_text('#include "common.h"\n#include "system/memory.h"\n#include <stdint.h>\n'+source[a:b]+'\n')
common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src','-I'+str(out),'-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx']
env=os.environ.copy();env['TMPDIR']=str(out/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0'
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={}
(out/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private),indent=2)+'\n')
for mode in os.environ.get('BATTLE_ACTOR_ABI_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 cc='clang' if mode=='UBSan' else 'gcc'
 objs=[support/(mode+'.'+x+'.o') for x in ['compat','cpu','vblank','mask']]
 for p in objs:assert sha(p)==audit[str(p)],p;support_pins[str(p)]=sha(p)
 obj=out/(mode+'.o');heap=out/(mode+'.heap.o');exe=out/(mode+'.test')
 for src,dst,warnings in [('pc_port/tests/battle_actor_service_abi_test.c',obj,['-Wall','-Wextra','-Werror']),(str(out/'heap-query.c'),heap,['-w'])]:
  cmd=[cc]+common+flags+warnings+['-c',src,'-o',str(dst)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections',str(obj),str(heap)]+list(map(str,objs))+['-ldl','-lcrypto','-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_ACTOR_ABI_ARGS','').split();commands.append(cmd)
 with (out/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc;print(mode,'status',rc,flush=True)
 if rc:print((out/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(out/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,support_pins=support_pins,returncodes=returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in out.iterdir() if p.is_file()},scope='Seven concrete actor service interfaces; sound/model/texture bodies modeled. The complete guest-domain unrelocator has its own differential and cache-precedence suite. Original heap-size query comparison uses a registered host pointer with consistent native links; translated guest aliases tested separately with actual native heap links.'),indent=2)+'\n')
print('ACTOR SERVICE ABI SUITE PASS',flush=True)
