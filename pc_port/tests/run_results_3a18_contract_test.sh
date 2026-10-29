#!/usr/bin/env bash
# Contract oracle for RESULTS caller 0x801E3A18; see
# results_3a18_contract_test.c.  Extracts the RESULTS module from
# disc/disc1.bin into a temporary directory outside the tree (retail bytes
# never enter the repository), then requires the C model to match the
# retail bytes in the production interpreter at O0/O2/UBSan.  Two model
# mutants must fail: caching the slot kind instead of re-reading it through
# 0x801E44EC, and omitting the guest frame.  No port build is needed.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/results-3a18-contract.XXXXXXXX")
echo "RESULTS 3A18 artifacts: $OUT"
python3 tools/scripts/extract_battle_results_module.py --output "$OUT/results_module.bin" \
    > "$OUT/results_module.json"
python3 - "$OUT/results_module.json" <<'PY'
import json, sys
m = json.load(open(sys.argv[1]))
assert m['payload_sha256'] == 'f474fd482506b89eaff9601d5958a1c867d35a4ad11c1f58f0ecef630008a0c8', m['payload_sha256']
assert m['load_address'] == '0x801DE000' and m['size'] == 0x6500
PY
CASES=${RESULTS_3A18_CASES:-20000}
BASE=(-std=gnu17 -Wall -Wextra -Werror -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    [ "$opt" = UBSan ] && flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/results_3a18_contract_test.c \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.test"
    "$OUT/$opt.test" "$OUT/results_module.bin" "$CASES" > "$OUT/$opt.log" 2>&1 || {
        cat "$OUT/$opt.log" >&2; echo "RESULTS 3A18 RED $opt" >&2; exit 1; }
    tail -2 "$OUT/$opt.log"
done
for mutant in MUTANT_CACHE_KIND MUTANT_NO_FRAME; do
    "${CC:-gcc}" "${BASE[@]}" -O2 -D"$mutant" pc_port/tests/results_3a18_contract_test.c \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" "$OUT/results_module.bin" "$CASES" > "$OUT/$mutant.log" 2>&1; then
        echo "RESULTS 3A18 FAIL mutant survived: $mutant" >&2
        exit 1
    fi
    grep -q 'RESULTS 3A18 FAIL' "$OUT/$mutant.log"
    echo "RESULTS 3A18 negative control PASS: $mutant fails"
done
