#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <SDL.h>
struct EXEC;
#include "psx/libapi.h"
extern void PsyX_Sys_InitSoundGate(void);
extern void PsyX_Sys_DispatchCounter2(void);
extern void PsyX_Sys_SoundGateEnterCritical(void);
extern void PsyX_Sys_SoundGateExitCritical(void);
extern int PsyX_Sys_ReserveSoundEvent(unsigned int);
extern int PsyX_Sys_ReleaseSoundEventReservation(void);
extern int PsyX_Sys_ValidateSoundEventReservation(unsigned int, int);
extern int PsyX_Sys_BeginReservedSoundBracket(unsigned int);
extern int PsyX_Sys_FinishReservedSoundBracket(void);
extern int PsyX_Sys_AbortReservedSoundBracket(void);
extern int PsyX_Sys_ReservedSoundBracketPristine(unsigned int);
static int calls;
static int simulated_native_failure(unsigned int event, int mutate) {
    jmp_buf failure;
    if (!PsyX_Sys_ReserveSoundEvent(event)) return -1;
    if (setjmp(failure))
        return PsyX_Sys_AbortReservedSoundBracket() == 1 ? 1 : -1;
    if (!PsyX_Sys_BeginReservedSoundBracket(event)) {
        PsyX_Sys_ReleaseSoundEventReservation();
        return -1;
    }
    if (mutate) EnableEvent(event);
    longjmp(failure, 1);
}
static int callback_target, callback_replacement;
static long reenter(void) {
    CloseEvent(callback_target);
    callback_replacement=OpenEvent(0xf2000002u,2,0x2000,NULL);
    EnableEvent(callback_replacement);
    DeliverEvent(0xf2000002u,2);
    return 0;
}
static long tick(void) { ++calls; return 0; }
static int foreign_pristine(void *p) { return PsyX_Sys_ReservedSoundBracketPristine((unsigned)(uintptr_t)p); }
static int foreign_release(void *unused) { (void)unused; return PsyX_Sys_ReleaseSoundEventReservation(); }
static int dispatch(void *unused) { (void)unused; PsyX_Sys_DispatchCounter2(); return 0; }
static void dispatch_thread(void) {
    SDL_Thread *t = SDL_CreateThread(dispatch, "sound-reserve-test", NULL);
    assert(t); SDL_WaitThread(t, NULL);
}
int main(void) {
    assert(SDL_Init(0) == 0);
    assert(!PsyX_Sys_ReserveSoundEvent(1));
    PsyX_Sys_InitSoundGate();
    int event = OpenEvent(0xf2000002u, 2, 0x1000, tick);
    int other = OpenEvent(1, 2, 0x1000, tick);
    assert(event > 0 && other > 0);
    assert(!PsyX_Sys_ReserveSoundEvent(0));
    assert(!PsyX_Sys_ReserveSoundEvent(event));
    EnableEvent(other); assert(!PsyX_Sys_ReserveSoundEvent(other));
    EnableEvent(event); dispatch_thread(); assert(calls == 1);
    PsyX_Sys_SoundGateEnterCritical(); assert(!PsyX_Sys_ReserveSoundEvent(event));
    PsyX_Sys_SoundGateExitCritical();
    assert(PsyX_Sys_ReserveSoundEvent(event));
    assert(PsyX_Sys_ValidateSoundEventReservation(event,1));
    assert(!PsyX_Sys_ValidateSoundEventReservation(other,1));
    assert(!PsyX_Sys_ReserveSoundEvent(event));
    SDL_Thread *foreign = SDL_CreateThread(foreign_release, "foreign-release", NULL);
    int foreign_result = -1; assert(foreign); SDL_WaitThread(foreign, &foreign_result);
    assert(foreign_result == 0);
    dispatch_thread(); assert(calls == 1);
    DisableEvent(event); assert(PsyX_Sys_ValidateSoundEventReservation(event,0)); EnableEvent(event);
    assert(PsyX_Sys_ValidateSoundEventReservation(event,1));
    dispatch_thread(); assert(calls == 1);
    assert(PsyX_Sys_ReleaseSoundEventReservation());
    assert(!PsyX_Sys_ReleaseSoundEventReservation());
    dispatch_thread(); assert(calls == 2);
    assert(PsyX_Sys_ReserveSoundEvent(event));
    CloseEvent(event);
    int replacement=OpenEvent(0xf2000002u,2,0x1000,tick); assert(replacement==event);
    EnableEvent(replacement);
    assert(!PsyX_Sys_ValidateSoundEventReservation(event,1));
    assert(PsyX_Sys_ReleaseSoundEventReservation());
    CloseEvent(replacement); assert(!PsyX_Sys_ReserveSoundEvent(event));
    CloseEvent(other);
    int poll = OpenEvent(0xf2000002u, 2, 0x2000, NULL); assert(poll>0);
    EnableEvent(poll); DeliverEvent(0xf2000002u, 2);
    assert(simulated_native_failure(poll,0)==1);
    assert(TestEvent(poll)==1); /* longjmp abort restored pending */
    DeliverEvent(0xf2000002u, 2);
    assert(simulated_native_failure(poll,1)==-1); /* mutation is not restored */
    assert(TestEvent(poll)==0);
    DeliverEvent(0xf2000002u, 2);
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(!PsyX_Sys_BeginReservedSoundBracket(poll+1));
    assert(!PsyX_Sys_ReservedSoundBracketPristine(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    assert(PsyX_Sys_ReservedSoundBracketPristine(poll));
    assert(!PsyX_Sys_ReservedSoundBracketPristine(poll+1));
    SDL_Thread *probe=SDL_CreateThread(foreign_pristine,"foreign-pristine",(void*)(uintptr_t)poll);
    int foreign_ok=-1;assert(probe);SDL_WaitThread(probe,&foreign_ok);assert(!foreign_ok);
    assert(!PsyX_Sys_ReleaseSoundEventReservation());
    assert(!PsyX_Sys_BeginReservedSoundBracket(poll));
    PsyX_Sys_SoundGateEnterCritical();
    assert(!PsyX_Sys_ReservedSoundBracketPristine(poll));
    assert(!PsyX_Sys_FinishReservedSoundBracket());
    assert(PsyX_Sys_AbortReservedSoundBracket()==1);
    assert(!PsyX_Sys_ReserveSoundEvent(poll)); /* callback hold remains */
    PsyX_Sys_SoundGateExitCritical();
    assert(TestEvent(poll)==1); /* pending event restored on abort */
    DeliverEvent(0xf2000002u,2);
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    assert(PsyX_Sys_FinishReservedSoundBracket());
    assert(TestEvent(poll)==0); /* retail disable/enable clears pending */
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    EnableEvent(poll); /* nested caller consumed the bracket */
    assert(!PsyX_Sys_ReservedSoundBracketPristine(poll));
    assert(!PsyX_Sys_FinishReservedSoundBracket());
    DisableEvent(poll); /* state looks unchanged, but history differs */
    assert(!PsyX_Sys_ReservedSoundBracketPristine(poll));
    assert(!PsyX_Sys_FinishReservedSoundBracket());
    assert(PsyX_Sys_AbortReservedSoundBracket()==2);
    assert(!PsyX_Sys_ReserveSoundEvent(poll)); /* callback hold remains */
    EnableEvent(poll); /* callback closes its own hold */
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    CloseEvent(poll);
    assert(!PsyX_Sys_ReservedSoundBracketPristine(poll));
    int newer=OpenEvent(0xf2000002u,2,0x2000,NULL); assert(newer==poll);
    EnableEvent(newer); assert(!PsyX_Sys_ReservedSoundBracketPristine(poll)); DeliverEvent(0xf2000002u,2);
    assert(PsyX_Sys_AbortReservedSoundBracket()==2);
    assert(TestEvent(newer)==1); /* replacement event not modified */
    CloseEvent(newer);
    poll=OpenEvent(0xf2000002u,2,0x2000,NULL); assert(poll>0);
    int trigger=OpenEvent(7,8,0x1000,reenter); assert(trigger>0);
    EnableEvent(poll); EnableEvent(trigger); callback_target=poll;
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    DeliverEvent(7,8); /* a synchronous callback closes and reuses the slot */
    assert(callback_replacement==poll);
    assert(!PsyX_Sys_FinishReservedSoundBracket());
    assert(PsyX_Sys_AbortReservedSoundBracket()==2);
    assert(TestEvent(callback_replacement)==1);
    CloseEvent(callback_replacement); CloseEvent(trigger);
    int observed=OpenEvent(0xf2000002u,2,0x1000,tick); assert(observed>0);
    poll=OpenEvent(0xf2000002u,2,0x2000,NULL); assert(poll>0);
    EnableEvent(observed); EnableEvent(poll); int before=calls;
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    PsyX_Sys_SoundGateEnterCritical();
    assert(PsyX_Sys_AbortReservedSoundBracket()==1);
    dispatch_thread(); assert(calls==before); /* callback-owned hold stays */
    PsyX_Sys_SoundGateExitCritical();
    dispatch_thread(); assert(calls==before+1); /* no leaked reservation */
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    EnableEvent(poll); /* callback consumes helper bracket */
    assert(PsyX_Sys_AbortReservedSoundBracket()==2);
    dispatch_thread(); assert(calls==before+2); /* no leaked hold */
    int second=OpenEvent(0xf2000002u,2,0x2000,NULL); assert(second>0);
    EnableEvent(second);
    assert(PsyX_Sys_ReserveSoundEvent(poll));
    assert(PsyX_Sys_BeginReservedSoundBracket(poll));
    DisableEvent(second); /* callback owns a second sound-event hold */
    assert(PsyX_Sys_AbortReservedSoundBracket()==1);
    dispatch_thread(); assert(calls==before+2);
    EnableEvent(second);
    dispatch_thread(); assert(calls==before+3);
    assert(TestEvent(second)==1);
    CloseEvent(second); CloseEvent(poll); CloseEvent(observed); SDL_Quit();
    puts("PASS synthetic sound-event reservation, bracket cleanup and callback re-entry");
    return 0;
}
