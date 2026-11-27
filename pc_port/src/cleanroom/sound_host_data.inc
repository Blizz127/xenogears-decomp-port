/* SPDX-License-Identifier: MIT */
/* Native sound storage and dispatch for the US executable. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../retail_data.h"

static SpuUnion soundRegisterStorage;
SpuUnion *g_pSoundSpuRegisters = &soundRegisterStorage;
u32 g_PcPortDiagPartialWdsLoads;
SoundPsxAddress D_800508A4[16];
SoundScriptHandlerFn g_SoundScriptHandlers[128];

u8 D_80050824[128];
u8 D_800509B0[228];
u8 D_80050A94[228];
u8 D_80050B78[120];
s16 D_80050BF0[3072];
_Static_assert(sizeof(soundRegisterStorage) == 512, "sound register bank size");


#include <stdatomic.h>
extern u8 *PcPort_SoundSequenceD298(u8 *, AudioManager *, AudioElement *);
extern u8 *PcPort_SoundSequenceD2D0(u8 *, AudioManager *, AudioElement *);
extern u8 *PcPort_SoundSequenceD300(u8 *, AudioManager *, AudioElement *);
extern u8 *PcPort_SoundSequenceD79C(u8 *, AudioManager *, AudioElement *);
extern u8 *PcPort_SoundSequenceE160(u8 *, AudioManager *, AudioElement *);

typedef s32 (*SoundBoundEnvelopeFn)(SoundEnvelope *);

static uint32_t SoundCallbackAddress(const u8 *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

/* The nine ADSR commands start at sequence opcode C2. The executable on the
 * player's disc supplies their guest function identities. Keep this reference
 * separate from the dispatch table so a moved or aliased entry still resolves
 * by target, and an unknown target cannot acquire a handler by position. */
enum { SOUND_ADSR_FIRST_SLOT = 0xc2 - 0x80, SOUND_ADSR_COMMAND_COUNT = 9 };
static u8 soundAdsrGuestIds[4 * SOUND_ADSR_COMMAND_COUNT];

static SoundScriptHandlerFn SoundResolveAdsrSequence(uint32_t guest)
{
    static const SoundScriptHandlerFn handlers[SOUND_ADSR_COMMAND_COUNT] = {
        SoundScriptSetAttackTime,
        SoundScriptSetDecayTime,
        SoundScriptSetSustain,
        SoundScriptSetRelease,
        SoundScriptSetSustainLevel,
        SoundScriptSetAdsrDRAndSR,
        SoundScriptSetAttackMode,
        SoundScriptSetSustainMode,
        SoundScriptSetReleaseMode,
    };
    if (guest == 0)
        return NULL;
    for (unsigned i = 0; i < SOUND_ADSR_COMMAND_COUNT; ++i) {
        if (guest == SoundCallbackAddress(&soundAdsrGuestIds[4 * i]))
            return handlers[i];
    }
    return NULL;
}

static SoundScriptHandlerFn SoundResolveNativeSequence(uint32_t guest)
{
    switch (guest) {
    case 0x8003CD00u: return SoundScriptDefaultHandler;
    case 0x8003CD30u: return SoundScriptFermata;
    case 0x8003CD4Cu: return SoundScriptNop3;
    case 0x8003CD7Cu: return SoundScriptDefaultSkip3;
    case 0x8003CD84u: return SoundScriptNop4;
    case 0x8003CE04u: return SoundScriptSaveOctaveAndIP;
    case 0x8003CE18u: return SoundScriptSetOctave;
    case 0x8003CE38u: return SoundScriptRaiseOctave;
    case 0x8003CE50u: return SoundScriptLowerOctave;
    case 0x8003CEC0u: return SoundScriptSetManagerUnk1a;
    case 0x8003CED4u: return SoundScriptAddManagerUnk1a;
    case 0x8003CEF0u: return func_8003CEF0;
    case 0x8003D0E8u: return func_8003D0E8;
    case 0x8003D208u: return SoundScriptSetUnk62;
    case 0x8003D298u: return PcPort_SoundSequenceD298;
    case 0x8003D328u: return SoundScriptPercussionOff;
    case 0x8003D340u: return SoundScriptSetActiveFlag800;
    case 0x8003D358u: return SoundScriptClearActiveFlag800;
    case 0x8003D4A4u: return SoundScriptSetVoiceFlags2000AndMode;
    case 0x8003D4C4u: return SoundScriptSetVoiceFlags2000ClearMode;
    case 0x8003D59Cu: return SoundScriptSetVoiceFlags4000ClearMode;
    case 0x8003D5BCu: return SoundScriptSkip3;
    case 0x8003D5C4u: return SoundScriptNop;
    case 0x8003D5CCu: return SoundScriptNop2;
    case 0x8003D5D4u: return SoundScriptCallE5BC;
    case 0x8003D74Cu: return SoundScriptSetUnk6E;
    case 0x8003D770u: return SoundScriptAddUnk6E;
    case 0x8003D7C8u: return func_8003D7C8;
    case 0x8003D854u: return SoundScriptToggleUnk04Bit2;
    case 0x8003D86Cu: return SoundScriptClearUnk04Bit1;
    case 0x8003DAECu: return SoundScriptSetUnkCEAndF6;
    case 0x8003DB0Cu: return SoundScriptClearUnkCEAndF6;
    case 0x8003DBE4u: return func_8003DBE4;
    case 0x8003DE54u: return SoundScriptSetUnkCEAndUnk116;
    case 0x8003DE74u: return SoundScriptClearUnkCEAndUnk116;
    case 0x8003DE94u: return SoundScriptSetUnk74;
    case 0x8003E140u: return SoundScriptSetUnkCEAndUnk136;
    case 0x8003E358u: return SoundScriptNop5;
    default: return SoundResolveAdsrSequence(guest);
    }
}

