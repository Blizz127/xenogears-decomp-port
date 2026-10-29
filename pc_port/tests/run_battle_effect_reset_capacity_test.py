"""Supplement the effect-reset proof with larger real-heap pool capacities.

Reuse pinned support objects from the full suite; production sources remain
unchanged. The generated fixture changes only pool counts after common setup.
"""
import hashlib,json,os,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2]
os.chdir(repo)
root=Path(os.environ['BATTLE_EFFECT_CAPACITY_OUT']).resolve();root.mkdir(exist_ok=False)
suite=Path(os.environ['BATTLE_EFFECT_SUITE']).resolve()
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
proof=json.loads((suite/'provenance.json').read_text());pins=proof['source_pins'].copy()
pins[str(Path(__file__).resolve().relative_to(repo))]=sha(__file__)
for p,d in pins.items():assert sha(p)==d,p
source=Path('pc_port/tests/battle_effect_reset_host_test.c').read_text()
source=source[:source.index(' if(argc==2) return effect_controls() ? 0:1;')]
source=source.replace('static void effect_setup(PcPortMipsCpu *cpu)','static unsigned capacity_small,capacity_wide;\nstatic void effect_setup_base(PcPortMipsCpu *cpu)',1)
wrapper='''static void effect_setup(PcPortMipsCpu *cpu)
{
 effect_setup_base(cpu);
 write_at(0x80140348u,2,capacity_small);
 write_at(0x8014034au,2,capacity_wide);
}

'''
source=source.replace('static int effect_case(void)',wrapper+'static int effect_case(void)',1)
source+=''' (void)effect_sdk_cases;(void)effect_controls;
 effect_index=0;real_heap=1;model_children=model_initializers=alias_case=perturb=0;
 const unsigned small[]={16,128,256},wide[]={16,32,64},variants[]={0,2,4};
 for(unsigned i=0;i<3;i++) {
  capacity_small=small[i];capacity_wide=wide[i];variant=variants[i];
  if(!effect_case()) return 1;
  if(heap_calls!=2 || user_calls!=2 || packet_calls!=2*(capacity_wide+1)) return 2;
  printf("BATTLE EFFECT CAPACITY PASS small=%u wide=%u packets=%u native steps=0\\n",capacity_small,capacity_wide,packet_calls);
 }
 return 0;
}
'''
fixture=root/'capacity.c';fixture.write_text(source)
common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src','-Ipc_port/tests','-I'+str(suite),'-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Wall','-Wextra','-Werror']
env=os.environ.copy();env['TMPDIR']=str(root/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0'
commands=[];support_pins={};results={}
for mode in ['O0','O2','UBSan']:
 flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
 obj=root/(mode+'.o');exe=root/(mode+'.test');compiler='clang' if mode=='UBSan' else 'gcc'
 cmd=[compiler]+common+flags+['-c',str(fixture),'-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 support=[suite/(mode+'.'+x+'.o') for x in ['compat','cpu','vblank','mask','heap','gpu']]
 support_pins.update({str(p.relative_to(repo)):sha(p) for p in support})
 cmd=['clang++','-no-pie']+flags+['-Wl,--gc-sections',str(obj)]+list(map(str,support))+['-ldl','-lcrypto','-o',str(exe)]
 commands.append(cmd);subprocess.run(cmd,check=True,env=env)
 cmd=[str(exe)];commands.append(cmd)
 log=root/(mode+'.log')
 with log.open('w') as f:rc=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,env=env).returncode
 assert rc==0,(mode,rc,log.read_text());results[mode]=rc
 print(mode,'capacity proof PASS',flush=True)
for p,d in pins.items():assert sha(p)==d,p
for p,d in support_pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=proof['private_inputs'],support_pins=support_pins,commands=commands,returncodes=results,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='Three additional full-reset comparisons per configuration with actual native heap and SDK: small/wide counts 16/16, 128/32, 256/64. Normal, scratchpad and KSEG stack domains; third case requests consolidation. These are explicit capacity cases, not a claim to have sampled exact production counts.'),indent=2)+'\n')
