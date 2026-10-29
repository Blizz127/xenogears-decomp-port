#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define XENO_PC_PORT 1
#define NUM_VOICES 24
typedef uint32_t u32;
typedef uint8_t u8;
typedef int32_t s32;
typedef struct SoundVoiceData SoundVoiceData;
static struct { uintptr_t before; SoundVoiceData *channels[NUM_VOICES]; uintptr_t after; } fixture;
#define g_SoundChannels fixture.channels
#include "sound_clear_body.inc"
int main(void) {
 fixture.before=(uintptr_t)0x12345678u;fixture.after=(uintptr_t)0x87654321u;
 for(unsigned i=0;i<NUM_VOICES;i++) {
  uintptr_t v=(uintptr_t)(i+1u);
#if UINTPTR_MAX > UINT32_MAX
  v|=(uintptr_t)0x12345678u<<32;
#endif
  fixture.channels[i]=(SoundVoiceData*)v;
 }
 SoundClearVoiceDataPointers();
 for(unsigned i=0;i<NUM_VOICES;i++)assert(fixture.channels[i]==NULL);
 assert(fixture.before==0x12345678u && fixture.after==0x87654321u);
 puts("sound channel clear PASS 24 full-width pointers, adjacent guards");
}
