#!/usr/bin/env bash
set -euo pipefail
exec python3 "$(dirname "$0")/run_battle_guest_fixtures.py" menu_sound_fault menu_sound sound_enabled
