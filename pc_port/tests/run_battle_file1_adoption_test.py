#!/usr/bin/env python3
"""Actual production bridge/native-binding versus pinned 315-instruction retail body.

The existing 463-case corpus, raw oracle bus/11 helper stubs and coverage
counters are extracted mechanically. Native execution substitutes only the
five guest helper boundaries, through the production service's nested CPU;
the six resident helpers use the production bridge and explicit host spies.
"""
from pathlib import Path
import argparse, hashlib, json, os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]

def run(cmd,log):
 p=subprocess.run(cmd,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 log.write_text(p.stdout)
 if p.returncode: raise RuntimeError(f'{cmd[0]} failed ({p.returncode}): {log}\n{p.stdout[-4000:]}')
 return p.stdout

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path);ap.add_argument('--runtime',type=Path,default=ROOT/'pc_port/src/battle_mips_runtime.c');ap.add_argument('--modes',nargs='+',default=['O0','O2','UBSan']);a=ap.parse_args()
 out=(a.out or Path(tempfile.mkdtemp(prefix='xeno-file1-adoption.'))).resolve();out.mkdir(parents=True,exist_ok=True)
 src=(ROOT/'pc_port/tests/battle_command_file1_controller_retail_test.c').read_text()
 payload=ROOT/'disc/battle_command_file1.bin'
 assert hashlib.sha256(payload.read_bytes()).hexdigest()=='64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670'
 # Reuse the original oracle code/corpus, never regenerate the retail body.
 oracle=src[src.index('enum {'):src.index('uint8_t g_PsxRam[')]
 oracle+=src[src.index('static Trace *trace;'):src.index('static uint8_t *ui(void)')].replace('static int native_mode;\n','')
 oracle+='static uint8_t *ui(void) { return guest(r32(guest(0x800d2d28))); }\nstatic uint8_t *win(void) { return guest(r32(guest(0x800d2dac))); }\n'
 oracle+=src[src.index('static void ev('):src.index('void func_8008F8F4(')]
 oracle+=src[src.index('static int bus_read('):src.index('static void init_common(')]
 init=src[src.index('static void init_common('):src.index('static int compare_state(')]
 init=init.replace('const Case *c,int native','const Case *c').replace('    native_mode=native; audit_yields_left=AUDIT_YIELDS;','    audit_yields_left=AUDIT_YIELDS;')
 start=init.index('    if(native) {');end=init.index('        memset(g_PsxRam,0xa5',start)
 init=init[:start]+init[end:];init=init.replace('    }\n    w16(ctl','    w16(ctl',1).replace('w=native?host_alt_window:guest(ALT_WINDOW_ADDR);','w=guest(ALT_WINDOW_ADDR);')
 oracle+=init
 (out/'oracle.inc').write_text(oracle)
 corpus=src[src.index('int main(int argc, char **argv)'):]
 corpus=corpus.replace('    Case tests[]={','    if (!adoption_gate_tests()) return 1;\n    Case tests[]={',1)
 corpus=corpus.replace('    printf("candidate differential GREEN cases=%u\\n",count);','    if (!adoption_extra_cases()) return 1;\n    printf("FILE1 ADOPTION PASS cases=%u gates=%u adopted=%u\\n",count,gate_checks,adopted_calls);')
 (out/'corpus.inc').write_text(corpus)
 paths=[a.runtime,ROOT/'pc_port/src/battle_file1_controller.inc',ROOT/'pc_port/src/battle_file1_controller.h',ROOT/'pc_port/src/battle_mips_runtime_internal.h',ROOT/'pc_port/src/battle_mips_adapter.c',ROOT/'src/battle_command_file1/message_controller_impl.inc',ROOT/'src/battle_command_file1/message_controller_bindings.inc',ROOT/'src/battle_command_file1/xeno_battle_command_file1_controller.h',ROOT/'pc_port/tests/battle_file1_adoption_test.c',Path(__file__),ROOT/'pc_port/tests/battle_command_file1_controller_retail_test.c']
 pins={str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
 (out/'pins.json').write_text(json.dumps(pins,indent=2)+'\n')
 elf=['--elf','build/out/slus_006.64.elf'] if (ROOT/'build/out/slus_006.64.elf').exists() else []
 run(['python3','tools/scripts/gen_battle_bridge_map.py',*elf,'--symbols','config/symbol_addrs.slus_006.64.txt','--symbols','linker/undefined_funcs_auto.battle.txt','--symbols','linker/undefined_syms_auto.battle.txt','--symbols','config/symbol_addrs.battle.txt','--out',str(out/'battle_bridge_map.inc')],out/'map.log')
 common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-DAUDIT_YIELDS=3','-include','assert.h','-ffunction-sections','-fdata-sections','-Ipc_port/include_shim','-Iinclude','-Ipc_port/src',f'-I{out}','-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Isrc/battle_command_file1']
 for mode in a.modes:
  flags=['-O1','-fsanitize=undefined','-fno-sanitize-recover=all'] if mode=='UBSan' else ['-'+mode]
  for name,source,warnings in [('compat','pc_port/src/psyq_compat.c',['-w']),('test','pc_port/tests/battle_file1_adoption_test.c',['-Wall','-Wextra','-Werror']),('cpu','pc_port/src/battle_mips_adapter.c',['-Wall','-Wextra','-Werror'])]:
   run(['clang',*common,*flags,*warnings,f'-DBATTLE_RUNTIME_SOURCE="{a.runtime.resolve()}"','-c',source,'-o',str(out/f'{mode}.{name}.o')],out/f'{mode}.{name}.build.log')
  run(['clang','-no-pie',*flags,'-Wl,--gc-sections',*[str(out/f'{mode}.{n}.o') for n in ['compat','test','cpu']],'-ldl','-o',str(out/mode)],out/f'{mode}.link.log')
  log=run([str(out/mode),str(payload)],out/f'{mode}.log')
  assert 'FILE1 ADOPTION PASS cases=463 ' in log,log[-2000:]
  pc=[l.split() for l in log.splitlines() if l.startswith('PC ')]
  assert len(pc)==315 and all(int(r[2])>0 for r in pc)
  branches=[r for r in pc if int(r[3])];assert len(branches)==21 and all(int(r[3])==3 for r in branches)
  print(mode, next(l for l in log.splitlines() if l.startswith('FILE1 ADOPTION PASS')), 'retail315/branches21')
 # Compiled semantic controls must fail an explicit assertion, not merely exit.
 adapter=(ROOT/'pc_port/src/battle_file1_controller.inc').read_text()
 runtime=a.runtime.read_text()
 edits={
  'wrong-archive':('runtime->file1.archive20_selected && archive == 3087','archive == 3087'),
  'blind-full-payload':('runtime->file1.verified = file1_hash_matches(FILE1_BYTES,\n            "64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670");','runtime->file1.verified = 1;'),
  'keep-stale-authority':('runtime->file1.verified = 0;','runtime->file1.verified = runtime->file1.verified;'),
  'skip-immutable-check':('if (!file1_hash_matches(FILE1_IMMUTABLE_BYTES,','if (0 && !file1_hash_matches(FILE1_IMMUTABLE_BYTES,'),
  'mask-bad-pointer':('    uintptr_t ram = (uintptr_t)g_PsxRam;','    if (address >= 0x80200000u && address < 0x80800000u) address = 0x80000000u | (address & 0x1fffffu);\n    uintptr_t ram = (uintptr_t)g_PsxRam;'),
  'skip-helper-generation-check':('    if (file1_call->generation != file1_call->runtime->file1.generation ||\n        !file1_identity_current(file1_call->runtime))','    if (0)'),
  'wrong-resident-frame':('cpu.gpr[29] = sp - 0x50u;','cpu.gpr[29] = sp - 0x48u;'),
  'wrong-constructor-stack':('cpu.gpr[29]+0x10u+(i-4u)*4u','cpu.gpr[29]+0x14u+(i-4u)*4u'),
  'constant-defaults':('#define xeno_battle_command_file1_defaults_9C10 ((uint16_t *)file1_checked(0x801e9c10u,10,2))','#define xeno_battle_command_file1_defaults_9C10 ((uint16_t[]){0x7fff,0x7fff,0x10,8,0x1f0})'),
  'stale-window-binding':('#define D_800D2DAC file1_global_pointer(0x800d2dacu,0x98,4)','#define D_800D2DAC ((uint8_t *)file1_checked(0x80052000u,0x98,4))'),
 }
 manifest={}
 for name,(old,new) in edits.items():
  assert adapter.count(old)==1,(name,adapter.count(old))
  mutant=adapter.replace(old,new,1)
  mutant=mutant.replace('#include "../../src/battle_command_file1/message_controller_impl.inc"',f'#include "{ROOT}/src/battle_command_file1/message_controller_impl.inc"')
  target=out/(name+'.inc');target.write_text(mutant)
  rs=runtime.replace('#include "battle_file1_controller.inc"',f'#include "{target}"')
  rpath=out/(name+'.c');rpath.write_text(rs)
  manifest[name]={'adapter_sha256':hashlib.sha256(mutant.encode()).hexdigest(),'runtime_sha256':hashlib.sha256(rs.encode()).hexdigest()}
  run(['clang',*common,'-O2','-Wall','-Wextra','-Werror',f'-DBATTLE_RUNTIME_SOURCE="{rpath}"','-c','pc_port/tests/battle_file1_adoption_test.c','-o',str(out/(name+'.o'))],out/(name+'.build.log'))
  # Compile ordinary O2 link dependencies even for a single requested mode.
  for dep,source in [('compat','pc_port/src/psyq_compat.c'),('cpu','pc_port/src/battle_mips_adapter.c')]:
   if not (out/f'O2.{dep}.o').exists():run(['clang',*common,'-O2','-w','-c',source,'-o',str(out/f'O2.{dep}.o')],out/f'O2.{dep}.build.log')
  run(['clang','-no-pie','-O2','-Wl,--gc-sections',str(out/(name+'.o')),str(out/'O2.compat.o'),str(out/'O2.cpu.o'),'-ldl','-o',str(out/name)],out/(name+'.link.log'))
  result=subprocess.run([str(out/name),str(payload)],cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
  (out/(name+'.log')).write_text(result.stdout)
  assert result.returncode==1 and 'FILE1 ADOPTION FAIL ' in result.stdout,(name,result.returncode,result.stdout[-2000:])
  print('control rejected',name)
 (out/'negative-controls.json').write_text(json.dumps(manifest,indent=2)+'\n')
 for p,h in pins.items(): assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h,p
 print('FILE1 ADOPTION source pins unchanged; artifacts',out)
if __name__=='__main__':main()
