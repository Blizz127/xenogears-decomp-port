#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

python3 pc_port/tests/field_script_vm_audit_test.py

args=()
if [[ -n "${FIELD_MATCHING_ROOT:-}" ]]; then
    args+=(--matching-root "$FIELD_MATCHING_ROOT")
fi

out="$(mktemp -d -t field-script-vm-audit.XXXXXX)"
trap 'rm -rf -- "$out"' EXIT
python3 pc_port/tools/audit_field_script_vm.py "${args[@]}" \
    --json "$out/ledger.json" --csv "$out/ledger.csv" >"$out/summary.log"

grep -q '^FIELD_SCRIPT_VM_AUDIT slots=483 unique=481 table_mismatches=0$' \
    "$out/summary.log"
test "$(python3 -c 'import json,sys; print(len(json.load(open(sys.argv[1]))["handlers"]))' "$out/ledger.json")" = 481
test "$(wc -l < "$out/ledger.csv")" = 482

echo "FIELD_SCRIPT_VM_AUDIT_RUNNER PASS"
