#!/usr/bin/env bash
set -euo pipefail
exec python3 "$(dirname "$0")/run_battle_guest_fixtures.py" command_menu_default command_menu_early_exit command_menu_fault command_menu_four_state command_menu_two_frame
