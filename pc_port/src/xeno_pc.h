#ifndef XENO_PC_H
#define XENO_PC_H

/*
 * Central plumbing for the native PC port build (XENO_PC_PORT).
 *
 * This header is included by the port build only. Its jobs (grown over the
 * phases of the roadmap) are:
 *   - Phase 0: define the port build identity.
 *   - Phase 1: neutralise INCLUDE_ASM (raw MIPS cannot run on x86) so the game
 *              translation units compile, with undefined symbols filled by
 *              auto-generated placeholder stubs until they are decompiled.
 *   - Phase 1+: map the game's PsyQ usage onto PsyCross's PsyQ-compatible
 *               headers (include/psx) instead of the on-hardware reimplementations
 *               in src/slus_006.64/psyq/*.
 */

#ifndef XENO_PC_PORT
#define XENO_PC_PORT 1
#endif

/* Raw assembly stubs are inert in the port build; missing functions are
 * provided as placeholders until decompiled (see SKIP_ASM in include_asm.h). */
#ifndef SKIP_ASM
#define SKIP_ASM 1
#endif

#endif /* XENO_PC_H */
