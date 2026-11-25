#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
fixture=("$here/../lib/run_sound_access_fixture.sh" "$here/host.c")
"${fixture[@]}"
echo "Windows CD attribute critical section"
exec "${fixture[@]}" -DTEST_WINDOWS_CD_GATE
