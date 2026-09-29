/*
 * movie_player.c — native port of the retail movie player module.
 *
 * Retail: archive directory 0x18 file 1 (disc/movie_player.bin, 0x16000
 * bytes), placed at 0x801D3000 by MovieMain (movie.bin) and by the field
 * overlay before in-field movies.  It is Square's STR player linked with
 * Sony libpress (MDEC) and libcd cdstream.  Split by
 * config/movie_player.yaml; retail authority asm/movie_player/C4.s.
 *
 * Layers:
 *   - Square STR player (func_801D30C4 .. func_801D43B0): transcribed.
 *   - libcd cdstream ring (StSetRing/StGetNext/StFreeRing/StCdInterrupt,
 *     func_801D583C .. func_801D5D54): transcribed; the CD hardware register
 *     and DMA accesses of StCdInterrupt are replaced by CdGetSector on the
 *     PsyCross spooler (see patches/psycross_cd_stream_movie.patch).
 *   - libpress: DecDCTvlc (func_801D4CC8, handwritten asm) is transcribed
 *     register-for-register and uses the module's own lookup tables read
 *     from the archive; DecDCTin/DecDCTout/DecDCToutCallback (MDEC hardware
 *     + DMA) are implemented in software from the psx-spx MDEC description
 *     with the module's IQ and scale tables.  MDEC output completes on the
 *     next player pump instead of asynchronously.
 *
 * Not yet implemented: XA-ADPCM audio for PICTURE+ADPCM movies (the audio
 * sectors are delivered to the ring and rejected by the 0x160 check exactly
 * as the retail SF filter would drop them; no sound is produced).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "system/archive.h"

#include "psyq/libcd.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"

/* ------------------------------------------------------------------------- */
/* SLUS symbols                                                              */
/* ------------------------------------------------------------------------- */

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern s32 D_8004FE4C;          /* PC HDD stream file handle */
extern s32 D_8005A470;          /* cdstream: 1 = no back-location tracking */
extern s16 D_8005A4B8;          /* ERROR FM (frame at last CD retry) */
extern s32 D_8005A4DC;          /* ERROR count */
extern u16 D_80062514;          /* frame count reported by the last movie */

extern void SoundSetCdVolumeWithFade(s32 targetVolume, s32 fadeFrames);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveCdSetMode(u_char mode);
extern void ArchiveClearStreamFileSections(void);
extern int* ArchiveAllocStreamFile(int numEntries, int allocMode);
extern s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern int func_80028F30(u32* ppPayload, u32* ppHeader);
extern void func_800294B4(void* pSection);
extern void func_8002A498(int channel);
extern u32 func_8002C3D8(void);
extern int PClseek(int fd, int offset, int mode);

/* ------------------------------------------------------------------------- */
/* Module .data / .bss shared with the game                                  */
/* ------------------------------------------------------------------------- */

s32 D_801D68B4;                 /* 1: field mode (strip rows uploaded per 16 lines) */
s32 D_801E89D4;                 /* skipped-frame counter ("Skp") */

static u16 D_801D68B8;          /* last picture width  */
static u16 D_801D68BA;          /* last picture height */
static u16 D_801D68BC;          /* screen width in halfwords */
static u16 D_801D68BE;          /* screen height */
static s32 D_801D68C0;          /* DecDCTvlc has more to decode */
static s32 D_801D68C4;          /* DecDCTvlcSize argument */
static s32 D_801D68C8;          /* MDEC mode: bit0 24-bit, bit1 STP */
static u32 D_801D68CC;          /* end frame */

static s32 D_801E8908;          /* StCdInterrupt status code */
static u16* D_801E8910;         /* header of the frame being VLC decoded */
static u16* D_801E8914;         /* data of the frame being VLC decoded */
static s32 D_801E8918;          /* VLC buffer index */
static u16* D_801E891C[2];      /* VLC output buffers */
static s32 D_801E8924;          /* strip buffer index */
static u16* D_801E8928[2];      /* MDEC strip output buffers */
static s32 D_801E8930;          /* rect index for the frame being decoded */
static RECT D_801E8934[2];      /* frame placement: x0, y0, x1, y1 */
static s32 D_801E8944;          /* rect index for the frame being uploaded */
static RECT D_801E8948[2];      /* current strip rect: x, y, w, h */
static volatile u8 D_801E8958;  /* frame uploaded, MDEC idle */
static u8 D_801E895C;           /* waiting for frame data */
static s8 D_801E8960;
static s8 D_801E8964;           /* 0 off, 1 playing, 2 looping, -1 stopped */
static u8 D_801E8968;           /* 1: PC HDD stream */
static s32 D_801E896C;          /* sector offset */
static s32 D_801E8970;          /* CD mode */
static u16 D_801E8974;          /* file index */
static u16 D_801E8978;          /* channel */
static s16 D_801E897C;          /* draw lines (-1 all) */
static u16 D_801E8980;          /* frame number being decoded */
static s32 D_801E8984;          /* start frame */
static s32 D_801E8988;          /* last uploaded frame (-1) */
static u16* D_801E898C;         /* ring buffer */
static void (*D_801E8990)(u16 frame, u16 x, u16 y);
static u32 D_801E8994;          /* frame number of the last fetched sector group */
static s32 D_801E8998;          /* LoadImage enabled */
static s32 D_801E899C;          /* saved archive directory */
static s32 D_801E89A0;          /* saved archive entry */
static s32 D_801E89A4;          /* fade-in pending */
static s32 D_801E89A8;          /* fade-out pending */
static u8 D_801E89AC[4];        /* CdlLOC of the last completed frame */
static s32 D_801E89B0;          /* frame number of the last completed frame */
static volatile void* D_801E89B4;/* current ring slot */
static volatile u32 D_801E89B8; /* frame number being received */
static volatile s16 D_801E89BC; /* sector index expected next */
static u16 D_801E89C0;          /* strip width in halfwords */
static s32 D_801E89C4;          /* StSetStream mode bit 0 */
static void (*D_801E89C8)(void*, u8*);
static void (*D_801E89CC)(void*);
static volatile s32 D_801E89D0;
static s32 D_801E89D8;
static volatile s32 D_801E89DC; /* start frame to wait for */
s32 D_801E89E0;                /* cdstream transition gate, shared with FE60 */
static volatile s32 D_801E89E4;
static volatile s32 D_801E89E8; /* last sector of a frame in flight */
static u32 D_801E89EC;
static s32 D_801E89F4;          /* consecutive StGetNext misses */
static volatile s32 D_801E89F8; /* ring write slot */
static volatile s32 D_801E89FC; /* ring slot where the current frame starts */
static volatile s32 D_801E8A00; /* ring read slot */
static s32 D_801E8A04;          /* HDD sector buffer */
static volatile u32 D_801E8A08; /* end frame (StSetStream) */
static volatile s32 D_801E8A0C; /* waiting for start frame */
static u16* D_801E8A10;
static u16* D_801E8A14;         /* ring base */
static u32 D_801E8A18;          /* ring slot count */

