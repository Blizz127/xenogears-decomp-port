#ifndef XENO_RENDERER_SPU_H
#define XENO_RENDERER_SPU_H

#include <stdint.h>

/* Mixer and voice control records passed between the game, LIBSPU.C, and
 * PsyX_SPUAL.cpp. Sample addresses are offsets into SPU RAM. */
typedef struct {
    int16_t left, right;
} SpuVolume;

typedef struct {
    SpuVolume volume;
    int32_t reverb, mix;
} SpuExtAttr;

typedef struct {
    uint32_t mask;
    SpuVolume mvol, mvolmode, mvolx;
    SpuExtAttr cd, ext;
} SpuCommonAttr;

typedef struct {
    uint32_t mask;
    int32_t mode;
    SpuVolume depth;
    int32_t delay, feedback;
} SpuReverbAttr;

typedef struct {
    uint32_t voice, mask;
    SpuVolume volume, volmode, volumex;
    uint16_t pitch, note, sample_note;
    int16_t envx;
    uint32_t addr, loop_addr;
    int32_t a_mode, s_mode, r_mode;
    uint16_t ar, dr, sr, rr, sl;
    uint16_t adsr1, adsr2;
} SpuVoiceAttr;

typedef struct {
    int16_t cd_left[512], cd_right[512];
    int16_t voice1[512], voice3[512];
} SpuDecodedData;

typedef void XenoSpuEvent(void);
typedef XenoSpuEvent *SpuTransferCallbackProc;
typedef XenoSpuEvent *SpuIRQCallbackProc;

enum {
    SPU_OFF = 0, SPU_ON = 1, SPU_OFF_ENV_ON = 2, SPU_ON_ENV_OFF = 3,
    SPU_TRANSFER_BY_DMA = 0
};

enum {
    SPU_COMMON_MVOLL = 1 << 0, SPU_COMMON_MVOLR = 1 << 1,
    SPU_COMMON_MVOLMODEL = 1 << 2, SPU_COMMON_MVOLMODER = 1 << 3,
    SPU_COMMON_CDVOLL = 1 << 6, SPU_COMMON_CDVOLR = 1 << 7,
    SPU_COMMON_CDREV = 1 << 8, SPU_COMMON_CDMIX = 1 << 9,
    SPU_COMMON_EXTVOLL = 1 << 10, SPU_COMMON_EXTVOLR = 1 << 11,
    SPU_COMMON_EXTREV = 1 << 12, SPU_COMMON_EXTMIX = 1 << 13
};

enum {
    SPU_REV_MODE_HALL = 5, SPU_REV_MODE_MAX = 10,
    SPU_REV_MODE_CHECK = -1
};
enum {
    SPU_REV_MODE = 1, SPU_REV_DEPTHL = 2, SPU_REV_DEPTHR = 4,
    SPU_REV_DELAYTIME = 8, SPU_REV_FEEDBACK = 16
};

enum {
    SPU_VOICE_VOLL = 1 << 0, SPU_VOICE_VOLR = 1 << 1,
    SPU_VOICE_PITCH = 1 << 4, SPU_VOICE_WDSA = 1 << 7,
    SPU_VOICE_ADSR_AMODE = 1 << 8, SPU_VOICE_ADSR_SMODE = 1 << 9,
    SPU_VOICE_ADSR_RMODE = 1 << 10, SPU_VOICE_ADSR_AR = 1 << 11,
    SPU_VOICE_ADSR_DR = 1 << 12, SPU_VOICE_ADSR_SR = 1 << 13,
    SPU_VOICE_ADSR_RR = 1 << 14, SPU_VOICE_ADSR_SL = 1 << 15,
    SPU_VOICE_LSAX = 1 << 16, SPU_VOICE_ADSR_ADSR1 = 1 << 17,
    SPU_VOICE_ADSR_ADSR2 = 1 << 18
};
/* The 24 hardware channels map directly to bits in the key/voice masks. */
#define SPU_VOICECH(index) (1L << (index))
#define SPU_KEYCH(index) SPU_VOICECH(index)

#ifdef __cplusplus
extern "C" {
#endif

void SpuInit(void);
void SpuQuit(void);

int SpuInitMalloc(int slots, char *table);
int SpuMalloc(int bytes);
int SpuMallocWithStartAddr(unsigned int address, int bytes);
void SpuFree(unsigned int address);

unsigned int SpuWrite(unsigned char *source, unsigned int bytes);
unsigned int SpuRead(unsigned char *destination, unsigned int bytes);
int SpuReadDecodedData(SpuDecodedData *destination, int channels);
int SpuSetTransferMode(int mode);
unsigned int SpuSetTransferStartAddr(unsigned int address);
SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc callback);
int SpuIsTransferCompleted(int wait);
unsigned int SpuFlush(unsigned int events);

int SpuSetIRQ(int enabled);
unsigned int SpuSetIRQAddr(unsigned int address);
SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc callback);

void SpuSetCommonAttr(SpuCommonAttr *settings);
void SpuSetCommonMasterVolume(short left, short right);
void SpuSetCommonCDMix(int enabled);
void SpuSetCommonCDReverb(int enabled);
void SpuSetCommonCDVolume(short left, short right);
int SpuSetMute(int muted);

int SpuSetReverb(int enabled);
int SpuGetReverb(void);
int SpuSetReverbModeParam(SpuReverbAttr *settings);
void SpuGetReverbModeParam(SpuReverbAttr *settings);
int SpuSetReverbDepth(SpuReverbAttr *settings);
void SpuSetReverbModeDepth(short left, short right);
int SpuSetReverbModeType(int mode);
void SpuGetReverbModeType(int *mode);
void SpuSetReverbModeDelayTime(int delay);
void SpuSetReverbModeFeedback(int feedback);
int SpuClearReverbWorkArea(int mode);
int SpuReserveReverbWorkArea(int enabled);
int SpuIsReverbWorkAreaReserved(int enabled);
unsigned int SpuSetReverbVoice(int enabled, unsigned int channels);
unsigned int SpuGetReverbVoice(void);

void SpuSetKey(int enabled, unsigned int channels);
int SpuGetKeyStatus(unsigned int channels);
void SpuGetAllKeysStatus(char *states);
void SpuSetKeyOnWithAttr(SpuVoiceAttr *settings);
void SpuSetVoiceAttr(SpuVoiceAttr *settings);

void SpuGetVoicePitch(int channel, unsigned short *pitch);
void SpuGetVoiceVolume(int channel, short *left, short *right);
void SpuGetVoiceEnvelope(int channel, short *amplitude);
/* This output follows host long (LP64 on Linux, LLP64 on Windows). */
void SpuGetVoiceEnvelopeAttr(int channel, long *key_state, short *amplitude);
void SpuSetVoicePitch(int channel, unsigned short pitch);
void SpuSetVoiceVolume(int channel, short left, short right);
void SpuSetVoiceStartAddr(int channel, unsigned int address);
void SpuSetVoiceLoopStartAddr(int channel, unsigned int address);

void SpuSetVoiceAR(int channel, unsigned short rate);
void SpuSetVoiceDR(int channel, unsigned short rate);
void SpuSetVoiceSR(int channel, unsigned short rate);
void SpuSetVoiceRR(int channel, unsigned short rate);
void SpuSetVoiceSL(int channel, unsigned short level);
void SpuSetVoiceADSRAttr(int channel, unsigned short attack, unsigned short decay,
                         unsigned short sustain, unsigned short release, unsigned short level,
                         int attack_mode, int sustain_mode, int release_mode);

#ifdef __cplusplus
}
#endif
#endif
