"""Complete color-fade bodies and fixed graphics arithmetic against original code."""
import hashlib,json,os,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_COLOR_FADE_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-timer-task-20261018');audit=json.loads((base/'final-audit.json').read_text())['pins']
support_dir=Path('scratchpad/native-effect-reset-20261018/test-4')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'scratchpad/native-module-20261017/module-test-2/module.bin':'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885','disc/SLUS_006.64':sha('disc/SLUS_006.64'),'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'}
for p,d in private.items():assert sha(p)==d,p
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h','include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','pc_port/tests/battle_color_fade_host_test.c','pc_port/tests/run_battle_color_fade_host_test.py']
paths+=['pc_port/extern/PsyCross/src/psx/INLINE_C.C','pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp','pc_port/extern/PsyCross/include/psx/gtereg.h','pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h']
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
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={}
(root/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private),indent=2)+'\n')
for mode in os.environ.get('BATTLE_COLOR_FADE_TEST_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 support=[support_dir/(mode+'.'+x+'.o') for x in ['compat','cpu','vblank','mask']]
 for p in support:assert sha(p)==audit[str(p)],p;support_pins[str(p)]=sha(p)
 gte=[]
 for name in ['psx/INLINE_C.C','gte/PsyX_GTE.cpp']:
  output=root/(mode+'.'+Path(name).name+'.o');gte.append(output)
  cmd=['g++','-std=c++17','-fno-pie','-ffunction-sections','-fdata-sections','-DUSE_EXTENDED_PRIM_POINTERS=0','-DUSE_PGXP=0','-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-fpermissive','-w','-include','pc_port/src/port_compat.h']+flags
  if name=='psx/INLINE_C.C':cmd+=['-DdoCOP2=color_fade_backend_command']
  cmd+=['-c','pc_port/extern/PsyCross/src/'+name,'-o',str(output)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 obj=root/(mode+'.o');exe=root/(mode+'.test')
 cmd=['clang' if mode=='UBSan' else 'gcc']+common+flags+['-c','pc_port/tests/battle_color_fade_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections',str(obj)]+list(map(str,support+gte))+['-ldl','-lcrypto','-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_COLOR_FADE_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)

for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,support_pins=support_pins,returncodes=returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Five complete color-fade setup/update/remove/setter bodies plus the previously adopted timer setter. Complete local helper composition; list services modeled. Full GPR/CP0/HI/LO and observable GTE register state, ordered accesses and byte-exact full memory at calls and return. Fixed graphics arithmetic is native C, checked against the existing PsyCross backend; independent hardware validation, SDK volatile state and pending-load resumption are outside scope.'),indent=2)+'\n')
print('COLOR FADE SUITE PASS',flush=True)
