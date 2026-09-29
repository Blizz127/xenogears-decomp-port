/* Force-include shim for host-differential builds of matching TUs.
 * Neutralizes INCLUDE_ASM (retail asm bytes are executed by remu instead)
 * and enables explicitly marked host-only candidates. These candidates
 * do not establish matching-build completion.
 * Usage: gcc -include tools/remu/tests/remu_shim.h ... <tu.c>
 */
#ifndef REMU_SHIM_H
#define REMU_SHIM_H
#define REMU_HOST_TEST 1
/* types.h skips its uintptr_t fallback under XENO_PC_PORT (the port gets the
 * real 64-bit type from <stdint.h>); provide it for host diff builds. */
#include <stdint.h>
#undef INCLUDE_ASM
#define INCLUDE_ASM(folder, name) /* retail asm: oracle is remu, not the host */
#undef INCLUDE_RODATA
#define INCLUDE_RODATA(folder, name)
#endif