/* ------------------------------------------------------------------------- */
/* Module blob (tables)                                                      */
/* ------------------------------------------------------------------------- */

#define MP_MODULE_SIZE        0x16000
#define MP_OFF_IQ             (0x801D76E4 - 0x801D3000)   /* 64 luma + 64 chroma */
#define MP_OFF_SCALE          (0x801D7768 - 0x801D3000)   /* 64 s16 */
#define MP_OFF_VLC_MAIN       (0x801D802C - 0x801D3000)   /* 8192 x 8 bytes */
#define MP_OFF_VLC_ESC        (0x801E802C - 0x801D3000)   /* 512 x 4 bytes */

static u8 s_module[MP_MODULE_SIZE];
static int s_moduleLoaded;

static void MoviePlayerLoadModule(void)
{
    int savedDir;
    int savedEntry;

    if (s_moduleLoaded)
        return;
    ArchiveGetArchiveOffsetIndices(&savedDir, &savedEntry);
    ArchiveSetIndex(0x18, 0);
    ArchiveReadFileToBuffer(1, s_module, 0, 0);
    ArchiveCdDataSync(0);
    ArchiveSetIndex(savedDir, savedEntry);
    s_moduleLoaded = 1;
    fprintf(stderr, "[movie-player] module tables loaded (archive 0x18/1)\n");
}

/* ------------------------------------------------------------------------- */
/* libpress: DecDCTvlc (func_801D4CC8, handwritten)                          */
/* ------------------------------------------------------------------------- */

static s32 s_vlcLimit = 0x00FFFFFF;      /* D_801D4C94, in halfwords */
static struct {                          /* D_801D5008 */
    const u16* bs;
    u16* out;
    u32 bits;
    s32 nbits;
    u32 q;
    s32 block;
    s32 dcCr;
    s32 dcCb;
    s32 dcY;
} s_vlc;

/* func_801D4C98: DecDCTvlcSize(words) */
static s32 DecDCTvlcSize(s32 words)
{
    s32 prev = s_vlcLimit;
    if (words - 1 <= 0) {
        s_vlcLimit = 0x00FFFFFF;
    } else {
        s_vlcLimit = words << 1;
    }
    return prev;
}

#define VLC_REFILL()                                   \
    do {                                               \
        if (v1 & 0x10) {                               \
            v1 &= 0xF;                                 \
            v0 |= (u32)*a0 << v1;                      \
            a0++;                                      \
        } else {                                       \
            v1 &= 0xF;                                 \
        }                                              \
    } while (0)

static s32 DecDCTvlc(const u16* bs, u16* out)
{
    const u8* a2 = s_module + MP_OFF_VLC_MAIN;
    const u8* a3 = s_module + MP_OFF_VLC_ESC;
    const u16* a0;
    u16* a1;
    u16* t6;
    u32 v0;
    s32 v1;
    u32 t0;
    u32 t1;
    u32 t2;
    u32 t3;
    u32 t4;
    s32 t5;
    s32 t7;
    s32 t8;
    s32 t9;
    s32 i;

    if (bs == NULL) {
        a0 = s_vlc.bs;
        a1 = s_vlc.out;
        v0 = s_vlc.bits;
        v1 = s_vlc.nbits;
        t4 = s_vlc.q;
        t5 = s_vlc.block;
        t7 = s_vlc.dcCr;
        t8 = s_vlc.dcCb;
        t9 = s_vlc.dcY;
        t6 = a1 + s_vlcLimit;
        goto ac;
    }

    t5 = 0;
    t7 = 0;
    t8 = 0;
    t9 = 0;
    a0 = bs;
    a1 = out;
    t6 = a1 + s_vlcLimit;
    t0 = a0[0];
    t1 = a0[1];
    t4 = a0[2];
    t2 = a0[3];
    v0 = a0[4];
    v1 = a0[5];
    if ((s32)(t2 - 3) >= 0) {
        t5 = 1;
    }
    t4 <<= 10;
    a0 += 6;
    v0 = (v0 << 16) | (u32)v1;
    v1 = 0;
    a1[0] = (u16)t0;
    a1[1] = (u16)t1;
    a1 += 1;

dc:
    t0 = v0 >> 22;
    if (t5 != 0) {
        const u8* at;
        const u16* e;

        a1 += 1;
        if (t0 == 0x3FF)
            goto end;
        at = (t5 - 3 < 0) ? (a2 - 0x400) : (a2 - 0x800);
        e = (const u16*)(at + ((v0 >> 24) << 2));
        t1 = e[0];
        t2 = e[1];
        t0 = 0;
        v0 <<= t1;
        if (t2 != 0) {
            u32 sh = 32 - t2;
            u32 pre = v0;

            t0 = pre >> sh;
            if ((s32)pre >= 0) {
                t3 = 0xFFFFFFFFu >> sh;
                t0 = t0 - t3;
            }
            v0 = pre << t2;
            v1 += (s32)t2;
        }
        v1 += (s32)t1;
        VLC_REFILL();
        if (t5 - 2 > 0) {
            t9 += (s32)t0;
            t1 = (u32)t9;
        } else if (t5 == 2) {
            t8 += (s32)t0;
            t1 = (u32)t8;
        } else {
            t7 += (s32)t0;
            t1 = (u32)t7;
        }
        t1 = ((t1 << 2) & 0x3FF) | t4;
        t5 += 1;
        *a1 = (u16)t1;
        if (t5 == 7) {
            t5 -= 6;
        }
    } else {
        a1 += 1;
        if (t0 == 0x1FF)
            goto end;
        v0 <<= 10;
        v1 += 10;
        VLC_REFILL();
        t0 |= t4;
        *a1 = (u16)t0;
    }

    /* .L801D4E8C */
    {
        s32 over = (s32)(a1 - t6);
        a1 += 1;
        if (over >= 0)
            goto save;
    }

ac:
    {
        const u32* e = (const u32*)(a2 + ((v0 >> 19) << 3));
        u32 at;

        t1 = e[0];
        if (t1 != 0) {
            t3 = e[1];
            at = t1 & 0xFF;
        } else {
            v0 <<= 8;
            v1 += 8;
            VLC_REFILL();
            t1 = *(const u32*)(a3 + ((v0 >> 23) << 2));
            t3 = 0;
            at = t1 & 0xFF;
        }
        v0 <<= at;
        v1 += (s32)at;
        VLC_REFILL();
        t1 >>= 16;
        if (t1 == 0x7C1F)
            goto escape;
        *a1 = (u16)t1;
        if (t1 == 0xFE00)
            goto dc;
        a1 += 1;
        if (t3 == 0)
            goto ac;
        t2 = t3 & 0xFFFF;
        if (t2 == 0x7C1F)
            goto escape;
        *a1 = (u16)t2;
        if (t2 == 0xFE00)
            goto dc;
        a1 += 1;
        t2 = t3 >> 16;
        if (t2 == 0)
            goto ac;
        if (t2 == 0x7C1F)
            goto escape;
        *a1 = (u16)t2;
        if (t2 == 0xFE00)
            goto dc;
        a1 += 1;
        goto ac;
    }

escape:
    *a1 = (u16)(v0 >> 16);
    a1 += 1;
    t0 = *a0;
    a0++;
    v0 <<= 16;
    v0 |= t0 << v1;
    goto ac;

end:
    for (i = 0; i <= 0x40; i++) {
        *a1++ = 0xFE00;
    }
    return 0;

save:
    s_vlc.bs = a0;
    s_vlc.out = a1;
    s_vlc.bits = v0;
    s_vlc.nbits = v1;
    s_vlc.q = t4;
    s_vlc.block = t5;
    s_vlc.dcCr = t7;
    s_vlc.dcCb = t8;
    s_vlc.dcY = t9;
    return 1;
}

