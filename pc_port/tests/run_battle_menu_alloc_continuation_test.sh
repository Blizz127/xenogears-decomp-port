#!/usr/bin/env bash
set -euo pipefail
exec bash "$(dirname "$0")/lib/run_owned_battle_guest.sh" battle_menu_alloc_continuation_test
