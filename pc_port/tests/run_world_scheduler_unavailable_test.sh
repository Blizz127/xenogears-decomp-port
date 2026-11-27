#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
build=$(mktemp -d "${TMPDIR:-/tmp}/xeno-world-scheduler.XXXXXXXX")
trap 'rm -rf "$build"' EXIT
ulimit -c 0
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Ipc_port/src \
    pc_port/tests/w34b5h_scheduler_prod_test.c pc_port/src/world_map_scheduler.c \
    -o "$build/test"
for setup in cold reset; do
    result=0
    "$build/test" "$setup" > "$build/stdout" 2> "$build/stderr" || result=$?
    if [[ $result != 1 || $(cat "$build/stdout") != 'World scheduler stopped at entry with guest memory intact' ||
        $(cat "$build/stderr") != '[xeno-port] Unavailable world-map routine reached: wm_80097800' ]]; then
        cat "$build/stdout" "$build/stderr" >&2
        printf 'Unexpected scheduler outcome on %s start (exit %s)\n' "$setup" "$result" >&2
        exit 1
    fi
    printf 'World scheduler stopped before resuming its caller (%s start)\n' "$setup"
done
