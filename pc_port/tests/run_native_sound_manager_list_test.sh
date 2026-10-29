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
slus_build_tu O0 src/slus_006.64/system/sound.c "$out/uut.o" SoundHandleError SoundReleaseAllVoices
slus_build_test O0 pc_port/tests/native_sound_manager_list_test.c "$out/test.o"
slus_link O0 "$out/test" "$out/test.o" "$out/uut.o"
timeout -k 5s 30s "$out/test"
