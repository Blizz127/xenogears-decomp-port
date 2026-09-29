#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
CARD_DIGIT_ROUTINE=801E6F5C exec bash pc_port/tests/run_menu_card_halfword_test.sh
