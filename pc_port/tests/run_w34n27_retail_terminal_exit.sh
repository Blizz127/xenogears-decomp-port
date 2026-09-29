#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export XENO_RETAIL_TRACE_PROFILE=terminal_exit
export W34N23_OUT="${W34N27_OUT:-$ROOT/pc_port/build_native/w34n27_retail_terminal_exit}"
exec bash "$ROOT/pc_port/tests/run_w34n23_retail_world_trace.sh" "$@"
