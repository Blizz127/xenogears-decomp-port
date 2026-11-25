#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
exec "$here/../lib/run_sound_access_fixture.sh" "$here/host.c"