/* ------------------------------------------------------------------------- */
/* libpress: software MDEC (DecDCTin / DecDCTout / DecDCToutCallback)        */
/* ------------------------------------------------------------------------- */

static const u8 s_zagzig[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

static const u16* s_mdecRl;      /* RL stream cursor (after the command word) */
static const u16* s_mdecRlEnd;
static u32 s_mdecCmd;
static void (*s_mdecOutCallback)(void);
static volatile int s_mdecOutPending;

static s32 MdecSigned10(u32 v)
{
    return (s32)(v << 22) >> 22;
}

/* psx-spx rl_decode_block */
static void MdecRlDecodeBlock(s32 blk[64], const u8* qt)
{
    u32 n;
    s32 k;
    s32 qScale;
    s32 val;

    memset(blk, 0, sizeof(s32) * 64);
    if (s_mdecRl >= s_mdecRlEnd)
        return;
    n = *s_mdecRl++;
    while (n == 0xFE00) {
        if (s_mdecRl >= s_mdecRlEnd)
            return;
        n = *s_mdecRl++;
    }
    k = 0;
    qScale = (n >> 10) & 0x3F;
    val = MdecSigned10(n & 0x3FF) * qt[k];
    for (;;) {
        if (qScale == 0)
            val = MdecSigned10(n & 0x3FF) * 2;
        if (val < -0x400)
            val = -0x400;
        if (val > 0x3FF)
            val = 0x3FF;
        if (qScale > 0)
            blk[s_zagzig[k]] = val;
        else
            blk[k] = val;
        if (s_mdecRl >= s_mdecRlEnd)
            return;
        n = *s_mdecRl++;
        k += ((n >> 10) & 0x3F) + 1;
        if (k > 63)
            return;
        val = (MdecSigned10(n & 0x3FF) * qt[k] * qScale + 4) >> 3;
    }
}

/* psx-spx real_idct_core with the module's scale table */
static void MdecIdct(s32 blk[64])
{
    const s16* scale = (const s16*)(s_module + MP_OFF_SCALE);
    s32 tmp[64];
    s32* src = blk;
    s32* dst = tmp;
    int pass;
    int x;
    int y;
    int z;

    for (pass = 0; pass < 2; pass++) {
        for (x = 0; x < 8; x++) {
            for (y = 0; y < 8; y++) {
                s32 sum = 0;
                for (z = 0; z < 8; z++) {
                    sum += src[y + z * 8] * (scale[x + z * 8] / 8);
                }
                dst[x + y * 8] = (sum + 0xFFF) >> 13;
            }
        }
        {
            s32* t = src;
            src = dst;
            dst = t;
        }
    }
    /* after two passes the result is back in blk */
}

static u8 MdecClampSigned(s32 v, int signedOut)
{
    if (v < -128)
        v = -128;
    if (v > 127)
        v = 127;
    if (!signedOut)
        v ^= 0x80;
    return (u8)v;
}

/* psx-spx yuv_to_rgb into a 16x16 RGB888 macroblock */
static void MdecYuvToRgb(const s32* cr, const s32* cb, const s32* yb, int xx, int yy,
                         u8 rgb[16 * 16 * 3], int signedOut)
{
    int x;
    int y;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            s32 R = cr[((x + xx) / 2) + ((y + yy) / 2) * 8];
            s32 B = cb[((x + xx) / 2) + ((y + yy) / 2) * 8];
            s32 G = (-(B * 0x580) - (R * 0xB6E)) >> 12;
            s32 Y = yb[x + y * 8];
            u8* p = &rgb[((x + xx) + (y + yy) * 16) * 3];

            R = (R * 0x166E) >> 12;
            B = (B * 0x1C5A) >> 12;
            p[0] = MdecClampSigned(Y + R, signedOut);
            p[1] = MdecClampSigned(Y + G, signedOut);
            p[2] = MdecClampSigned(Y + B, signedOut);
        }
    }
}