#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
extern int PcPort_SoundResidentRuntimeReady(void);
extern int PcPort_SoundResidentHybridEnabled(void);

static SoundScriptHandlerFn SoundResolveHybridSequence(uint32_t guest)
{
    switch (guest) {
    case 0x8003CD08u: return func_8003CD08;
    case 0x8003CE68u: return func_8003CE68;
    case 0x8003CE9Cu: return func_8003CE9C;
    case 0x8003D110u: return func_8003D110;
    case 0x8003D13Cu: return func_8003D13C;
    case 0x8003D1BCu: return func_8003D1BC;
    case 0x8003D2D0u: return PcPort_SoundSequenceD2D0;
    case 0x8003D300u: return PcPort_SoundSequenceD300;
    case 0x8003D370u: return func_8003D370;
    case 0x8003D3A4u: return func_8003D3A4;
    case 0x8003D60Cu: return func_8003D60C;
    case 0x8003D79Cu: return PcPort_SoundSequenceD79C;
    case 0x8003D7FCu: return func_8003D7FC;
    case 0x8003D884u: return func_8003D884;
    case 0x8003DAB0u: return func_8003DAB0;
    case 0x8003DB2Cu: return func_8003DB2C;
    case 0x8003DB58u: return func_8003DB58;
    case 0x8003DB98u: return func_8003DB98;
    case 0x8003DE18u: return func_8003DE18;
    case 0x8003DEB4u: return func_8003DEB4;
    case 0x8003DEE4u: return func_8003DEE4;
    case 0x8003DF3Cu: return func_8003DF3C;
    case 0x8003E160u: return PcPort_SoundSequenceE160;
    case 0x8003E4BCu: return func_8003E4BC;
    default: return NULL;
    }
}

extern s32 PcPort_SoundEnvelopeF190(SoundEnvelope *);
static SoundBoundEnvelopeFn SoundResolveHybridEnvelope(uint32_t guest)
{
    switch (guest) {
    case 0x8003F190u: return PcPort_SoundEnvelopeF190;
    case 0x8003F1A4u: return func_8003F1A4;
    case 0x8003F1ECu: return func_8003F1EC;
    case 0x8003F240u: return func_8003F240;
    case 0x8003F2A0u: return func_8003F2A0;
    case 0x8003F308u: return func_8003F308;
    case 0x8003F354u: return func_8003F354;
    case 0x8003F3C0u: return func_8003F3C0;
    default: return NULL;
    }
}
#endif

static u8 soundSequenceWords[128 * 4];
static u8 soundEnvelopeWords[16 * 4];
static u8 soundSequenceUsesResident[128];
static SoundBoundEnvelopeFn soundCallableMethods[16];
static unsigned soundBoundCommands, soundBoundMethods;
static int soundContainsResident;
static atomic_uint soundRefusedSequences;

static void SoundBindAvailableCallbacks(const u8 sequence[512], const u8 envelope[64])
{
    unsigned commands = 0, methods = 0;
    int usesResident = 0;
#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
    const int residentReady = PcPort_SoundResidentHybridEnabled() &&
                              PcPort_SoundResidentRuntimeReady();
#else
    (void)envelope;
#endif

    for (unsigned slot = 0; slot < 128; ++slot) {
        uint32_t guest = SoundCallbackAddress(sequence + 4 * slot);
        SoundScriptHandlerFn handler = SoundResolveNativeSequence(guest);
        soundSequenceUsesResident[slot] = 0;
#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
        if (!handler && residentReady) {
            handler = SoundResolveHybridSequence(guest);
            soundSequenceUsesResident[slot] = handler != NULL;
            usesResident |= handler != NULL;
        }
#endif
        g_SoundScriptHandlers[slot] = handler;
        commands += handler != NULL;
    }

    for (unsigned slot = 0; slot < 16; ++slot) {
        SoundBoundEnvelopeFn method = NULL;
#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
        if (residentReady)
            method = SoundResolveHybridEnvelope(SoundCallbackAddress(envelope + 4 * slot));
        usesResident |= method != NULL;
#endif
        soundCallableMethods[slot] = method;
        uintptr_t target = (uintptr_t)method;
        D_800508A4[slot] = target <= UINT32_MAX ? (SoundPsxAddress)target : 0;
        methods += D_800508A4[slot] != 0;
    }
    soundBoundCommands = commands;
    soundBoundMethods = methods;
    soundContainsResident = usesResident;
}

