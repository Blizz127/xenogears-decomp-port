/* Linker-wrapped yield makes blocked-state observations deterministic.
 * No fake event completion is injected into production code. */
#include <assert.h>
#include <stdio.h>
#include <SDL.h>
struct EXEC;
#include "psx/libapi.h"
extern void PsyX_Sys_InitSoundGate(void);
static SDL_sem *blocked,*release;
static SDL_atomic_t done;
static int handle,result;
void __wrap_SDL_Delay(Uint32 ms) {
    assert(ms==0);
    assert(SDL_SemPost(blocked)==0);
    assert(SDL_SemWaitTimeout(release,2000)==0);
}
static int waiter(void *unused) {
    (void)unused;result=WaitEvent(handle);SDL_AtomicSet(&done,1);return 0;
}
int main(void) {
    /* Both pre-mutex boot calls and initialized calls must work. */
    for(int initialized=0;initialized<2;++initialized) {
        if(initialized)PsyX_Sys_InitSoundGate();
        handle=OpenEvent(0xf4000001,4,0x2000,NULL);
        assert(WaitEvent(handle)==0);EnableEvent(handle);
        DeliverEvent(0xf4000001,4);
        assert(WaitEvent(handle)==1 && TestEvent(handle)==0);
        CloseEvent(handle);assert(WaitEvent(handle)==0);
    }
    assert(WaitEvent(0)==0 && WaitEvent(0xffffffffu)==0);
    blocked=SDL_CreateSemaphore(0);release=SDL_CreateSemaphore(0);assert(blocked&&release);
    handle=OpenEvent(0xf4000001,4,0x2000,NULL);EnableEvent(handle);
    SDL_Thread *thread=SDL_CreateThread(waiter,"event-wait-test",NULL);assert(thread);
    assert(SDL_SemWaitTimeout(blocked,2000)==0 && !SDL_AtomicGet(&done));
    /* BIOS only tests enabled on entry. Disabling during the wait does
     * not turn an unfinished event into a successful/failed return. */
    DisableEvent(handle);SDL_SemPost(release);
    assert(SDL_SemWaitTimeout(blocked,2000)==0 && !SDL_AtomicGet(&done));
    EnableEvent(handle);DeliverEvent(0xf4000001,4);SDL_SemPost(release);
    SDL_WaitThread(thread,NULL);
    assert(SDL_AtomicGet(&done) && result==1 && TestEvent(handle)==0);
    CloseEvent(handle);SDL_DestroySemaphore(blocked);SDL_DestroySemaphore(release);
    puts("PASS WaitEvent immediate states, actual blocked waiter, disable while waiting, and delivered completion");
}