static void MdecDecodeMacroblock(u8 rgb[16 * 16 * 3], int signedOut)
{
    const u8* iqY = s_module + MP_OFF_IQ;
    const u8* iqC = s_module + MP_OFF_IQ + 64;
    s32 cr[64];
    s32 cb[64];
    s32 yb[64];

    MdecRlDecodeBlock(cr, iqC);
    MdecIdct(cr);
    MdecRlDecodeBlock(cb, iqC);
    MdecIdct(cb);
    MdecRlDecodeBlock(yb, iqY);
    MdecIdct(yb);
    MdecYuvToRgb(cr, cb, yb, 0, 0, rgb, signedOut);
    MdecRlDecodeBlock(yb, iqY);
    MdecIdct(yb);
    MdecYuvToRgb(cr, cb, yb, 8, 0, rgb, signedOut);
    MdecRlDecodeBlock(yb, iqY);
    MdecIdct(yb);
    MdecYuvToRgb(cr, cb, yb, 0, 8, rgb, signedOut);
    MdecRlDecodeBlock(yb, iqY);
    MdecIdct(yb);
    MdecYuvToRgb(cr, cb, yb, 8, 8, rgb, signedOut);
}

/* func_801D4534: DecDCTReset */
static void DecDCTReset(u32 mode)
{
    (void)mode;
    s_mdecRl = NULL;
    s_mdecRlEnd = NULL;
    s_mdecOutPending = 0;
}

/* func_801D46A0: DecDCTin(buf, mode).  Retail rewrites the command word's
 * depth (bit 27 cleared for 24-bit) and STP (bit 25) bits, then DMAs
 * (cmd & 0xFFFF) words after the command word into the MDEC. */
static void DecDCTin(u32* buf, s32 mode)
{
    u32 cmd = *buf;

    if (mode & 1)
        cmd &= ~0x08000000u;
    else
        cmd |= 0x08000000u;
    if (mode & 2)
        cmd |= 0x02000000u;
    else
        cmd &= ~0x02000000u;
    *buf = cmd;
    s_mdecCmd = cmd;
    s_mdecRl = (const u16*)(buf + 1);
    s_mdecRlEnd = s_mdecRl + (cmd & 0xFFFF) * 2;
}

/* func_801D471C: DecDCTout(dst, words) — decode enough macroblocks to fill
 * `words` 32-bit words, then raise the (deferred) DMA-complete callback. */
static void DecDCTout(u32* dst, s32 words)
{
    u8 rgb[16 * 16 * 3];
    u8* out = (u8*)dst;
    s32 bytes = words * 4;
    int depth = (s_mdecCmd >> 27) & 3;      /* 2 = 24-bit, 3 = 15-bit */
    int signedOut = (s_mdecCmd >> 26) & 1;
    int stp = (s_mdecCmd >> 25) & 1;
    s32 mbBytes = (depth == 2) ? 16 * 16 * 3 : 16 * 16 * 2;

    while (bytes >= mbBytes) {
        int i;

        MdecDecodeMacroblock(rgb, signedOut);
        if (depth == 2) {
            memcpy(out, rgb, mbBytes);
        } else {
            u16* p16 = (u16*)out;
            for (i = 0; i < 16 * 16; i++) {
                u32 r = rgb[i * 3 + 0] >> 3;
                u32 g = rgb[i * 3 + 1] >> 3;
                u32 b = rgb[i * 3 + 2] >> 3;
                p16[i] = (u16)((stp << 15) | (b << 10) | (g << 5) | r);
            }
        }
        out += mbBytes;
        bytes -= mbBytes;
    }
    s_mdecOutPending = 1;
}

/* func_801D47D8: DecDCToutCallback */
static void DecDCToutCallback(void (*func)(void))
{
    s_mdecOutCallback = func;
}

/* Deliver pending MDEC completions.  Each callback may issue the next
 * DecDCTout, so drain until the frame is uploaded. */
static void MoviePlayerMdecService(void)
{
    while (s_mdecOutPending) {
        s_mdecOutPending = 0;
        if (s_mdecOutCallback)
            s_mdecOutCallback();
    }
}

/* ------------------------------------------------------------------------- */
/* libcd cdstream ring (func_801D583C .. func_801D5D54)                      */
/* ------------------------------------------------------------------------- */

#define ST_SLOT_HEADER  0x20
#define ST_SLOT_DATA    0x7E0

typedef struct {
    u16 status;      /* 0 free, 1 wrap/end marker, 2 ready, 3 receiving, 4 consumed
                        (aliased with the STR 0x0160 status word while receiving) */
    u16 type;
    u16 sectorIndex;
    u16 sectorCount;
    u32 frameNumber;
    u32 frameSize;
    u16 width;
    u16 height;
    u16 blocks;
    u16 magic3800;
    u16 qscale;
    u16 version;
    u8 loc[4];
} StSlot;

static u8 s_sector[2048];       /* spooler-thread copy of the current sector */

static volatile StSlot* StSlotAt(s32 i)
{
    return (volatile StSlot*)((u8*)D_801E8A14 + (i << 5));
}

static u16* StDataAt(s32 i)
{
    return (u16*)((u8*)D_801E8A14 + (D_801E8A18 << 5) + i * ST_SLOT_DATA);
}

/* func_801D5C34 */
static void StClearSlots(s32 first, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++) {
        StSlotAt(first + (s32)i)->status = 0;
    }
}

/* func_801D5920: StClearRing */
static void StClearRing(void)
{
    D_801E8A00 = 0;
    D_801E89FC = 0;
    D_801E89F8 = 0;
    D_801E89E8 = 0;
    StClearSlots(0, D_801E8A18);
    D_801E89D0 = 0;
    D_801E89BC = 0;
    D_801E89B8 = 0;
}

/* func_801D583C: StSetRing */
static void StSetRing(u16* ring, u32 slots)
{
    D_801E8A14 = ring;
    D_801E8A18 = slots;
    StClearRing();
}

/* func_801D5D34: StSetMask */
static void StSetMask(s32 mask, s32 startFrame, u32 endFrame)
{
    D_801E8A0C = mask;
    D_801E89DC = startFrame;
    D_801E8A08 = endFrame;
}

/* func_801D5AF4: StSetStream */
static void StSetStream(s32 mode, s32 startFrame, u32 endFrame,
                        void (*func1)(void*, u8*), void (*func2)(void*))
{
    StSetMask(1, startFrame, endFrame);
    D_801E8A04 = 0;
    D_801E89C8 = func1;
    D_801E89C4 = mode & 1;
    D_801E89E4 = 0;
    D_801E89D8 = 0;
    D_801E89BC = 0;
    D_801E89B8 = 0;
    D_801E89CC = func2;
}

