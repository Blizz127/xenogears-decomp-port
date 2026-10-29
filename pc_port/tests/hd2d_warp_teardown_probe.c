/* Read-only diagnostics around native field cleanup; never skip real calls. */
#include <stdio.h>
#define TRACE_VOID(name) \
 extern void __real_##name(void); \
 void __wrap_##name(void) { \
   fprintf(stderr,"[capture-teardown] enter " #name "\n"); \
   __real_##name(); \
   fprintf(stderr,"[capture-teardown] leave " #name "\n"); \
 }
TRACE_VOID(func_800798BC)
TRACE_VOID(func_800A91F0)
TRACE_VOID(func_800A31E8)
TRACE_VOID(FieldParticlesFreeAll)
TRACE_VOID(func_800864F0)
TRACE_VOID(func_8007FFE8)
TRACE_VOID(FieldFree)
TRACE_VOID(FieldPartyFreeSkinDataBuffers)
TRACE_VOID(func_80085988)
