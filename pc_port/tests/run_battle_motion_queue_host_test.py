"""Original-body motion queue/restart body comparisons with pinned support objects."""
import hashlib,json,os,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_MOTION_QUEUE_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-resource-bounds-20261018');audit=json.loads((base/'final-audit.json').read_text())['pins']
support_dir=Path('scratchpad/native-effect-reset-20261018/test-4')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291','disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in private.items():assert sha(p)==d,p
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h','include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','pc_port/tests/battle_motion_queue_host_test.c','pc_port/tests/run_battle_motion_queue_host_test.py','pc_port/tests/battle_motion_service_abi_test.c']
paths+=['pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h']
pins={p:sha(p) for p in paths}
for p in paths:
 dest=root/'source-snapshot'/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(Path(p).read_bytes())
bridge=support_dir/'battle_bridge_map.inc';assert sha(bridge)==audit[str(bridge)];(root/'battle_bridge_map.inc').write_bytes(bridge.read_bytes())
common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src','-I'+str(root),'-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Wall','-Wextra','-Werror']
env=os.environ.copy();env['TMPDIR']=str(root/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0'
# Only rename the two bus definitions. Every production call site, including
# the native bodies and oracle CPU bus, now reaches the fixture's observer.
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
for name in ['read','write']:
 old='static int runtime_'+name+'('
 assert runtime.count(old)==1
 runtime=runtime.replace(old,'static int model_test_raw_'+name+'(')
observed=root/'observed-runtime.c';observed.write_text(runtime)
common+=['-DBATTLE_RUNTIME_SOURCE="'+str(observed)+'"']
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={};abi_returncodes={}
(root/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private),indent=2)+'\n')
for mode in os.environ.get('BATTLE_MOTION_QUEUE_TEST_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 support=[support_dir/(mode+'.'+x+'.o') for x in ['compat','cpu','vblank','mask']]
 for p in support:assert sha(p)==audit[str(p)],p;support_pins[str(p)]=sha(p)
 obj=root/(mode+'.o');exe=root/(mode+'.test')
 cmd=['clang' if mode=='UBSan' else 'gcc']+common+flags+['-c','pc_port/tests/battle_motion_queue_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections',str(obj)]+list(map(str,support))+['-ldl','-lcrypto','-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_MOTION_QUEUE_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
 abi_common=[a for a in common if not a.startswith('-DBATTLE_RUNTIME_SOURCE=')]+['-DBATTLE_RUNTIME_SOURCE="'+str(repo/'pc_port/src/battle_mips_runtime.c')+'"']
 obj=root/(mode+'.abi.o');exe=root/(mode+'.abi.test')
 cmd=['clang' if mode=='UBSan' else 'gcc']+abi_common+flags+['-c','pc_port/tests/battle_motion_service_abi_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections',str(obj)]+list(map(str,support))+['-ldl','-lcrypto','-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)];commands.append(cmd)
 with (root/(mode+'.abi.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 abi_returncodes[mode]=rc;print(mode,'ABI status',rc,flush=True)
 if rc:print((root/(mode+'.abi.log')).read_text(),flush=True);raise SystemExit(rc)

for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,support_pins=support_pins,returncodes=returncodes,abi_returncodes=abi_returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Complete original 236-byte motion queue/restart body; motion-updater boundary modeled for parent-body proof. Ordered accesses, full GPR/CP0/HI/LO, complete memory and updater call snapshots compared. Native legs require zero interpreted instructions. Failure and pending-return boundaries are explicit; test-only original CPU oracle is not production code. Separate exact five-word void ABI tests exercise the production bridge, including resolved/cached calls and pending/invalid frames.'),indent=2)+'\n')
print('MOTION QUEUE SUITE PASS',flush=True)