/* func_801D5C70: StGetNext */
static s32 StGetNext(u16** ppData, u16** ppHeader)
{
    volatile StSlot* slot = StSlotAt(D_801E8A00);

    if (slot->status == 1) {
        D_801E8A00 = 0;
        if (D_801E8A08 != 0)
            slot->status = 0;
        slot = StSlotAt(D_801E8A00);
    }
    if (slot->status == 2) {
        slot->status = 4;
        *ppData = StDataAt(D_801E8A00);
        *ppHeader = (u16*)slot;
        return 0;
    }
    return 1;
}

/* func_801D5B7C: StFreeRing */
static s32 StFreeRing(u16* pData)
{
    s32 index = (s32)(((u8*)pData - ((u8*)D_801E8A14 + (D_801E8A18 << 5))) / ST_SLOT_DATA);
    volatile StSlot* slot = StSlotAt(index);
    u16 count = slot->sectorCount;
    s32 i = 0;

    if (slot->status != 4)
        return 1;
    for (i = 0; i < count; i++) {
        StSlotAt(index + i)->status = 0;
    }
    D_801E8A00 = index + i;
    return 0;
}

/* func_801D5A04: data-transfer complete for the last sector of a frame */
static void StFrameReady(void)
{
    volatile StSlot* slot = StSlotAt(D_801E89FC);

    slot->status = 2;
    memcpy(D_801E89AC, (const void*)slot->loc, 4);
    D_801E89B0 = (s32)slot->frameNumber;
    D_801E89FC = D_801E89F8;
    if (D_801E89C8 != NULL)
        D_801E89C8((void*)D_801E89C8, D_801E89AC);
    D_801E89E8 = 0;
}

/* func_801D5A94: StGetBackloc */
static s16 StGetBackloc(u8* loc)
{
    if (D_8005A470 == 0) {
        CdIntToPos(CdPosToInt((CdlLOC*)D_801E89AC) + 1, (CdlLOC*)loc);
        return (s16)D_801E89B0;
    }
    return -1;
}

/* func_801D5D54: StCdInterrupt — ready callback per delivered sector.  The
 * retail body reads the sector through the CD FIFO/DMA registers; here it
 * comes from the PsyCross spooler via CdGetSector. */
static void StCdInterrupt(u_char status, u_char* result)
{
    volatile StSlot* slot;
    u8 loc[4];
    int sector;
    u8 hdr[ST_SLOT_HEADER];

    (void)result;
    if (D_801E89E8 == 1)
        return;
    if (status & 4) {
        D_801E8908 = 3;
        return;
    }

    slot = StSlotAt(D_801E89F8);
    D_801E89B4 = slot;
    if (slot->status != 0) {
        D_801E8908 = 4;
        return;
    }

    /* Retail pulls 8 words into the slot header and, later, the remaining
     * 0x1F8 words into the data area from the CD FIFO.  PsyCross's
     * CdGetSector always copies from the start of the delivered sector, so
     * take the whole 2048 bytes once and split them. */
    CdGetSector(s_sector, 2048 / 4);
    memcpy(hdr, s_sector, ST_SLOT_HEADER);
    memcpy((void*)slot, hdr, ST_SLOT_HEADER);
    {
        extern int g_cdCurrentSector;
        sector = g_cdCurrentSector;
    }
    CdIntToPos(sector, (CdlLOC*)loc);
    memcpy((void*)slot->loc, loc, 4);

    if (D_801E8A0C == 1 && D_801E89DC != 0) {
        if ((u32)D_801E89DC != slot->frameNumber) {
            slot->status = 0;
            return;
        }
        D_801E8A0C = 0;
    }

    if (slot->status != 0x160 || ((slot->type >> 10) & 0x1F) != (u32)D_801E89E4) {
        D_801E8908 = 5;
        slot->status = 0;
        return;
    }
    if (D_801E89BC != (s16)slot->sectorIndex ||
        (D_801E89B8 != 0 && D_801E89B8 != slot->frameNumber)) {
        D_801E89B8 = 0;
        D_801E89BC = 0;
        StClearSlots(D_801E89FC, (u32)(D_801E89F8 - D_801E89FC));
        D_801E89F8 = D_801E89FC;
        slot->status = 0;
        D_801E8908 = 6;
        return;
    }

    if (slot->sectorIndex == 0) {
        D_801E89BC = 0;
        D_801E89B8 = slot->frameNumber & 0xFFFF;
        if (D_801E8A08 != 0 && D_801E89B8 >= D_801E8A08) {
            D_801E89B8 = 0;
            D_801E89BC = 0;
            StClearSlots(D_801E89FC, (u32)(D_801E89F8 - D_801E89FC));
            D_801E89F8 = D_801E89FC;
            slot->status = 0;
            D_801E8A0C = 1;
            if (D_801E89CC != NULL)
                D_801E89CC(NULL);
            D_801E8908 = 7;
            return;
        }
        if ((D_801E8A18 - (u32)D_801E89F8 - 1) < slot->sectorCount) {
            if (D_801E8A08 == 0) {
                slot->status = 1;
                D_801E8A0C = 1;
                if (D_801E89CC != NULL)
                    D_801E89CC((void*)slot);
                D_801E8908 = 8;
                return;
            }
            if (StSlotAt(0)->status != 0) {
                slot->status = 0;
                D_801E8908 = 9;
                return;
            }
            slot->status = 1;
            memcpy((void*)StSlotAt(0), (const void*)slot, ST_SLOT_HEADER);
            D_801E89F8 = 0;
            slot = StSlotAt(0);
            D_801E89B4 = slot;
        }
        D_801E89FC = D_801E89F8;
    }

    D_801E8908 = 0xA;
    D_801E89BC = D_801E89BC + 1;
    D_801E8A10 = StDataAt(D_801E89F8);
    memcpy(D_801E8A10, s_sector + ST_SLOT_HEADER, ST_SLOT_DATA);
    if (slot->sectorCount - 1 == slot->sectorIndex) {
        D_801E89E8 = 1;
        D_801E89BC = 0;
        D_801E89B8 = 0;
        D_801E89E4 = D_801E89D8;
    }
    slot->status = 3;
    D_801E89F8 += 1;
    if (D_801E89E8 != 0)
        StFrameReady();
}

/* func_801D586C: install the stream callbacks and start CdlReadS */
static s32 StStartRead(s32 mode)
{
    u8 param = (u8)mode;

    CdControl(CdlSetmode, &param, NULL);
    if (mode & 0x100) {
        D_8005A470 = (mode & 0x20) ? 0 : 1;
        CdDataCallback(NULL);
        CdReadyCallback(StCdInterrupt);
    }
    return CdControl(CdlReadS, NULL, NULL);
}

