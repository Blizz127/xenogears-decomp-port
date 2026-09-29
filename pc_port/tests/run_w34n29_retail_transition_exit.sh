#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export XENO_RETAIL_TRACE_PROFILE=transition_exit
export W34N23_OUT="${W34N29_OUT:-$ROOT/pc_port/build_native/w34n29_retail_transition_exit}"
exec bash "$ROOT/pc_port/tests/run_w34n23_retail_world_trace.sh" "$@"
