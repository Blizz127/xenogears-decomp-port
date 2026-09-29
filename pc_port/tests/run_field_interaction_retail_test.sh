#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${INTERACTION_OUT:-$(mktemp -d pc_port/build_native/interaction.XXXXXXXX)}
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
import hashlib,sys
out=Path(sys.argv[1]);b=Path('disc/field.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
assert hashlib.sha256(b[0x13eac:0x14668]).hexdigest()=='973c768052da8e2c0630598c325452807cbc3c5ad221ce34dac9ec701a7afdd3'
s=Path('src/field/main/misc8.c').read_text();a=s.index('void func_8008399C(s32 actorIndex, void* pFieldActor, void* pActorData) {');z=s.index('\n}',a)+2;(out/'body.c').write_text(s[a:z]+'\n')
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt");if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang -std=gnu17 -fno-pie -no-pie "${flags[@]}" -Ipc_port/src "-DINTERACTION_BODY=\"$PWD/$OUT/body.c\"" pc_port/tests/field_interaction_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt"
 "$OUT/$opt" disc/field.bin
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys,json,hashlib
out=Path(sys.argv[1]);s=(out/'body.c').read_text()
controls=[
 ('inner-confirm','distance < squaredLimits[2]','distance < squaredLimits[0]'),
 ('outer-passive','distance >= squaredLimits[0]','distance >= squaredLimits[2]'),
 ('optional-normal-facing','if ((u32)(delta - 0x2BC) < 0xA89u) {','if ((*(u32*)(otherData + 4) & 0x40000) && (u32)(delta - 0x2BC) < 0xA89u) {'),
 ('normal-global-write','if (rectangular && g_FieldSystemMode == 0)','if (g_FieldSystemMode == 0)'),
 ('discard-selected-button','goto enqueue;','continue;'),
 ('clear-skipped-parent','continue;\n        }\n        otherY','*(u8*)(actorData + 0x74) = 0xFF;\n            continue;\n        }\n        otherY'),
 ('slot-routine','(scriptRoutine << 18)','(scriptRoutine << 17)')]
for name,old,new in controls:
 assert old in s,name
 (out/(name+'.c')).write_text(s.replace(old,new))
pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path('src/field/main/misc8.c'),Path('disc/field.bin'),Path('pc_port/tests/field_interaction_retail_test.c'),Path('pc_port/tests/run_field_interaction_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),out/'body.c']}
(out/'pins.json').write_text(json.dumps({'pins':pins,'controls':[x[0] for x in controls],'scope':'verbatim production body vs all 495 retail instructions; controlled ratan2, rectangle, script lookup boundaries; bounded SQR arithmetic; not whole-game parity'},indent=2)+'\n')
PY
for mutant in inner-confirm outer-passive optional-normal-facing normal-global-write discard-selected-button clear-skipped-parent slot-routine; do
 clang -std=gnu17 -fno-pie -no-pie -O2 -Ipc_port/src "-DINTERACTION_BODY=\"$PWD/$OUT/$mutant.c\"" pc_port/tests/field_interaction_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
 rc=0
 "$OUT/$mutant" disc/field.bin > "$OUT/$mutant.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^INTERACTION FAIL ' "$OUT/$mutant.log"; then cat "$OUT/$mutant.log";exit 1;fi
 echo "INTERACTION CONTROL REJECTED $mutant"
done
echo "INTERACTION artifacts $OUT"
