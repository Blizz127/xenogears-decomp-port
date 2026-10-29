"""Exercise meaningful sixth descriptors without changing the full-suite fixture."""
import hashlib,json,os,subprocess
from pathlib import Path
repo=Path(__file__).resolve().parents[2];os.chdir(repo)
root=Path(os.environ['BATTLE_RESOURCE_SIXTH_OUT']).resolve();root.mkdir(exist_ok=False)
suite=Path(os.environ['BATTLE_RESOURCE_POSITIVE_SUITE']).resolve()
proof=json.loads((suite/'receipt.json').read_text())
def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
for name in ['source_pins','private_inputs','support_pins','artifact_pins']:
    for p,d in proof[name].items():assert sha(p)==d,p
source=Path('pc_port/tests/battle_resource_init_host_test.c').read_text()
a=source.index('static const uint16_t descriptors[][6]={');b=source.index('static void setup',a)
source=source[:a]+'static const uint16_t descriptors[][6]={{0,0,0,0,0,1},{0,0,0,0,0,2},{0,0,0,0,0,3},{0,0,0,0,0,5},{0,0,0,0,0,7}};\n'+source[b:]
source=source[:source.index('int main(int argc,char **argv)')]+'''
int main(void)
{
 (void)unpin_cases;(void)resource_abi_cases;(void)copy_abi_cases;(void)controls;
 FILE *f=fopen("scratchpad/native-module-20261017/module-test-2/module.bin","rb");
 assert(f&&fread(module,1,sizeof(module),f)==sizeof(module));fclose(f);
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 unsigned total=0;
 for(service_mode=0;service_mode<2;service_mode++)for(variant=0;variant<20;variant++){
  descriptor_case=variant/4;early=allocation_fail=perturb=0;
  if(!run_case())return 1;
  total++;
 }
 printf("RESOURCE INIT PASS %u isolated sixth-descriptor comparisons; types 1/2/3/5/7, four domains, two service modes; native steps=0\\n",total);
 return failed?1:0;
}
'''
fixture=root/'fixture.c';fixture.write_text(source)
(root/'battle_bridge_map.inc').write_bytes((suite/'battle_bridge_map.inc').read_bytes())
pins=dict(proof['source_pins']);pins[str(Path(__file__).relative_to(repo))]=sha(__file__)
env=os.environ.copy();env['TMPDIR']=str(root/'compiler-tmp');Path(env['TMPDIR']).mkdir();env['UBSAN_OPTIONS']='symbolize=0'
commands=[];returncodes={}
for index,mode in enumerate(['O0','O2','UBSan']):
    compile=proof['commands'][index*3].copy();link=proof['commands'][index*3+1].copy()
    obj=root/(mode+'.o');exe=root/(mode+'.test')
    compile[compile.index('-c')+1]=str(fixture);compile[compile.index('-o')+1]=str(obj)
    compile.insert(1,'-DBATTLE_RUNTIME_SOURCE="'+str(repo/'pc_port/src/battle_mips_runtime.c')+'"')
    compile=[('-I'+str(root)) if v=='-I'+str(suite) else v for v in compile]
    link=[str(obj) if v==str(suite/(mode+'.o')) else v for v in link];link[link.index('-o')+1]=str(exe)
    commands.extend([compile,link,[str(exe)]])
    subprocess.run(compile,check=True,env=env);subprocess.run(link,check=True,env=env)
    with (root/(mode+'.log')).open('w') as log:rc=subprocess.run([str(exe)],stdout=log,stderr=subprocess.STDOUT,env=env).returncode
    returncodes[mode]=rc;assert rc==0,(mode,rc)
    print(mode,'sixth descriptor PASS',flush=True)
for p,d in pins.items():assert sha(p)==d,p
(root/'receipt.json').write_text(json.dumps(dict(source_pins=pins,private_inputs=proof['private_inputs'],support_pins=proof['support_pins'],returncodes=returncodes,commands=commands,artifact_pins={str(p.relative_to(repo)):sha(p) for p in root.iterdir() if p.is_file()},scope='40 additional full original-caller comparisons per configuration; only descriptor six is active. Types 1/2/3/5 exercise observable final-slot effects; type 7 retains its original index exclusion. Native legs execute zero instructions; external bodies remain modeled.'),indent=2)+'\n')
