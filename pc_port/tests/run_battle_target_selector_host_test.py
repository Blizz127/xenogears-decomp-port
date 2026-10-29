"""Complete private target-selector bodies, with pinned existing support objects."""
import hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_TARGET_SELECTOR_TEST_OUT']).resolve();root.mkdir(exist_ok=False)
base=Path('scratchpad/native-motion-track-20261019')
proof=json.loads((base/'positive-suite.json').read_text())
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
private={'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291','disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in private.items():assert sha(p)==d,p
private_bodies={'parent':dict(offset=0x3f948,bytes=224,sha256='d886f9d31b12dc1934b50bcaeb611d2358a2a3550b205432bb9a9a48771601c8'),'child':dict(offset=0x3f910,bytes=56,sha256='d8f1f9a0bfa426fc0272bb8d71bde1fa7c5313bf132d67c5e19118e10095d065')}
with Path('disc/battle.bin').open('rb') as f:
 for body in private_bodies.values():
  f.seek(body['offset']);assert hashlib.sha256(f.read(body['bytes'])).hexdigest()==body['sha256']
paths=list(map(str,Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/battle_mips_adapter.h','pc_port/src/battle_guest_resolver.h',
 'include/types.h','include/system/graphics.h','include/system/memory.h','include/system/sound.h','include/battle_lift.h',
 'pc_port/src/retail_data.h','pc_port/src/psx_memory.c','pc_port/src/psx_memory.h','pc_port/extern/PsyCross/include/psx/gtereg.h',
 'pc_port/tests/battle_target_selector_host_test.c','pc_port/tests/run_battle_target_selector_host_test.py','pc_port/tests/run_prologue_step_slice.sh',
 'asm/battle/nonmatchings/main73_p2/func_800AF438.s','asm/battle/matchings/mainasm_3F910/func_800AF400.s','pc_port/src/battle_guest_abi_thunks.inc','pc_port/src/battle_overlay_host_leaves.inc']
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
overrides={name:os.environ[name] for name in ['CC','CXX','CPPFLAGS','BATTLE_TARGET_SELECTOR_TEST_LIBS'] if name in os.environ}
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
for mode in os.environ.get('BATTLE_TARGET_SELECTOR_TEST_MODES','O0 O2 UBSan').split():
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
 cmd=cc+common+flags+['-UNDEBUG','-c','pc_port/tests/battle_target_selector_host_test.c','-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 export_names=['D_80050104','D_800595C0','D_80059308','D_8005930C','D_800500F8','D_800500FC','D_80050100','D_80059578']
 exports=['-Wl,--export-dynamic-symbol='+name for name in export_names]
 libraries=shlex.split(os.environ['BATTLE_TARGET_SELECTOR_TEST_LIBS']) if 'BATTLE_TARGET_SELECTOR_TEST_LIBS' in os.environ else ['-ldl','-lcrypto']+shlex.split(subprocess.check_output(['pkg-config','--libs','sdl2'],text=True))
 cmd=cxx+['-no-pie']+flags+['-Wl,--gc-sections']+exports+[str(obj)]+list(map(str,support))+libraries+['-o',str(exe)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)]+os.environ.get('BATTLE_TARGET_SELECTOR_TEST_ARGS','').split();commands.append(cmd)
 with (root/(mode+'.log')).open('w') as log:rc=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env).returncode
 returncodes[mode]=rc
 print(mode,'status',rc,flush=True)
 if rc:print((root/(mode+'.log')).read_text(),flush=True);raise SystemExit(rc)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=private,private_bodies=private_bodies,support_pins=support_pins,returncodes=returncodes,commands=commands,toolchain_overrides=overrides,toolchains=toolchains,compiler_metadata=compiler_metadata,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Complete224-byte target-selector and56-byte first-bit bodies; committed GPR/HI/LO/CP0, full GTE/memory, lower shared actor/output backing, overlay globals remaining in raw RAM, ordered accesses/failures and real CallJal interface. Fault pipeline resumption is not covered. Original instruction execution is test-only, native legs require zero interpreted instructions. Full game and owner-device acceptance remain incomplete.'),indent=2)+'\n')
print('TARGET SELECTOR HOST SUITE PASS',flush=True)