/* func_801D5980: StUnSetRing */
static void StUnSetRing(void)
{
    CdDataCallback(NULL);
    CdReadyCallback(NULL);
}

/* ------------------------------------------------------------------------- */
/* Square STR player                                                         */
/* ------------------------------------------------------------------------- */

/* func_801D41AC: (re)start streaming the file */
static void MpStartStream(u16 fileIndex, s32 sectorOffset, u16 channel, s32 mode, u8* loc)
{
    int savedDir;
    int savedEntry;
    u8 pos[4];

    SoundSetCdVolumeWithFade(0, 0);
    func_8002A498(0);
    ArchiveCdDataSync(0);
    ArchiveGetArchiveOffsetIndices(&savedDir, &savedEntry);
    ArchiveSetIndex(D_801E899C, D_801E89A0);
    D_801E89A4 = 1;
    D_801E89A8 = 1;
    if (D_801E8968 != 0) {
        ArchiveReadFileToBuffer(fileIndex, D_801E898C, channel, mode);
        PClseek(D_8004FE4C, (mode & 8) ? sectorOffset * 0x920 : sectorOffset << 11, 0);
    } else {
        CdIntToPos(ArchiveDecodeSector(fileIndex) + sectorOffset, (CdlLOC*)pos);
        if (loc == NULL)
            loc = pos;
        while (CdControlB(CdlSetloc, loc, NULL) == 0) {
        }
        while (StStartRead(mode | 0x80) == 0) {
        }
    }
    ArchiveSetIndex(savedDir, savedEntry);
}

/* func_801D4318: stop */
void func_801D4318(void)
{
    SoundSetCdVolumeWithFade(0, 0);
    func_8002A498(0);
    DecDCToutCallback(NULL);
    DecDCTReset(0);
    D_801E8964 = -1;
    if (D_801E8968 != 0) {
        ArchiveClearStreamFileSections();
    } else {
        StUnSetRing();
        while (CdControlB(CdlPause, NULL, NULL) == 0) {
        }
        ArchiveCdSetMode(0xA0);
    }
    ArchiveCdDataSync(0);
}

/* func_801D43B0: stop and release buffers */
void func_801D43B0(void)
{
    func_801D4318();
    HeapFree(D_801E891C[0]);
    HeapFree(D_801E891C[1]);
    HeapFree(D_801E8928[0]);
    HeapFree(D_801E8928[1]);
    HeapFree(D_801E898C);
    D_801E891C[0] = NULL;
    D_801E891C[1] = NULL;
    D_801E8928[0] = NULL;
    D_801E8928[1] = NULL;
    D_801E898C = NULL;
}

static void MoviePlayerCapture(u16 frame);

/* func_801D30C4: DecDCTout complete — upload the strip, continue or finish */
static void MpStripComplete(void)
{
    s32 idx = D_801E8944;
    RECT* strip = &D_801E8948[idx];
    u16 fullHeight = (u16)strip->h;

    /* Retail re-runs a CD interrupt it had deferred while the MDEC DMA was
     * busy (D_801E89D0).  The native ring never defers, so that branch is
     * not reproduced here. */
    if (D_801E897C >= 0 && D_801E897C < (s16)fullHeight) {
        strip->h = D_801E897C;
    }

    if (D_801D68B4 != 0) {
        RECT part;
        s32 rows = strip->w / D_801E89C0;
        s32 i;
        s32 line = 0;

        part.y = strip->y;
        part.w = D_801E89C0;
        part.h = strip->h;
        for (i = 0; i < rows; i++) {
            part.x = strip->x;
            strip->x += D_801E89C0;
            if (D_801E8998 != 0) {
                LoadImage((RECT16*)&part,
                          (u_long*)((u8*)D_801E8928[D_801E8924] + line * D_801E89C0 * 2));
            }
            line += fullHeight;
        }
    } else {
        if (D_801E8998 != 0) {
            LoadImage((RECT16*)strip, (u_long*)D_801E8928[D_801E8924]);
        }
        strip->x += strip->w;
    }
    strip->h = fullHeight;
    D_801E8924 = 1 - D_801E8924;

    if (strip->x >= D_801E8934[idx].w) {
        if (D_801E8990 != NULL) {
            u16 x;
            u16 y = D_801E8934[idx].y;

            if (D_801D68C8 & 1) {
                x = (u16)((D_801E8934[idx].x * 2) / 3);
            } else {
                x = D_801E8934[idx].x;
            }
            D_801E8990(D_801E8980, x, y);
        }
        MoviePlayerCapture(D_801E8980);
        D_801E8958 = 1;
        D_801E8988 = D_801E8980;
        D_801E8998 = D_801E89E0;
        D_801E8944 = 1 - D_801E8944;
        return;
    }
    {
        s32 words = (strip->w * strip->h) / 2;
        DecDCTout((u32*)D_801E8928[D_801E8924], words);
    }
}

