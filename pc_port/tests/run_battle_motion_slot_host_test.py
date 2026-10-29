"""Complete private motion-slot bodies, with pinned existing support objects."""
import hashlib,json,os,shlex,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_MOTION_SLOT_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-motion-track-20261019')
audit=json.loads((base/'final-audit.json').read_text())['pins']
proof=json.loads((base/'positive-suite.json').read_text())
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291','disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in private.items():assert sha(p)==d,p
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h',
 'include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','include/battle_lift.h',
 'pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h','pc_port/extern/PsyCross/include/psx/gtereg.h',
 'pc_port/tests/battle_motion_slot_host_test.c','pc_port/tests/run_battle_motion_slot_host_test.py','pc_port/tests/run_prologue_step_slice.sh',
 'asm/battle/nonmatchings/main73_p2/func_800AF518.s','asm/battle/nonmatchings/main58/func_8009C050.s','pc_port/src/battle_guest_abi_thunks.inc','pc_port/src/battle_overlay_host_leaves.inc']
pins={p:sha(p) for p in paths}
for p in paths:
 dest=root/'source-snapshot'/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(Path(p).read_bytes())
support_dir=Path('scratchpad/native-effect-reset-20261018/test-4')
gte_dir=Path('scratchpad/native-camera-update-20261018/test-4')
bridge=support_dir/'battle_bridge_map.inc';assert sha(bridge)==proof['support_pins'][str(bridge)];(root/'battle_bridge_map.inc').write_bytes(bridge.read_bytes())
common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src','-I'+str(root),'-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Wall','-Wextra','-Werror']
env=os.environ.copy();env['TMPDIR']=str(root/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0';env['XENO_CPU_EXECUTION_STATS']='1'
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
for name in ['read','write']:
 old='static int runtime_'+name+'('
 assert runtime.count(old)==1
 runtime=runtime.replace(old,'static int model_test_raw_'+name+'(')
observed=root/'observed-runtime.c';observed.write_text(runtime)
common+=['-DBATTLE_RUNTIME_SOURCE="'+str(observed)+'"']
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={}
(root/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private),indent=2)+'\n')
for mode in os.environ.get('BATTLE_MOTION_SLOT_TEST_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 support=[support_dir/(mode+'.'+x+'.o') for x in ['cpu','vblank','mask']]
 support += [gte_dir/(mode+'.'+x+'.o') for x in ['INLINE_C.C','PsyX_GTE.cpp','psyq_compat.c','retail_leaf_adapters.c']]
 for p in support:assert sha(p)==proof['support_pins'][str(p)],p;support_pins[str(p)]=sha(p)
 sdk_paths=[p for p in proof['support_pins'] if p.endswith('/'+mode+'.LIBGTE.C.o')];assert len(sdk_paths)==1
 sdk=Path(sdk_paths[0])
 assert sha(sdk)==proof['support_pins'][str(sdk)];support.append(sdk);support_pins[str(sdk)]=sha(sdk)
 obj=root/(mode+'.o');exe=root/(mode+'.test')
 cmd=['clang' if mode=='UBSan' else 'gcc']+common+flags+['-c','pc_port/tests/battle_motion_slot_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 exports=['-Wl,--export-dynamic-symbol='+name for name in ['D_80050104','D_800595C0','D_80059308','D_8005930C','D_800500F8','D_800500FC','D_80050100','D_80059578']]
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections']+exports+[str(obj)]+list(map(str,support))+['-ldl','-lcrypto']+shlex.split(subprocess.check_output(['pkg-config','--libs','sdl2'],text=True))+['-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_MOTION_SLOT_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,support_pins=support_pins,returncodes=returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Complete352-byte motion-slot parent and144-byte actual status child; full CPU/GTE/memory, ordered accesses/failures and real CallJal interface. Original instruction execution is test-only, native legs require zero interpreted instructions. Full game and owner-device acceptance remain incomplete.'),indent=2)+'\n')
print('MOTION SLOT HOST SUITE PASS',flush=True)
