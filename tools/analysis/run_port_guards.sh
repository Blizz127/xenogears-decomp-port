#!/usr/bin/env bash
# Run the port data/SDK policy guards (see pc_port/README.md "Game data and
# Sony code" and pc_port/THIRD_PARTY.md):
#   1. retail_data_guard.py  -- no retail game bytes/tables in tracked files
#   2. psyq_classify.py      -- config/psyq_classification.tsv is current
#   3. psyq_port_guard.py    -- no Psy-Q SDK code/tables in pc_port (and, when
#                               the port binary exists, none linked into it)
#
# Usage: tools/analysis/run_port_guards.sh [--staged] [--no-disc] [--binary PATH]...
#   --staged   scan the git index only (pre-commit hook)
#   --no-disc  hash signatures only; never read disc/ (CI)
# Without --no-disc the disc files in disc/ are used when present.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
PY="${PYTHON:-python3}"
retail_args=()
psyq_args=()
bins=()
while [ $# -gt 0 ]; do
    case "$1" in
        --staged) retail_args+=(--staged) ;;
        --no-disc) retail_args+=(--no-disc); psyq_args+=(--no-disc) ;;
        --binary) shift; bins+=(--binary "$1") ;;
        *) echo "usage: $0 [--staged] [--no-disc] [--binary PATH]..." >&2; exit 2 ;;
    esac
    shift
done
if [ "${#bins[@]}" -eq 0 ] && [ -x pc_port/build_native/xeno-port ]; then
    bins=(--binary pc_port/build_native/xeno-port)
fi

rc=0
echo "== retail_data_guard"
"$PY" tools/analysis/retail_data_guard.py "${retail_args[@]}" || rc=1
echo "== psyq_classify --check"
"$PY" tools/analysis/psyq_classify.py --check || rc=1
echo "== psyq_port_guard"
"$PY" tools/analysis/psyq_port_guard.py -q "${psyq_args[@]}" "${bins[@]}" || rc=1
if [ "$rc" -ne 0 ]; then
    echo "port guards: FAIL"
else
    echo "port guards: PASS"
fi
exit "$rc"