static void PcPort_InitEnvelopeMethodTable(void)
{
    SoundBindAvailableCallbacks(soundSequenceWords, soundEnvelopeWords);
}

unsigned PcPort_SoundBoundSequenceSlots(void) { return soundBoundCommands; }
unsigned PcPort_SoundBoundEnvelopeSlots(void) { return soundBoundMethods; }

int PcPort_TrySoundSequenceCommand(unsigned slot, u8 **cursor,
                                   AudioManager *manager, AudioElement *element)
{
    if (slot >= 128 || !cursor || !*cursor || !manager || !element)
        return 0;
    SoundScriptHandlerFn handler = g_SoundScriptHandlers[slot];
    if (!handler)
        return 0;
#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
    if (soundSequenceUsesResident[slot] && !PcPort_SoundResidentRuntimeReady())
        return 0;
#endif
    *cursor = handler(*cursor, manager, element);
    return 1;
}

int PcPort_TrySoundEnvelopeMethod(unsigned slot, SoundEnvelope *envelope, s32 *result)
{
    if (slot >= 16 || !envelope || !result || !soundCallableMethods[slot])
        return 0;
#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
    if (!PcPort_SoundResidentRuntimeReady())
        return 0;
#endif
    *result = soundCallableMethods[slot](envelope);
    return 1;
}

int PcPort_SoundNativeSequencerAvailable(void)
{
#ifdef XENO_SOUND_HAS_COMPLETE_NATIVE_SEQUENCE_DRIVERS
    return !soundContainsResident && soundBoundCommands == 128 && soundBoundMethods == 16;
#else
    return 0;
#endif
}

static int SoundSequencePlaybackAvailable(void)
{
    if (PcPort_SoundNativeSequencerAvailable())
        return 1;
#if defined(XENO_SOUND_HAS_COMPLETE_NATIVE_SEQUENCE_DRIVERS) && defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
    return soundContainsResident && soundBoundCommands == 128 && soundBoundMethods == 16 &&
           PcPort_SoundResidentRuntimeReady();
#else
    return 0;
#endif
}

int PcPort_AdmitSoundSequence(void)
{
    if (SoundSequencePlaybackAvailable())
        return 1;
    if (atomic_fetch_add_explicit(&soundRefusedSequences, 1, memory_order_relaxed) == 0)
        fprintf(stderr, "[xeno-port] sequence playback unavailable (%u/128 command slots, "
                "%u/16 packed envelope slots); audio maintenance continues.\n",
                soundBoundCommands, soundBoundMethods);
    return 0;
}

unsigned PcPort_SoundRefusedSequences(void)
{
    return atomic_load_explicit(&soundRefusedSequences, memory_order_relaxed);
}

/* Keep the BIOS event active for sound maintenance when sequence traversal
 * is unavailable. The backend register flush has its own slot. */
extern long PcPort_SoundMaintenanceTick(void);

static long (*PcPort_SoundSequenceTimer(void))(void)
{
#ifdef XENO_SOUND_HAS_COMPLETE_NATIVE_SEQUENCE_DRIVERS
    if (SoundSequencePlaybackAvailable())
        return func_8003C020;
#endif
    return PcPort_SoundMaintenanceTick;
}

static void __attribute__((constructor)) SoundLoadProgramData(void)
{
    static const XenoRetailRange ranges[] = {
        XENO_RD(D_80050824, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050824u), sizeof(D_80050824)),
        XENO_RD(D_800509B0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800509B0u), sizeof(D_800509B0)),
        XENO_RD(D_80050A94, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050A94u), sizeof(D_80050A94)),
        XENO_RD(D_80050B78, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050B78u), sizeof(D_80050B78)),
        XENO_RD(D_80050BF0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050BF0u), sizeof(D_80050BF0)),
        XENO_RD(soundSequenceWords, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050624u), sizeof soundSequenceWords),
        XENO_RD(soundAdsrGuestIds, XENO_RD_SLUS,
                XENO_RD_SLUS_OFF(0x80050624u) + 4 * SOUND_ADSR_FIRST_SLOT,
                sizeof soundAdsrGuestIds),
        XENO_RD(soundEnvelopeWords, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800508A4u), sizeof soundEnvelopeWords)
    };
    XenoRetailData_LoadTable("sound_sdata", ranges, sizeof ranges / sizeof ranges[0]);
    /* Component tests may load the host service without the executable. */
    if (XenoRetailData_Optional() && !XenoRetailData_File(XENO_RD_SLUS))
        return;
    PcPort_InitEnvelopeMethodTable();
}
