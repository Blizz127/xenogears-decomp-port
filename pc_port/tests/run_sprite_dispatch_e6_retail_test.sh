#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_e6_retail_test
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
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${flags[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_e6_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
echo 'SPRITE E6 O0/O2/UBSAN PASS'

python3 - <<'MUTANTS'
from pathlib import Path
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
mutations = {
    'omit_high_clear': ('slot[1] = 0;', '(void)0;'),
    'late_operand_read': ('u8 value = ((u8*)operands)[1];\n        slot[1] = 0;',
                          'u8 value;\n        slot[1] = 0;\n        value = ((u8*)operands)[1];'),
    'wrong_high_byte': ('slot[1] = 0;', 'slot[1] = 1;'),
}
for name, (old, new) in mutations.items():
    assert source.count(old) == 1
    Path('pc_port/build_native/sprite_dispatch_e6_retail_test', name + '.c').write_text(source.replace(old, new))
MUTANTS
for mutant in omit_high_clear late_operand_read wrong_high_byte; do
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie -O2 -Wl,--gc-sections pc_port/tests/sprite_dispatch_e6_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" "$OUT/O2.guards.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE E6 mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE E6 FAIL case=' "$OUT/$mutant.log"
done
echo 'SPRITE E6 negative controls PASS: omit_high_clear late_operand_read wrong_high_byte'
