#!/usr/bin/env bash
# Identity and context controls with an isolated fixture runtime.
set -euo pipefail
if [[ ${XENO_DIGITS_GATE_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_DIGITS_GATE_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-digits-gate.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
gcc -std=gnu17 -O0 -fno-pie -no-pie -Ipc_port/src \
    pc_port/tests/battle_8aaa0_gate_test.c -lcrypto -o "$OUT/test"
env -u XENO_BATTLE_NATIVE_DIGITS_EXPERIMENT "$OUT/test" disc/battle.bin
