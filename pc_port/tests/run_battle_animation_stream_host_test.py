"""Complete private animation-stream bodies, with pinned existing support objects."""
import hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_ANIMATION_STREAM_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-motion-track-20261019')
proof=json.loads((base/'positive-suite.json').read_text())
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291','disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in private.items():assert sha(p)==d,p
private_bodies={'0x800ae1bc': {'offset': 255692, 'bytes': 100, 'sha256': '1c4e52b31bceb27241444be3a73a03f2b3cd8da6391c72904f881383e32c689e'}}
with Path('disc/battle.bin').open('rb') as f:
 for body in private_bodies.values():
  f.seek(body['offset']);assert hashlib.sha256(f.read(body['bytes'])).hexdigest()==body['sha256']
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h',
 'include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','include/battle_lift.h',
 'pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h','pc_port/extern/PsyCross/include/psx/gtereg.h',
 'pc_port/tests/battle_animation_stream_host_test.c','pc_port/tests/run_battle_animation_stream_host_test.py','pc_port/tests/run_prologue_step_slice.sh',
 'asm/battle/matchings/mainasm_3E6CC/func_800AE1BC.s','pc_port/src/battle_guest_abi_thunks.inc','pc_port/src/battle_overlay_host_leaves.inc']
pins={p:sha(p) for p in paths}
for p in paths:
 dest=root/'source-snapshot'/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(Path(p).read_bytes())
support_dir=Path('scratchpad/native-effect-reset-20261018/test-4')
gte_dir=Path('scratchpad/native-camera-update-20261018/test-4')
bridge=support_dir/'battle_bridge_map.inc';assert sha(bridge)==proof['support_pins'][str(bridge)];(root/'battle_bridge_map.inc').write_bytes(bridge.read_bytes())
common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src','-I'+str(root),'-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Wall','-Wextra','-Werror']
common+=shlex.split(os.environ.get('CPPFLAGS',''))
env=os.environ.copy();env['TMPDIR']=str(root/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0';env['XENO_CPU_EXECUTION_STATS']='1'
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
for name in ['read','write']:
 old='static int runtime_'+name+'('
 assert runtime.count(old)==1
 runtime=runtime.replace(old,'static int model_test_raw_'+name+'(')
observed=root/'observed-runtime.c';observed.write_text(runtime)
common+=['-DBATTLE_RUNTIME_SOURCE="'+str(observed)+'"']
commands=[];support_pins={str(bridge):sha(bridge)};returncodes={};toolchains={};compiler_metadata={}
overrides={name:os.environ[name] for name in ['CC','CXX','CPPFLAGS','BATTLE_ANIMATION_STREAM_TEST_LIBS'] if name in os.environ}
def compiler_info(command):
 key=shlex.join(command)
 if key not in compiler_metadata:
  executable=shutil.which(command[0],path=env.get('PATH'))
  assert executable, 'Missing compiler: '+command[0]
  executable=Path(executable).resolve()
  version=subprocess.run(command+['--version'],check=True,text=True,capture_output=True,env=env)
  compiler_metadata[key]=dict(command=command,executable=str(executable),executable_sha256=sha(executable),version=version.stdout+version.stderr)
 return key
def save_provenance():
 (root/'provenance.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,private_bodies=private_bodies,toolchain_overrides=overrides,toolchains=toolchains,compiler_metadata=compiler_metadata),indent=2)+'\n')
save_provenance()
for mode in os.environ.get('BATTLE_ANIMATION_STREAM_TEST_MODES','O0 O2 UBSan').split():
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 support=[support_dir/(mode+'.'+x+'.o') for x in ['cpu','vblank','mask']]
 support += [gte_dir/(mode+'.'+x+'.o') for x in ['INLINE_C.C','PsyX_GTE.cpp','psyq_compat.c','retail_leaf_adapters.c']]
 for p in support:assert sha(p)==proof['support_pins'][str(p)],p;support_pins[str(p)]=sha(p)
 sdk_paths=[p for p in proof['support_pins'] if p.endswith('/'+mode+'.LIBGTE.C.o')];assert len(sdk_paths)==1
 sdk=Path(sdk_paths[0])
 assert sha(sdk)==proof['support_pins'][str(sdk)];support.append(sdk);support_pins[str(sdk)]=sha(sdk)
 obj=root/(mode+'.o');exe=root/(mode+'.test')
 cc=shlex.split(os.environ.get('CC','clang' if mode=='UBSan' else 'gcc'))
 cxx=shlex.split(os.environ.get('CXX','clang++'))
 assert cc and cxx, 'CC/CXX must name a compiler'
 toolchains[mode]=dict(compile=compiler_info(cc),link=compiler_info(cxx),mode_flags=flags)
 save_provenance()
 cmd=cc+common+flags+['-UNDEBUG','-c','pc_port/tests/battle_animation_stream_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 export_names=['D_80050104','D_800595C0','D_80059308','D_8005930C','D_800500F8','D_800500FC','D_80050100','D_80059578']
 exports=['-Wl,--export-dynamic-symbol='+name for name in export_names]
 libraries=shlex.split(os.environ['BATTLE_ANIMATION_STREAM_TEST_LIBS']) if 'BATTLE_ANIMATION_STREAM_TEST_LIBS' in os.environ else ['-ldl','-lcrypto']+shlex.split(subprocess.check_output(['pkg-config','--libs','sdl2'],text=True))
 cmd=cxx+['-no-pie']+flags+['-Wl,--gc-sections']+exports+[str(obj)]+list(map(str,support))+libraries+['-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_ANIMATION_STREAM_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,private_bodies=private_bodies,support_pins=support_pins,returncodes=returncodes,commands=commands,toolchain_overrides=overrides,toolchains=toolchains,compiler_metadata=compiler_metadata,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Complete 100-byte animation-stream leaf; committed GPR/HI/LO/CP0, full GTE/memory, actual lower shared backing and overlay raw RAM, live aliases, fullword selector, wrapped address arithmetic, ordered access/failure prefixes and actual three-argument/twenty-word CallJal. Fault pipeline resumption and full game/device acceptance are not covered. Native legs require zero interpreted instructions.'),indent=2)+'\n')
print('ANIMATION STREAM HOST SUITE PASS',flush=True)
