#ifndef XENO_SOUND_DEVICE_INTERFACE_H
#define XENO_SOUND_DEVICE_INTERFACE_H

/*
 * Sound interface used by the game and reconstructed libspu sources.
 * The record offsets are visible in the SpuSetCommonAttr and reverb
 * register accesses. The volume controller embeds the 40-byte mixer record
 * directly (include/system/sound.h), including on 64-bit hosts.
 */
typedef struct {
    short left, right;
} SpuVolume;

typedef struct {
    SpuVolume volume;
    int reverb, mix;
} XenoSpuInputMixer;

typedef struct {
    unsigned int mask;
    SpuVolume mvol;
    SpuVolume mvolmode;
    unsigned int reserved_0c;
    XenoSpuInputMixer cd, ext;
} SpuCommonAttr;

typedef struct {
    unsigned int mask;
    int mode;
    SpuVolume depth;
    int delay, feedback;
} SpuReverbAttr;

/* Four consecutive 1 KiB sample buffers. The decoded read switches to
 * the two voice channels at byte 0x800. */
typedef struct {
    short cd_left[512], cd_right[512];
    short voice1[512], voice3[512];
} SpuDecodedData;

/* Transfer completion and SPU interrupts both use no-argument callbacks. */
typedef void (*XenoSpuCallback)(void);
typedef XenoSpuCallback SpuTransferCallbackProc;
typedef XenoSpuCallback SpuIRQCallbackProc;

enum {
    SPU_SUCCESS = 0,
    SPU_ERROR = -1,
    SPU_OFF = 0,
    SPU_ON = 1,
    SPU_RESET = 3,
    SPU_OFF_ENV_ON = 2,
    SPU_ON_ENV_OFF = 3
};

enum {
    SPU_TRANSFER_BY_DMA = 0,
    SPU_TRANSFER_BY_IO = 1,
    SPU_ALL = 0,
    SPU_CDONLY = 5,
    SPU_VOICEONLY = 6,
    SPU_DECODE_FIRSTHALF = 0,
    SPU_DECODE_SECONDHALF = 1
};

enum {
    SPU_COMMON_MVOLL     = 1 << 0,
    SPU_COMMON_MVOLR     = 1 << 1,
    SPU_COMMON_MVOLMODEL = 1 << 2,
    SPU_COMMON_MVOLMODER = 1 << 3,
    SPU_COMMON_CDVOLL    = 1 << 6,
    SPU_COMMON_CDVOLR    = 1 << 7,
    SPU_COMMON_CDREV     = 1 << 8,
    SPU_COMMON_CDMIX     = 1 << 9,
    SPU_COMMON_EXTVOLL   = 1 << 10,
    SPU_COMMON_EXTVOLR   = 1 << 11,
    SPU_COMMON_EXTREV    = 1 << 12,
    SPU_COMMON_EXTMIX    = 1 << 13
};

enum {
    SPU_VOICE_DIRECT = 0,
    SPU_VOICE_LINEARIncN, SPU_VOICE_LINEARIncR,
    SPU_VOICE_LINEARDecN, SPU_VOICE_LINEARDecR,
    SPU_VOICE_EXPIncN, SPU_VOICE_EXPIncR, SPU_VOICE_EXPDec
};

enum {
    SPU_REV_MODE_CHECK = -1,
    SPU_REV_MODE_OFF = 0,
    SPU_REV_MODE_ECHO = 7,
    SPU_REV_MODE_DELAY = 8,
    SPU_REV_MODE_MAX = 10,
    SPU_REV_MODE_CLEAR_WA = 0x100
};

/* The matching sources spell console words as long. The native provider
 * (PsyCross LIBSPU.C) declares the transfer, reverb and IRQ ABI with int,
 * including the word it writes through SpuGetReverbModeType's pointer. */
#ifdef XENO_PC_PORT
typedef int XenoSpuWord;
typedef unsigned int XenoSpuUnsignedWord;
#else
typedef long XenoSpuWord;
typedef unsigned long XenoSpuUnsignedWord;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SpuInit(void);
#ifndef XENO_PC_PORT
void SpuStart(void);
#endif
void SpuQuit(void);
XenoSpuWord SpuInitMalloc(XenoSpuWord slots, char *table);

void SpuSetCommonAttr(SpuCommonAttr *settings);
void SpuGetVoiceEnvelopeAttr(int voice, long *key_state, short *amplitude);
void SpuSetVoiceLoopStartAddr(int voice, XenoSpuUnsignedWord address);
long SpuSetNoiseClock(long rate);

XenoSpuWord SpuSetReverb(XenoSpuWord enabled);
XenoSpuWord SpuSetReverbModeType(XenoSpuWord mode);
void SpuGetReverbModeType(XenoSpuWord *mode);
void SpuSetReverbModeDepth(short left, short right);
void SpuSetReverbModeDelayTime(XenoSpuWord delay);
void SpuSetReverbModeFeedback(XenoSpuWord feedback);
XenoSpuWord SpuClearReverbWorkArea(XenoSpuWord mode);

SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc callback);
XenoSpuWord SpuSetTransferMode(XenoSpuWord mode);
XenoSpuUnsignedWord SpuSetTransferStartAddr(XenoSpuUnsignedWord address);
XenoSpuUnsignedWord SpuWrite(unsigned char *source, XenoSpuUnsignedWord bytes);
XenoSpuUnsignedWord SpuRead(unsigned char *destination, XenoSpuUnsignedWord bytes);
XenoSpuWord SpuReadDecodedData(SpuDecodedData *destination, XenoSpuWord channels);

XenoSpuWord SpuSetIRQ(XenoSpuWord enabled);
SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc callback);

#ifdef __cplusplus
}
#endif
#endif
