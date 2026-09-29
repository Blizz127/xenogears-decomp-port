#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_opcode_a9_retail_test.XXXXXXXX)
export TEST_OUT
BODY_SOURCE=${SPRITE_A9_SOURCE:-src/slus_006.64/system/animation_scripts.c}
export BODY_SOURCE
printf 'SPRITE A9 artifacts: %s\n' "$TEST_OUT"

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os, shutil, struct, subprocess
out=Path(os.environ['TEST_OUT']); body=Path(os.environ['BODY_SOURCE']); image=Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
bias=0x8000f800
pins={'SLUS_006.64':{'size':len(image),'sha256':sha256(image).hexdigest()},'retail':{}}
for name,start,end,expected in [
 ('dispatcher',0x8001fbe4,0x80021ad8,'7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
 ('dispatch_table',0x800183d8,0x800185a4,'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),
 ('a9_handler',0x80021698,0x800216f4,'9c729396bad3f859aa200f99af71826f25260c7f0fd8d8668dda58519b9caf83'),
 ('scale_helper',0x80022cac,0x80022cdc,'7edc9e76b551fe6d2b564c8ba92beb0785e2426f125dcc3695d928640f42f3c0'),
 ('shared_pc_tail',0x80024ec8,0x80024f00,'79d31a7a8066fc17965df52c0f133edd6375cf9fcddf6355fe97b6e98b3330e9'),
 ('opcode_lengths',0x8004fc40,0x8004fd40,'f329de9c682e328cd9ca4718972691f3bb716046f2f57803c3a24e9b19326c0a')]:
 data=image[start-bias:end-bias]; actual=sha256(data).hexdigest(); assert actual==expected,(name,actual)
 pins['retail'][name]={'start':hex(start),'end':hex(end),'size':len(data),'sha256':actual}
entry=struct.unpack_from('<I',image,0x800183d8+(0xa9-0x8a)*4-bias)[0]
length=image[0x8004fc40+0xa9-bias]
assert entry==0x80021698 and length==2
pins['retail']['a9_entry']=hex(entry); pins['retail']['a9_length']=length
paths=[body,Path('src/slus_006.64/system/temp1.c'),Path('pc_port/port_owned_overrides.txt'),Path('pc_port/src/game_overrides.c'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/tests/sprite_opcode_a9_retail_test.c'),Path('pc_port/tests/run_sprite_opcode_a9_retail_test.sh')]
for path in paths:
 data=path.read_bytes(); pins[str(path)]=sha256(data).hexdigest(); (out/path.name).write_bytes(data)
source=Path('src/slus_006.64/system/temp1.c').read_text(); start=source.index('s32 func_80022CAC('); depth=0; end=None
ownership=Path('pc_port/port_owned_overrides.txt').read_text().splitlines()
assert sum(line.startswith('func_80022CAC|') for line in ownership)==0, 'native helper should be retired to the matching owner (temp1.c)'
for i in range(source.index('{',start),len(source)):
 if source[i]=='{': depth+=1
 elif source[i]=='}':
  depth-=1
  if depth==0: end=i+1; break
assert end is not None
helper=source[start:end]
for anchor in ('pSpriteData + 0x3A','if (factor != 0)','(s64)value * factor','value + 0x3FF','adj >> 10'):
 assert anchor in helper, ('native helper is missing audited nonstub anchor',anchor)
(out/'a9_helper.c').write_text('#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef int32_t s32; typedef int64_t s64;\n'+helper.replace('func_80022CAC(','a9_helper_impl(',1)+'\n')
pins['native_helper_owner']='src/slus_006.64/system/temp1.c'
pins['native_helper_body_sha256']=sha256(helper.encode()).hexdigest()
outer=Path('src/slus_006.64/system/temp1.c').read_text()
for anchor in ('func_8001FBE4(pData, opcode, pc + 1);',
               'pc + D_8004FC40[opcode]', 'func_800248D4(pData);'):
 assert anchor in outer, ('native shared PC tail anchor missing',anchor)
outer_start=outer.index('void func_800248D4(void* pSpriteData)')
outer_depth=0; outer_end=None
for i in range(outer.index('{',outer_start),len(outer)):
 if outer[i]=='{': outer_depth+=1
 elif outer[i]=='}':
  outer_depth-=1
  if outer_depth==0: outer_end=i+1; break
assert outer_end is not None
outer_body=outer[outer_start:outer_end]
(out/'production_func_800248D4.c').write_text(outer_body+'\n')
pins['native_outer_body_sha256']=sha256(outer_body.encode()).hexdigest()
pins['native_pc_execution']='actual temp1.c func_800248D4 source body linked and differentially executed through A9 then 86 wait'
pins['native_outer_test_link']='unrelated temp1 definitions weakened so fail-closed fixture guards own unexpected direct callees'
text=body.read_text(); marker='    case 0xA9:'
pins['production_handler_present']=text.count(marker)==1
if pins['production_handler_present']:
 a=text.index(marker); b=text.index('    case 0xAA:',a); handler=text[a:b]
 (out/'production_a9_handler.txt').write_text(handler)
 pins['production_handler_sha256']=sha256(handler.encode()).hexdigest()
pins['scope']='4608 full retail-dispatch cases; actual retail/native scale helper; shared-tail stride; five handler mutants; no rendering/gameplay claim.'
pins['compilers']={}
for compiler in ('gcc','clang'):
 executable=Path(shutil.which(compiler) or '').resolve()
 assert executable.is_file(), compiler+' missing'
 pins['compilers'][compiler]={'path':str(executable),
   'sha256':sha256(executable.read_bytes()).hexdigest(),
   'version':subprocess.check_output([compiler,'--version'],text=True).splitlines()[0]}
(out/'provenance.json').write_text(json.dumps(pins,indent=2,sort_keys=True)+'\n')
if not pins['production_handler_present']:
 print('SPRITE A9 SEMANTIC RED: production handler absent')
 raise SystemExit(1)
PY

build_case() {
    local name=$1 opt=$2 source=${3:-$BODY_SOURCE}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
        -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h
        -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    local driver=gcc link_driver=clang
    if [ "$opt" = UBSan ]; then
        set +e
        clang -std=gnu17 "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/src \
            -c "$source" -o "$TEST_OUT/$name.clang-body.o" \
            >"$TEST_OUT/$name.clang-build.log" 2>&1
        local clang_status=$?
        set -e
        if [ "$clang_status" -eq 0 ]; then
            driver=clang
            link_driver=clang
            mv "$TEST_OUT/$name.clang-body.o" "$TEST_OUT/$name.body.o"
            printf 'all-Clang UBSan\n' >"$TEST_OUT/$name.compiler-mode.txt"
        else
            rg -q "undeclared function 'func_8001F6B0'" "$TEST_OUT/$name.clang-build.log"
            rg -q "undeclared function 'SpriteComputeTransformMatrix'" "$TEST_OUT/$name.clang-build.log"
            rg -q "undeclared function 'ScaleMatrixL'" "$TEST_OUT/$name.clang-build.log"
            printf 'mixed UBSan: GCC production/helper objects plus Clang fixture/link; full production TU is Clang-incompatible; see %s\n' \
                "$name.clang-build.log" >"$TEST_OUT/$name.compiler-mode.txt"
        fi
    fi
    if [ ! -f "$TEST_OUT/$name.body.o" ]; then
        "$driver" -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Ipc_port/src -c "$source" -o "$TEST_OUT/$name.body.o" \
        >"$TEST_OUT/$name.build.log" 2>&1
    else
        : >"$TEST_OUT/$name.build.log"
    fi
    "$driver" -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/src \
        -c "$TEST_OUT/a9_helper.c" -o "$TEST_OUT/$name.helper.o" \
        >>"$TEST_OUT/$name.build.log" 2>&1
    "$driver" -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -DUSE_EXTENDED_PRIM_POINTERS=0 -Ipc_port/src \
        -c src/slus_006.64/system/temp1.c -o "$TEST_OUT/$name.outer.o" \
        >>"$TEST_OUT/$name.build.log" 2>&1
    for symbol in AnimScriptTick func_80022CAC func_80022CDC func_80022D44 func_80023124 func_80023290 func_800245D8; do
        nm --defined-only "$TEST_OUT/$name.outer.o" | rg -q " [TtWw] $symbol$"
        objcopy --weaken-symbol="$symbol" "$TEST_OUT/$name.outer.o"
    done
    # temp1.c gained a C body for func_80023B84 (the fixture supplies its own
    # double); weaken it too when present so the fixture's definition wins.
    objcopy --weaken-symbol=func_80023B84 "$TEST_OUT/$name.outer.o"
    "$link_driver" -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" \
        -Wl,--gc-sections pc_port/tests/sprite_opcode_a9_retail_test.c \
        pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" \
        "$TEST_OUT/$name.helper.o" "$TEST_OUT/$name.outer.o" \
        -o "$TEST_OUT/$name.test" \
        >>"$TEST_OUT/$name.build.log" 2>&1
    nm -S "$TEST_OUT/$name.helper.o" | rg ' [Tt] a9_helper_impl$' \
        >"$TEST_OUT/$name.helper-symbol.txt"
}

verify_source_pins() {
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins=json.loads((Path(os.environ['TEST_OUT'])/'provenance.json').read_text())
for path,expected in pins.items():
 if '/' in path and isinstance(expected,str):
  assert sha256(Path(path).read_bytes()).hexdigest()==expected,'source changed during test: '+path
PY
}

for opt in O0 O2 UBSan; do
    build_case "$opt" "$opt"
    "$TEST_OUT/$opt.test" >"$TEST_OUT/$opt.log" 2>&1
    cat "$TEST_OUT/$opt.log"
done
verify_source_pins

python3 - <<'PY'
from pathlib import Path
import os
out=Path(os.environ['TEST_OUT']); source=Path(os.environ['BODY_SOURCE']).read_text()
a=source.index('    case 0xA9:'); b=source.index('    case 0xAA:',a); body=source[a:b]
mutations={
 'unsigned_operand':('(s8)((u8*)operands)[0]','(u8)((u8*)operands)[0]'),
 'missing_q12_round':('        if ((s32)product < 0) product += 0xFFFu;\n',''),
 'wrong_mirror_bit':('if ((flags >> 2) & 1u)','if ((flags >> 3) & 1u)'),
 'wrong_position_word':('AnimationWrite32(p, AnimationRead32(p) + delta);','AnimationWrite32(p + 4, AnimationRead32(p + 4) + delta);'),
 'wrong_helper_input':('func_80022CAC(p, (s32)product >> 12)','func_80022CAC(p, ((s32)product >> 12) + 1)'),
}
for name,(old,new) in mutations.items():
 if body.count(old)!=1: raise SystemExit(f'mutation anchor {name} count={body.count(old)}')
 changed=body.replace(old,new,1)
 (out/f'{name}.c').write_text(source[:a]+changed+source[b:])
PY

for kind in unsigned_operand missing_q12_round wrong_mirror_bit wrong_position_word wrong_helper_input; do
    build_case "control_$kind" O2 "$TEST_OUT/$kind.c"
    status=0
    "$TEST_OUT/control_$kind.test" >"$TEST_OUT/control_$kind.log" 2>&1 || status=$?
    if [ "$status" -ne 1 ] || ! rg -q '^SPRITE A9 (DIFFERENTIAL|RETAIL PC ADVANCE|NATIVE OUTER) FAIL ' "$TEST_OUT/control_$kind.log"; then
        cat "$TEST_OUT/control_$kind.log" >&2
        printf 'SPRITE A9 CONTROL FAILURE %s status=%s\n' "$kind" "$status" >&2
        exit 1
    fi
    printf 'SPRITE A9 control rejected %s: %s\n' "$kind" \
        "$(rg -m1 '^SPRITE A9 (DIFFERENTIAL|RETAIL PC ADVANCE|NATIVE OUTER) FAIL ' "$TEST_OUT/control_$kind.log")"
done
verify_source_pins
printf 'SPRITE A9 GREEN: O0/O2/UBSan 4608 cases, raw/native outer PC gate, actual scale helper, five controls; %s\n' "$TEST_OUT"
