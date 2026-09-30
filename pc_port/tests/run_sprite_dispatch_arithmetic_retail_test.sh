#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_arithmetic_retail_test
mkdir -p "$OUT"
python3 - <<'CHECK'
from hashlib import sha256
from pathlib import Path
assert sha256(Path('disc/SLUS_006.64').read_bytes()).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
CHECK
for opt in O0 O2 UBSan; do
 flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc -std=gnu17 -fpermissive "${flags[@]}" -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 # Borrow dependency guards: any unexpected helper call aborts the fixture.
 gcc -std=gnu17 "${flags[@]}" -Dmain=noop_main -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c -o "$OUT/$opt.guards.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${flags[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_arithmetic_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
echo 'SPRITE ARITH O0/O2/UBSAN PASS'


python3 - <<'MUTANTS'
from pathlib import Path
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
mutations = {
    'zero_divisor': ('value = b ? a / b : 0xFFFFFFFFu;', 'value = b ? a / b : 0u;'),
    'unsigned_immediate_divisor': ('(u32)((s32)value / operand)', '(u32)((s32)value / (u8)operand)'),
    'shift_mask': ('value <<= (u32)operand & 31u;', 'value <<= (u32)operand & 7u;'),
    'unsigned_byte_shift': ('(s32)(s8)value >>', '(s32)(u8)value >>'),
    'signed_word_shift': ('case 0xDC: value >>= (u32)operand & 31u;', 'case 0xDC: value = (u32)((s32)(s16)value >> ((u32)operand & 31u));'),
    'invented_alias_subtract': ('else value = a + b;', 'else value = a - b;'),
}
for name, (old, new) in mutations.items():
    assert source.count(old) == 1, (name, source.count(old))
    Path('pc_port/build_native/sprite_dispatch_arithmetic_retail_test', name + '.c').write_text(source.replace(old, new))
MUTANTS
for mutant in zero_divisor unsigned_immediate_divisor shift_mask unsigned_byte_shift signed_word_shift invented_alias_subtract; do
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie -O2 -Wl,--gc-sections pc_port/tests/sprite_dispatch_arithmetic_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" "$OUT/O2.guards.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE ARITH mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE ARITH FAIL case=' "$OUT/$mutant.log"
done
echo 'SPRITE ARITH negative controls PASS: zero divisor, signed divisor, shift mask, byte sign, word sign, table aliases'
