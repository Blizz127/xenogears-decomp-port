#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/effect_constructor_register_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
from tools.scripts.audit_field_clip_vm import read_overlay, OVERLAY_SHA
b=read_overlay('disc/disc1.bin')
assert sha256(b).hexdigest()==OVERLAY_SHA
assert sha256(b[0x4a00:0x5258]).hexdigest()=='4ede271950970cf43e61e2864660eaa9a27bbd4145f191c81330dab80d6ec32c'
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang -std=c17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/src pc_port/tests/effect_constructor_register_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
if "$OUT/O2.test" zero-carry > "$OUT/zero-carry.log" 2>&1; then
 echo 'EFFECT REGISTER zero-carry mutant survived' >&2; exit 1
fi
rg -q 'EFFECT REGISTER FAIL' "$OUT/zero-carry.log"
echo 'EFFECT REGISTER negative control PASS: zero-carry (RAM-only mutation)'
