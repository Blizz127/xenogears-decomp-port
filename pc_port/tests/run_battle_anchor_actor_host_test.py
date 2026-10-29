"""Full actor coordinator and anchor composition against original code."""
import hashlib,json,os,shlex,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_ANCHOR_ACTOR_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-effect-models-20261018');audit=json.loads((base/'final-audit.json').read_text())['pins']
support_dir=Path('scratchpad/native-effect-reset-20261018/test-4')
gte_dir=Path('scratchpad/native-camera-update-20261018/test-4')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'scratchpad/native-module-20261017/module-test-2/module.bin':'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885','disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119','disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'}
for p,d in private.items():assert sha(p)==d,p
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h','include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','pc_port/tests/battle_anchor_update_host_test.c','pc_port/tests/run_battle_anchor_actor_host_test.py','pc_port/tests/run_prologue_step_slice.sh']
paths+=['pc_port/extern/PsyCross/src/psx/INLINE_C.C','pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp','pc_port/extern/PsyCross/include/psx/gtereg.h','pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h','pc_port/src/psyq_compat.c','pc_port/src/psyq_compat_leaf.inc','pc_port/src/gte_normalize_table.h','pc_port/src/retail_leaf_adapters.c','pc_port/extern/PsyCross/src/psx/LIBGTE.C','pc_port/src/port_compat.h']
paths+=['pc_port/tests/battle_anchor_update_host_test.c','pc_port/tests/battle_effects_render_host_test.c','pc_port/extern/PsyCross/include/psx/gtemac.h','pc_port/extern/PsyCross/include/psx/inline_c.h','pc_port/extern/PsyCross/include/psx/libgte.h']
paths+=['asm/battle/nonmatchings/main68/func_800A44C0.s','src/battle/main68.c']
paths+=['include/battle_lift.h','include/battle_guest_abi.h','pc_port/extern/PsyCross/src/psx/LIBGTE.C']
paths+=['pc_port/tests/battle_anchor_actor_host_test.c','pc_port/tests/battle_actor_update_host_test.c','pc_port/tests/battle_actor_update_integration_test.c','pc_port/tests/battle_anchor_update_integration_test.c','pc_port/build_port.sh','pc_port/patches/psycross_compmatrix_translation_overflow.patch','asm/slus_006.64/matchings/psyq/libgte/CompMatrix.s']
pins={p:sha(p) for p in paths}
for p in paths:
 dest=root/'source-snapshot'/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(Path(p).read_bytes())
bridge=support_dir/'battle_bridge_map.inc';assert sha(bridge)==audit[str(bridge)];(root/'battle_bridge_map.inc').write_bytes(bridge.read_bytes())
common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src','-I'+str(root),'-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Wall','-Wextra','-Werror']
env=os.environ.copy();env['TMPDIR']=str(root/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0'
# Instrument only the production bus definitions. Calls and bodies are intact.
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
for name in ['read','write']:
 old='static int runtime_'+name+'('
 assert runtime.count(old)==1
 runtime=runtime.replace(old,'static int model_test_raw_'+name+'(')
observed=root/'observed-runtime.c';observed.write_text(runtime)
common+=['-DBATTLE_RUNTIME_SOURCE="'+str(observed)+'"']
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={}
(root/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private),indent=2)+'\n')
for mode in os.environ.get('BATTLE_ANCHOR_ACTOR_TEST_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 support=[support_dir/(mode+'.'+x+'.o') for x in ['cpu','vblank','mask']]
 gte=[gte_dir/(mode+'.'+name+'.o') for name in ['INLINE_C.C','PsyX_GTE.cpp','psyq_compat.c','retail_leaf_adapters.c']]
 for p in support+gte:assert sha(p)==audit[str(p)],p;support_pins[str(p)]=sha(p)
 sdk=root/(mode+'.LIBGTE.C.o')
 cmd=['g++','-std=c++17','-fno-pie','-DUSE_EXTENDED_PRIM_POINTERS=0','-DUSE_PGXP=0','-ffunction-sections','-fdata-sections','-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-fpermissive','-w','-include','pc_port/src/port_compat.h']+flags+['-c','pc_port/extern/PsyCross/src/psx/LIBGTE.C','-o',str(sdk)]
 commands.append(cmd);subprocess.run(cmd,check=True,env=env);gte.append(sdk)
 obj=root/(mode+'.o');exe=root/(mode+'.test')
 cmd=['clang' if mode=='UBSan' else 'gcc']+common+flags+['-c','pc_port/tests/battle_anchor_actor_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 exports=['-Wl,--export-dynamic-symbol='+name for name in ['D_80050104','D_800595C0','D_80059308','D_8005930C','D_800500F8','D_800500FC','D_80050100','D_80059578']]
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections']+exports+[str(obj)]+list(map(str,support+gte))+['-ldl','-lcrypto']+shlex.split(subprocess.check_output(['pkg-config','--libs','sdl2'],text=True))+['-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_ANCHOR_ACTOR_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,support_pins=support_pins,returncodes=returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Complete actor and anchor bodies with original/native matrix SDK, full parent and child GPR/CP0/HI/LO and final memory/GTE state, parent ordered accesses and all child entry snapshots. SDK CPU volatile registers normalize to the C ABI; nine other overlay children remain modeled. Existing actor body/fault/gate/cache and actual SDK tests are included. Original CPU test-only, native legs interpreter disabled. Normal boot, hardware graphics/audio/timing/save and whole game are separate requirements.'),indent=2)+'\n')
print('ANCHOR ACTOR HOST SUITE PASS',flush=True)
