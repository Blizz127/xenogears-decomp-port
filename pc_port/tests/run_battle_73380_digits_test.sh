#!/usr/bin/env bash
set -euo pipefail
exec python3 "$(dirname "$0")/run_battle_guest_fixtures.py" 73380_digits
