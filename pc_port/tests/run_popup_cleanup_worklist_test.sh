#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
source pc_port/tests/lib/slus_oracle_lib.sh
out=$(mktemp -d)
OUT=$out
trap 'rm -rf "$out"' EXIT
slus_check_disc
slus_build_deps O0
slus_build_tu O0 pc_port/src/work_list_port.c "$out/uut.o" none
slus_build_tu O0 src/slus_006.64/system/memory.c "$out/memory.o" none
slus_build_test O0 pc_port/tests/popup_cleanup_worklist_test.c "$out/test.o"
slus_link O0 "$out/test" "$out/test.o" "$out/uut.o" "$out/memory.o"
timeout -k 5s 30s "$out/test"
