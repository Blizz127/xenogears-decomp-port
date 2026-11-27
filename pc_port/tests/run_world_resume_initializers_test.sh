#!/usr/bin/env bash
source "$(dirname "$0")/lib/world_guest_checks.sh"
run_world_guest_fixture pc_port/tests/world_resume_initializers_test.c