/* func_801D3538: initialise buffers for a width x height picture */
s32 func_801D3538(s32 width, s32 height, s32 vlcScale, s32 stripWidth,
                  s32 ringSectors, s32 sectorSize, s32 mode)
{
    u32 w16 = (u32)width & 0xFFFF;
    u32 strip16 = (u32)stripWidth & 0xFFFF;
    s32 vlcBytes;
    s32 stripBytes;

    MoviePlayerLoadModule();

    D_801E8968 = (func_8002C3D8() != 0) ? 1 : 0;
    D_801E8964 = 0;
    if (D_801D68B4 != 0) {
        strip16 = w16;
    }
    DecDCTReset(0);
    vlcBytes = (s32)(w16 * (u32)(height & 0xFFFF) * ((u32)(vlcScale & 0xFFFF) * 2));
    D_801D68C4 = sectorSize;
    D_801D68C8 = mode & 3;
    vlcBytes = (vlcBytes < 0 ? vlcBytes + 0xFF : vlcBytes) >> 8;
    D_801E891C[0] = HeapAlloc(vlcBytes, 0);
    D_801E891C[1] = HeapAlloc(vlcBytes, 0);
    if (D_801D68C8 & 1) {
        strip16 = ((strip16 & 0xFFFF) * 3) >> 1;
        w16 = (w16 * 3) >> 1;
    }
    D_801D68BC = (u16)w16;
    D_801D68BE = (u16)height;
    stripBytes = (s32)((strip16 & 0xFFFF) * (u32)(height & 0xFFFF) * 2);
    D_801E8928[0] = HeapAlloc(stripBytes, 0);
    D_801E8928[1] = HeapAlloc(stripBytes, 0);

    D_801E8934[0].x = 0;
    D_801E8934[0].y = 0;
    D_801E8934[0].w = (s16)w16;
    D_801E8934[0].h = (s16)height;
    D_801E8934[1].x = 0;
    D_801E8934[1].y = 0;
    D_801E8934[1].w = (s16)w16;
    D_801E8934[1].h = (s16)height;
    D_801E8948[0].x = 0;
    D_801E8948[0].y = 0;
    D_801E8948[0].w = (s16)strip16;
    D_801E8948[0].h = (s16)height;
    D_801E8948[1].x = 0;
    D_801E8948[1].y = 0;
    D_801E8948[1].w = (s16)strip16;
    D_801E8948[1].h = (s16)height;

    if (D_801E8968 != 0) {
        D_801E898C = (u16*)ArchiveAllocStreamFile(ringSectors & 0xFFFF, 0);
    } else {
        u32 slots = (u32)ringSectors & 0xFFFF;
        D_801E898C = HeapAlloc(slots << 11, 0);
        StSetRing(D_801E898C, slots);
    }
    if (D_801E898C != NULL) {
        D_801E8964 = 1;
        return 0;
    }
    return -1;
}

/* func_801D37CC: start playing a file */
void func_801D37CC(u16 fileIndex, s32 sectorOffset, u16 startFrame, u16 endFrame, u16 channel,
                   s32 flags, u16 loop, u16 x0, u16 y0, u16 x1, u16 y1, u16 lines,
                   void (*callback)(u16, u16, u16))
{
    if (D_801E8964 == 0)
        return;

    ArchiveGetArchiveOffsetIndices(&D_801E899C, &D_801E89A0);
    DecDCToutCallback(MpStripComplete);
    D_801E8964 = (loop == 0) ? 1 : 2;
    D_801E8960 = 0;
    D_801E897C = (s16)lines;
    D_801E8974 = fileIndex;
    D_801E896C = sectorOffset;
    if (D_801E8968 != 0) {
        if (flags & 1) {
            D_801E8978 = channel;
            D_801E8970 = 0x248;
        } else {
            D_801E8978 = 1;
            D_801E8970 = 0x200;
        }
        ArchiveClearStreamFileSections();
        D_8005A4B8 = 0;
        D_80062514 = 0;
    } else {
        if (flags & 1) {
            CdlFILTER filter;

            D_801E8978 = channel;
            D_801E8970 = 0x148;
            filter.file = 1;
            filter.chan = (u_char)D_801E8978;
            filter.pad = 0;
            while (CdControlB(CdlSetfilter, (u_char*)&filter, NULL) == 0) {
            }
        } else {
            D_801E8970 = 0x100;
        }
        StSetStream(D_801D68C8 & 1, startFrame, (u32)-1, NULL, NULL);
    }
    if (flags & 2) {
        D_801E8970 &= ~0x40;
    }

    /* Two VRAM placements, alternated per frame (rect index D_801E8930 /
     * D_801E8944): frames go to (x0,y0) and (x1,y1) in turn.  The .w/.h
     * fields hold the right/bottom edges once the picture size is known. */
    D_801E8990 = callback;
    if (D_801D68C8 & 1) {
        D_801E8934[0].x = (s16)((x0 * 3) >> 1);
        D_801E8934[1].x = (s16)((x1 * 3) >> 1);
        D_801E8934[0].y = (s16)y0;
        D_801E89C0 = 0x18;
    } else {
        D_801E89C0 = 0x10;
        D_801E8934[0].x = (s16)x0;
        D_801E8934[0].y = (s16)y0;
        D_801E8934[1].x = (s16)x1;
    }
    D_801E8934[1].y = (s16)y1;
    D_801E8958 = 1;
    D_801E895C = 1;
    D_801E8988 = -1;
    D_801E89E0 = 1;
    D_801E8998 = 1;
    D_801E8984 = startFrame;
    D_801E8918 = 0;
    D_801E8924 = 0;
    D_801E8930 = 0;
    D_801E8944 = 0;
    D_801D68B8 = 0;
    D_801D68BA = 0;
    D_801E89F4 = 0;
    D_801D68C0 = 0;
    D_801E89D4 = 0;
    D_801D68CC = endFrame;
    MpStartStream(fileIndex, sectorOffset, D_801E8978, D_801E8970, NULL);
}

/* func_801D3B00: fetch the next complete frame from the ring */
static u16* MpGetFrame(u32 endFrame, u16** ppHeader)
{
    u16* data;
    u16* header;
    u32 frame;

    (void)endFrame;
    if (D_801E8968 != 0) {
        u32 payloadAddress;
        u32 headerAddress;
        if (func_80028F30(&payloadAddress, &headerAddress) != 0)
            return NULL;
        data = (u16*)(uintptr_t)payloadAddress;
        header = (u16*)(uintptr_t)headerAddress;
        frame = *(u32*)(header + 4);
        D_801E8994 = frame;
        if (frame >= endFrame)
            func_8002A498(0);
    } else {
        if (StGetNext(&data, &header) != 0) {
            D_801E89F4 += 1;
            return NULL;
        }
        frame = *(u32*)(header + 4);
        D_801E89F4 = 0;
        D_801E89EC = D_801E8994;
        if (D_801E8994 + 1 < frame)
            D_801E89D4 += 1;
        D_801E8994 = frame;
    }

    if (D_801D68B8 != header[8] || D_801D68BA != header[9]) {
        D_801D68B8 = header[8];
        D_801D68BA = header[9];
        if (D_801D68C8 & 1) {
            s32 w16 = (D_801D68B8 * 3) >> 1;
            D_801E8934[1].w = (s16)(D_801E8934[1].x + w16);
            D_801E8934[0].w = (s16)(D_801E8934[0].x + w16);
        } else {
            D_801E8934[0].w = (s16)(D_801E8934[0].x + D_801D68B8);
            D_801E8934[1].w = (s16)(D_801E8934[1].x + D_801D68B8);
        }
        D_801E8934[0].h = (s16)(D_801E8934[0].y + D_801D68BA);
        D_801E8934[1].h = (s16)(D_801E8934[1].y + D_801D68BA);
        if (D_801D68BE < D_801D68BA) {
            D_801E8948[0].h = (s16)D_801D68BE;
            D_801E8948[1].h = (s16)D_801D68BE;
        } else {
            D_801E8948[0].h = (s16)D_801D68BA;
            D_801E8948[1].h = (s16)D_801D68BA;
        }
    }
    *ppHeader = header;
    return data;
}

