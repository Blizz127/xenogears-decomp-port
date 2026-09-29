#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
python3 - <<'PY'
import re,pathlib,hashlib
L=pathlib.Path('asm/world_map/120C.s');s=L.read_text() if L.exists() else '\n'.join(f.read_text() for f in sorted(pathlib.Path('asm/world_map').rglob('*.s')) if '/data/' not in str(f));d=pathlib.Path('disc/world_map.bin').read_bytes()
for name in '8008A52C 8008B498 8008BD1C 8008C6EC 8008D520 8008DE9C 8008E4F4 800907C4 80087F60 8008868C 800879E0 80087904 80088C90'.split():
 text=s[s.index('glabel func_'+name):s.index('.size func_'+name)]
 rows=[(int(a,16),bytes.fromhex(b)) for a,b in re.findall(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',text)]
 assert [a for a,b in rows]==list(range(int(name,16),rows[-1][0]+4,4))
 data=b''.join(b for a,b in rows);off=int(name,16)-0x8006faf0
 assert data==d[off:off+len(data)]
 print(name,len(data),hashlib.sha256(data).hexdigest())
PY
for opt in '-O0' '-O2' '-O2 -fsanitize=undefined -fno-sanitize-recover=all'; do
 gcc -std=gnu17 -Wno-builtin-declaration-mismatch -DXENO_PC_PORT -include assert.h -Iinclude -Ipc_port/src -ffunction-sections -fdata-sections -fno-pie -no-pie $opt pc_port/tests/world_resume_initializers_test.c pc_port/tests/world_resume_initializer_bindings.c pc_port/src/world_map_resume_initializers.c pc_port/src/psx_memory.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$build/test"
 "$build/test"
done
# Negative controls: each plausible regression must be rejected by the same
# production-linked disc differential, including omission of a scheduler binding.
python3 - "$build" <<'PY'
import pathlib,subprocess,sys
root=pathlib.Path.cwd();tmp=pathlib.Path(sys.argv[1])
source=(root/'pc_port/src/world_map_resume_initializers.c').read_text()
scheduler=(root/'pc_port/src/world_map_scheduler.c').read_text()
mutations=[
 ('lost-saved-position','    U32(slot + 0x4Cu) =','    U32(slot + 0x28u) = 0;\n    U32(slot + 0x4Cu) ='),
 ('wrong-sprite-scale','SpriteSetScale(object, 0x1800)','SpriteSetScale(object, 0x1000)'),
 ('sprite-flag-not-cleared','&= ~4u','&= ~0u'),
 ('absent-party-created','U8(0x8006F369u) == 0xFF','U8(0x8006F369u) == 0xFE'),
 ('channel-mode-boundary','mode >= 4u && mode < 8u','mode >= 4u && mode < 7u'),
 ('model-packet-transparency','command | 2u','command'),
 ('missing-resume-binding','wm_8008A52C != 0','wm_8008A52C == 0'),
]
for name,old,new in mutations:
 is_binding=name=='missing-resume-binding'
 text=scheduler if is_binding else source
 assert text.count(old)==1,(name,text.count(old))
 impl=tmp/'mutant.c';impl.write_text(source if is_binding else text.replace(old,new))
 sched=tmp/'scheduler.c';sched.write_text(text.replace(old,new) if is_binding else scheduler)
 binding=tmp/'bindings.c';binding.write_text((root/'pc_port/tests/world_resume_initializer_bindings.c').read_text().replace('../src/world_map_scheduler.c',str(sched)))
 cmd=['gcc','-std=gnu17','-Wno-builtin-declaration-mismatch','-DXENO_PC_PORT','-include','assert.h','-Iinclude','-Ipc_port/src','-ffunction-sections','-fdata-sections','-fno-pie','-no-pie','-O2','pc_port/tests/world_resume_initializers_test.c',str(binding),str(impl),'pc_port/src/psx_memory.c','pc_port/src/battle_mips_adapter.c','-Wl,--gc-sections','-o',str(tmp/'mutant')]
 subprocess.run(cmd,check=True)
 result=subprocess.run([str(tmp/'mutant')],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 assert result.returncode!=0,'SURVIVED: '+name
 print('negative control detected:',name)
PY
