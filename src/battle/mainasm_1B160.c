/* battle mainasm_1B160: the retail-asm run at file offset 0x1B160, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif

#ifndef XENO_PC_PORT
s32 ArchiveDataSync();
void func_800716D8();
void func_8008AC50(void);
#endif

#ifndef XENO_PC_PORT

#ifndef XENO_PC_PORT
void func_8008AC50(void) {
loop_1:
    if (ArchiveDataSync() != 0) {
        func_800716D8();
        goto loop_1;
    }
}
#endif

#endif
