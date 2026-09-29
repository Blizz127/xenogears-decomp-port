#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/saved_callback_adapter_test
mkdir -p "$OUT"
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
 -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -w -c pc_port/src/work_list_port.c -o "$OUT/$mode.work.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/saved_callback_adapter_test.c -o "$OUT/$mode.test.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.work.o" "$OUT/$mode.test.o" -o "$OUT/$mode.test"
 "$OUT/$mode.test"
done
ulimit -c 0
set +e
"$OUT/O2.test" unresolved > "$OUT/unresolved.log" 2>&1
status=$?
set -e
[ "$status" -eq 134 ]
rg -q 'unresolved guest callback 0x80123450' "$OUT/unresolved.log"
echo 'SAVED CALLBACK unresolved guest rejection PASS'
