"""Complete guest callback selector and existing native callback consumer against original code."""
import hashlib,json,os,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_CALLBACK_SELECTOR_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-effect-models-20261018');audit=json.loads((base/'final-audit.json').read_text())['pins']
support_dir=Path('scratchpad/native-effect-reset-20261018/test-4')
gte_dir=Path('scratchpad/native-camera-update-20261018/test-4')
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'scratchpad/native-module-20261017/module-test-2/module.bin':'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885','disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119','disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'}
for p,d in private.items():assert sha(p)==d,p
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h','include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','pc_port/tests/battle_callback_selector_host_test.c','pc_port/tests/run_battle_callback_selector_host_test.py','pc_port/tests/run_prologue_step_slice.sh']
paths+=['pc_port/extern/PsyCross/src/psx/INLINE_C.C','pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp','pc_port/extern/PsyCross/include/psx/gtereg.h','pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h','pc_port/src/psyq_compat.c','pc_port/src/psyq_compat_leaf.inc','pc_port/src/gte_normalize_table.h','pc_port/src/retail_leaf_adapters.c','pc_port/extern/PsyCross/src/psx/LIBGTE.C','pc_port/src/port_compat.h']
paths+=['pc_port/tests/battle_effects_render_host_test.c','pc_port/extern/PsyCross/include/psx/gtemac.h','pc_port/extern/PsyCross/include/psx/inline_c.h','pc_port/extern/PsyCross/include/psx/libgte.h']
paths+=['src/battle/main68.c','src/battle/main72.c','include/common.h','asm/battle/matchings/main72_p1/func_800AA820.s']
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
consumer=root/'actual-consumer.c'
consumer.write_text('#include \"'+str(repo/'src/battle/main68.c')+'\"\nint32_t selector_consume(uint32_t fn,int32_t a,int32_t b,int32_t c){return m68_effect_callback(fn,a,b,c); }\n')
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={}
(root/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private),indent=2)+'\n')
for mode in os.environ.get('BATTLE_CALLBACK_SELECTOR_TEST_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 support=[support_dir/(mode+'.'+x+'.o') for x in ['cpu','vblank','mask']]
 gte=[gte_dir/(mode+'.'+name+'.o') for name in ['INLINE_C.C','PsyX_GTE.cpp','psyq_compat.c','retail_leaf_adapters.c']]
 gte.append(base/'test-7'/(mode+'.LIBGTE.C.o'))
 for p in support+gte:assert sha(p)==audit[str(p)],p;support_pins[str(p)]=sha(p)
 obj=root/(mode+'.o');exe=root/(mode+'.test');consumer_obj=root/(mode+'.consumer.o')
 cmd=['clang' if mode=='UBSan' else 'gcc']+[x for x in common if x not in ['-Wall','-Wextra','-Werror']]+flags+['-DXENO_BATTLE_OVERLAY_HOST_BODIES','-c',str(consumer),'-o',str(consumer_obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=['clang' if mode=='UBSan' else 'gcc']+common+flags+['-c','pc_port/tests/battle_callback_selector_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 exports=['-Wl,--export-dynamic-symbol='+name for name in ['D_80050104','D_800595C0','D_80059308','D_8005930C','D_800500F8','D_800500FC','D_80050100','D_80059578']]
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections']+exports+[str(obj),str(consumer_obj)]+list(map(str,support+gte))+['-ldl','-lcrypto','-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_CALLBACK_SELECTOR_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,support_pins=support_pins,returncodes=returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Complete 120-byte AA820 guest callback selector: full architecture, unchanged memory/GTE, identity/context/opt-out and canonical cache boundaries. Actual native JAL and existing main68 callback consumer composed with four production callbacks and SDK trig, compared against original callback return values for nonzero divisors. Original CPU is test-only; native legs disabled. No complete motion engine, battle or whole-game acceptance.'),indent=2)+'\n')
print('CALLBACK SELECTOR HOST SUITE PASS',flush=True)
