/* Battle RESULTS module routines the port may run as native C instead of
 * interpreting the retail bytes the battle loads at 0x801DE000.  Hand-
 * maintained; one {jal target, name} entry per adopted routine.  Each name
 * must be defined as battle_results_<name> by pc_port/src/battle_results_host.c
 * (build_port.sh fails otherwise), and every entry must pass
 * pc_port/tests/run_battle_results_host_differential_test.sh.
 *
 * The address is the routine's real call target (the jal operand), not the
 * first prologue instruction: several results routines hoist loads above
 * their ADDIU SP (see BATCHES_INDEX.md "entry corrections").  The runtime
 * dispatches on the guest PC equal to this address, so it must also be
 * reached only as a call -- the differential test checks that no branch,
 * J or data word in the module targets it.
 *
 * Names in this comment are left unquoted on purpose: the build and test
 * scripts scrape quoted func_ names out of this file.
 *
 * Everything not listed here is interpreted (disclosed CPU fallback).
 */
    {0x801e24b0u, "func_801E24B0"},
    {0x801df910u, "func_801DF910"},
    {0x801dfa38u, "func_801DFA38"},
    {0x801e1590u, "func_801E1590"},
    {0x801e2888u, "func_801E2888"},
    {0x801e403cu, "func_801E403C"},
    {0x801e41b4u, "func_801E41B4"},
    {0x801e1370u, "func_801E1370"},
    {0x801e2eb0u, "func_801E2EB0"},
    {0x801e3be0u, "func_801E3BE0"},
    {0x801e3d54u, "func_801E3D54"},
    {0x801e3e14u, "func_801E3E14"},
    {0x801e3ea4u, "func_801E3EA4"},
    {0x801e3f28u, "func_801E3F28"},
    {0x801e3fb0u, "func_801E3FB0"},
    {0x801e3a18u, "func_801E3A18"},