/* func_801D3D54: one decode step (MDEC issue for the finished frame, VLC of
 * the next) */
static void MpDecodeStep(void)
{
    u16* vlcIn;
    u16* vlcOut;

    if (D_801D68C0 == 0) {
        if (D_801E895C == 0) {
            s32 idx = D_801E8930;
            s32 words;

            D_801E8948[idx].x = D_801E8934[idx].x;
            D_801E8948[idx].y = D_801E8934[idx].y;
            D_801E8980 = (u16)D_801E8994;
            DecDCTin((u32*)D_801E891C[D_801E8918], D_801D68C8);
            words = (D_801E8948[idx].w * D_801E8948[idx].h) / 2;
            DecDCTout((u32*)D_801E8928[D_801E8924], words);
            D_801E8958 = 0;
            D_801E8930 = 1 - D_801E8930;
            D_801E8918 = 1 - D_801E8918;
        }
        D_801E8914 = MpGetFrame(D_801D68CC, &D_801E8910);
        if (D_801E8914 == NULL) {
            D_801E895C = 1;
            return;
        }
        D_801E895C = 0;
        DecDCTvlcSize(D_801D68C4);
        vlcIn = D_801E8914;
        vlcOut = D_801E891C[D_801E8918];
    } else {
        vlcIn = NULL;
        vlcOut = NULL;
    }
    D_801D68C0 = DecDCTvlc(vlcIn, vlcOut);
    if (D_801D68C0 == 0) {
        if (D_801E8968 != 0) {
            func_800294B4(D_801E8910);
        } else {
            StFreeRing(D_801E8914);
        }
    }
}

/* DIAGNOSTIC (XENO_MOVIE_CAPTURE_DIR=<dir>, XENO_MOVIE_CAPTURE_FRAMES=a,b,c):
 * request a window screenshot when the listed frame numbers complete.
 * Removal: delete MoviePlayerCapture and its call in MpStripComplete. */
extern void PsyX_TakeScreenshotPath(const char* path)
    __asm__("_Z23PsyX_TakeScreenshotPathPKc");

static void MoviePlayerCapture(u16 frame)
{
    static int s_enabled = -1;
    static const char* s_dir;
    static const char* s_frames;

    if (s_enabled < 0) {
        s_dir = getenv("XENO_MOVIE_CAPTURE_DIR");
        s_frames = getenv("XENO_MOVIE_CAPTURE_FRAMES");
        s_enabled = (s_dir && s_dir[0] && s_frames && s_frames[0]) ? 1 : 0;
    }
    if (!s_enabled)
        return;
    {
        const char* p = s_frames;
        while (*p) {
            char* end;
            long want = strtol(p, &end, 10);
            if (end == p)
                break;
            if (want == (long)frame) {
                char path[512];
                snprintf(path, sizeof(path), "%s/movie-frame-%03u.png", s_dir, (unsigned)frame);
                PsyX_TakeScreenshotPath(path);
                fprintf(stderr, "[movie-capture] frame %u -> %s\n", (unsigned)frame, path);
                break;
            }
            p = (*end == ',') ? end + 1 : end;
        }
    }
}

/* DIAGNOSTIC (XENO_MOVIE_DIAG=1): periodic stream/ring state on stderr.
 * Removal: delete MoviePlayerDiag and its call in func_801D3F7C. */
static void MoviePlayerDiag(void)
{
    static int s_enabled = -1;
    static int s_pumps;

    if (s_enabled < 0) {
        const char* e = getenv("XENO_MOVIE_DIAG");
        s_enabled = (e && e[0] && e[0] != '0') ? 1 : 0;
    }
    if (!s_enabled)
        return;
    if ((++s_pumps % 60) == 0) {
        fprintf(stderr,
                "[movie-diag] pumps=%d lastframe=%d shown=%d state=%d cdstat=%d "
                "ring rd=%d wr=%d start=%d misses=%d skips=%d frameDone=%d needData=%d vlc=%d\n",
                s_pumps, (int)D_801E8994, (int)D_801E8988, (int)D_801E8964, (int)D_801E8908,
                (int)D_801E8A00, (int)D_801E89F8, (int)D_801E89FC, (int)D_801E89F4,
                (int)D_801E89D4, (int)D_801E8958, (int)D_801E895C, (int)D_801D68C0);
    }
}

/* func_801D3F7C: per-frame pump */
void func_801D3F7C(void)
{
    if (D_801E8964 <= 0)
        return;

    MoviePlayerDiag();
    MoviePlayerMdecService();

    if (D_801E8984 < D_801E8988 && D_801E89A4 != 0) {
        D_801E89A4 = 0;
        SoundSetCdVolumeWithFade(0x7FFF, 0x28);
    }
    if (D_801E8988 >= (s32)D_801D68CC - 3 && D_801E89A8 != 0) {
        D_801E89A8 = 0;
        SoundSetCdVolumeWithFade(0, 0x28);
    }
    if (D_801E8988 >= (s32)D_801D68CC) {
        if (D_801E8964 == 1) {
            func_801D4318();
        } else {
            D_801E8960 = 0;
            D_801E8988 = -1;
            MpStartStream(D_801E8974, D_801E896C, D_801E8978, D_801E8970, NULL);
        }
    }
    if (D_801E8958 != 0 || D_801E895C != 0 || D_801D68C0 != 0) {
        MpDecodeStep();
    }
    if (D_801E89F4 >= 0x871) {
        u8 loc[4];
        s16 frame;
        u8* restart;

        D_801E89F4 = 0;
        frame = StGetBackloc(loc);
        D_8005A4B8 = frame;
        D_8005A4DC += 1;
        D_8005A4A8 = (u32)CdPosToInt((CdlLOC*)loc);
        D_8005A4B4 = (u32)D_801E896C;
        restart = loc;
        if ((s32)D_801D68CC < frame || frame <= 0)
            restart = NULL;
        D_801E8988 = -1;
        MpStartStream(D_801E8974, D_801E896C, D_801E8978, D_801E8970, restart);
    }
}
