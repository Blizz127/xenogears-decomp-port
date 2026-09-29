/*
 * Retail intro STR player for the PC port.
 *
 * Disc1 archive file 2 is a 320x224 Type-2 .STR (magic 0x0160). Boot feeds
 * those CD sectors, expands the BS v2 Huffman stream to MDEC run-length
 * codes, inverse-DCTs, and blits 16bpp frames. This is the intro-video step
 * the skip-card used to stand in for.
 */
#include "boot_str.h"
#include "boot_assets.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "psyq/libetc.h"

#define BOOT_STR_ENTRY PC_PORT_BOOT_INTRO_STR_ENTRY
#define BOOT_STR_W 320
#define BOOT_STR_H 224
/* 16-bit tpage well right of the 320x224 draw buffers and below the kernel
 * font at (960,256). Splash used (640,0) for the Square logo; that is gone
 * before BootMain. */
#define BOOT_STR_TPAGE_X 640
#define BOOT_STR_TPAGE_Y 0

static int s_lba;
static int s_size;
static int s_nsect;
static int s_loaded;
static int s_frameCount;
static int s_lastSectors;
static int s_curOff;
static int s_played = -1; /* last decoded picture index, -1 if none */
static int s_atEnd;
static uint16_t s_pixels[BOOT_STR_W * BOOT_STR_H];

/* PSn00bSDK-style compressed VLC table (MPEG-1 AC + MDEC escape). */
#define VLC_TABLE_LEN 226
static const uint32_t s_vlcCompressed[VLC_TABLE_LEN] = {
    0x03e00000, 0x000d000b, 0x000d03f5, 0x000d2002, 0x000d23fe, 0x000d1003,
    0x000d13fd, 0x000d000a, 0x000d03f6, 0x000d0804, 0x000d0bfc, 0x000d1c02,
    0x000d1ffe, 0x000d5402, 0x000d57fe, 0x000d5001, 0x000d53ff, 0x000d0009,
    0x000d03f7, 0x000d4c01, 0x000d4fff, 0x000d4801, 0x000d4bff, 0x000d0405,
    0x000d07fb, 0x000d0c03, 0x000d0ffd, 0x000d0008, 0x000d03f8, 0x000d1802,
    0x000d1bfe, 0x000d4401, 0x000d47ff, 0x006b4001, 0x006b43ff, 0x006b1402,
    0x006b17fe, 0x006b0007, 0x006b03f9, 0x006b0803, 0x006b0bfd, 0x006b0404,
    0x006b07fc, 0x006b3c01, 0x006b3fff, 0x006b3801, 0x006b3bff, 0x006b1002,
    0x006b13fe, 0x0fe00000, 0x03e80802, 0x03e80bfe, 0x03e82401, 0x03e827ff,
    0x03e80004, 0x03e803fc, 0x03e82001, 0x03e823ff, 0x07e71c01, 0x07e71fff,
    0x07e71801, 0x07e71bff, 0x07e70402, 0x07e707fe, 0x07e71401, 0x07e717ff,
    0x01e93401, 0x01e937ff, 0x01e90006, 0x01e903fa, 0x01e93001, 0x01e933ff,
    0x01e92c01, 0x01e92fff, 0x01e90c02, 0x01e90ffe, 0x01e90403, 0x01e907fd,
    0x01e90005, 0x01e903fb, 0x01e92801, 0x01e92bff, 0x0fe60003, 0x0fe603fd,
    0x0fe61001, 0x0fe613ff, 0x0fe60c01, 0x0fe60fff, 0x1fe50002, 0x1fe503fe,
    0x1fe50801, 0x1fe50bff, 0x3fe40401, 0x3fe407ff, 0xffe2fe00, 0x7fe30001,
    0x7fe303ff, 0x03e00000, 0x00110412, 0x001107ee, 0x00110411, 0x001107ef,
    0x00110410, 0x001107f0, 0x0011040f, 0x001107f1, 0x00111803, 0x00111bfd,
    0x00114002, 0x001143fe, 0x00113c02, 0x00113ffe, 0x00113802, 0x00113bfe,
    0x00113402, 0x001137fe, 0x00113002, 0x001133fe, 0x00112c02, 0x00112ffe,
    0x00117c01, 0x00117fff, 0x00117801, 0x00117bff, 0x00117401, 0x001177ff,
    0x00117001, 0x001173ff, 0x00116c01, 0x00116fff, 0x00300028, 0x003003d8,
    0x00300027, 0x003003d9, 0x00300026, 0x003003da, 0x00300025, 0x003003db,
    0x00300024, 0x003003dc, 0x00300023, 0x003003dd, 0x00300022, 0x003003de,
    0x00300021, 0x003003df, 0x00300020, 0x003003e0, 0x0030040e, 0x003007f2,
    0x0030040d, 0x003007f3, 0x0030040c, 0x003007f4, 0x0030040b, 0x003007f5,
    0x0030040a, 0x003007f6, 0x00300409, 0x003007f7, 0x00300408, 0x003007f8,
    0x006f001f, 0x006f03e1, 0x006f001e, 0x006f03e2, 0x006f001d, 0x006f03e3,
    0x006f001c, 0x006f03e4, 0x006f001b, 0x006f03e5, 0x006f001a, 0x006f03e6,
    0x006f0019, 0x006f03e7, 0x006f0018, 0x006f03e8, 0x006f0017, 0x006f03e9,
    0x006f0016, 0x006f03ea, 0x006f0015, 0x006f03eb, 0x006f0014, 0x006f03ec,
    0x006f0013, 0x006f03ed, 0x006f0012, 0x006f03ee, 0x006f0011, 0x006f03ef,
    0x006f0010, 0x006f03f0, 0x00ee2802, 0x00ee2bfe, 0x00ee2402, 0x00ee27fe,
    0x00ee1403, 0x00ee17fd, 0x00ee0c04, 0x00ee0ffc, 0x00ee0805, 0x00ee0bfb,
    0x00ee0407, 0x00ee07f9, 0x00ee0406, 0x00ee07fa, 0x00ee000f, 0x00ee03f1,
    0x00ee000e, 0x00ee03f2, 0x00ee000d, 0x00ee03f3, 0x00ee000c, 0x00ee03f4,
    0x00ee6801, 0x00ee6bff, 0x00ee6401, 0x00ee67ff, 0x00ee6001, 0x00ee63ff,
    0x00ee5c01, 0x00ee5fff, 0x00ee5801, 0x00ee5bff
};

static uint32_t s_ac[8192];
static uint32_t s_ac00[512];
static int s_vlcReady;

static const uint8_t s_iq[64] = {
    2, 16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27,
    27, 27, 26, 26, 26, 26, 27, 27, 27, 29, 29, 29, 34, 34, 34, 29,
    29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38, 37, 35, 35, 34,
    35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83
};

static const uint8_t s_zigzag[64] = {
    0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42, 3, 8, 12, 17, 25,
    30, 41, 43, 9, 11, 18, 24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48,
    49, 57, 58, 62, 63
};

static int s_zagzig[64];
static float s_scalezag[64];

static void BootStrBuildVlc(void)
{
    static const float sf[8] = {
        1.000000000f, 1.387039845f, 1.306562965f, 1.175875602f,
        1.000000000f, 0.785694958f, 0.541196100f, 0.275899379f
    };
    uint32_t tmp[8192 + 512];
    int i, o = 0, x, y;

    if (s_vlcReady)
        return;
    for (i = 0; i < VLC_TABLE_LEN; i++) {
        uint32_t v = s_vlcCompressed[i];
        uint32_t val = v & 0x001fffff;
        int reps = (int)(v >> 21) + 1;
        int r;
        for (r = 0; r < reps && o < (int)(sizeof(tmp) / sizeof(tmp[0])); r++)
            tmp[o++] = val;
    }
    memcpy(s_ac, tmp, sizeof(s_ac));
    memcpy(s_ac00, tmp + 8192, sizeof(s_ac00));
    for (i = 0; i < 64; i++)
        s_zagzig[s_zigzag[i]] = i;
    /* nocash scalezag: scalezag[zigzag[x+y*8]] = sf[x]*sf[y]/8 */
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++)
            s_scalezag[s_zigzag[x + y * 8]] = sf[x] * sf[y] / 8.0f;
    }
    s_vlcReady = 1;
}

typedef struct {
    const uint16_t* words;
    int nwords;
    int bitpos;
} StrBits;

static uint32_t BootStrPeek(StrBits* b, int length)
{
    uint32_t v = 0;
    int bit = b->bitpos;
    int i;

    for (i = 0; i < length; i++) {
        int word = bit >> 4;
        int off = bit & 15;
        uint16_t w = (word >= 0 && word < b->nwords) ? b->words[word] : 0;
        v = (v << 1) | (uint32_t)((w >> (15 - off)) & 1);
        bit++;
    }
    return v;
}

static void BootStrAdvance(StrBits* b, int num)
{
    b->bitpos += num;
}

static int BootStrVlc(const uint16_t* words, int nwords, uint16_t qscale,
                      uint16_t* rl, int rlMax)
{
    StrBits b;
    int coeff, nrl, eob;
    uint16_t qs;
    int blk_start = 0, blk_ac = 0, blk_ac13 = 0, blk_esc = 0, blk_ac00 = 0;
    long tot_ac13 = 0, tot_esc = 0, tot_ac00 = 0, tot_ac = 0, tot_bits = 0;
    int ac_hist[64];
    int bits_min = 1 << 30, bits_max = 0;
    int dump = getenv("XENO_BOOT_VLC_BLOCK") != NULL;
    FILE* per = NULL;
    const int want_eob = (BOOT_STR_W / 16) * (BOOT_STR_H / 16) * 6;

    if (nwords < 1)
        return 0;
    memset(ac_hist, 0, sizeof(ac_hist));
    b.words = words;
    b.nwords = nwords;
    b.bitpos = 0;
    coeff = 0;
    nrl = 0;
    eob = 0;
    qs = (uint16_t)((qscale & 63) << 10);
    if (dump)
        per = fopen("scratchpad/boot_fix_20260901/vlc_block_per.log", "w");
    if (per)
        fprintf(per, "blk mb mx my type ac bits n_ac13 n_esc n_ac00\n");
    while (nrl < rlMax) {
        if (coeff) {
            if (BootStrPeek(&b, 2) == 2) {
                int bits;
                rl[nrl++] = 0xFE00;
                BootStrAdvance(&b, 2);
                bits = b.bitpos - blk_start;
                if (blk_ac < 64)
                    ac_hist[blk_ac]++;
                else
                    ac_hist[63]++;
                tot_ac += blk_ac;
                tot_ac13 += blk_ac13;
                tot_esc += blk_esc;
                tot_ac00 += blk_ac00;
                tot_bits += bits;
                if (bits < bits_min)
                    bits_min = bits;
                if (bits > bits_max)
                    bits_max = bits;
                if (per) {
                    int mb = eob / 6;
                    static const char* nm[6] = { "Cr", "Cb", "Y1", "Y2", "Y3", "Y4" };
                    fprintf(per, "%d %d %d %d %s %d %d %d %d %d\n", eob, mb,
                            mb / 14, mb % 14, nm[eob % 6], blk_ac, bits,
                            blk_ac13, blk_esc, blk_ac00);
                }
                eob++;
                coeff = 0;
                if (eob >= want_eob)
                    break;
                continue;
            }
            if (BootStrPeek(&b, 6) == 1) {
                uint32_t val = BootStrPeek(&b, 22);
                BootStrAdvance(&b, 22);
                rl[nrl++] = (uint16_t)val;
                blk_ac++;
                blk_esc++;
            } else if (BootStrPeek(&b, 8) != 0) {
                uint32_t value = s_ac[BootStrPeek(&b, 13) & 8191];
                int len = (int)(value >> 16);
                if (len <= 0 || len > 13)
                    break;
                BootStrAdvance(&b, len);
                rl[nrl++] = (uint16_t)value;
                blk_ac++;
                blk_ac13++;
            } else {
                uint32_t p17 = BootStrPeek(&b, 17);
                uint32_t value;
                int len;
                if (p17 >= 512)
                    break;
                value = s_ac00[p17];
                len = (int)(value >> 16);
                if (len <= 0 || len > 17)
                    break;
                BootStrAdvance(&b, len);
                rl[nrl++] = (uint16_t)value;
                blk_ac++;
                blk_ac00++;
            }
            coeff++;
        } else {
            uint32_t value = BootStrPeek(&b, 10);
            /* 10-bit 0x1FF is a valid DC (+511), not an end-of-frame cut. */
            rl[nrl++] = (uint16_t)(value | qs);
            BootStrAdvance(&b, 10);
            coeff = 1;
            blk_start = b.bitpos - 10;
            blk_ac = blk_ac13 = blk_esc = blk_ac00 = 0;
        }
    }
    if (per)
        fclose(per);
    if (dump && eob > 0) {
        int i;
        long codes = tot_ac13 + tot_esc + tot_ac00 + eob;
        printf("[xeno-port][boot] vlc.block eob=%d ac13=%ld esc=%ld ac00=%ld "
               "ac=%ld bits=%ld min=%d max=%d mean=%.1f\n",
               eob, tot_ac13, tot_esc, tot_ac00, tot_ac, tot_bits, bits_min,
               bits_max, tot_bits / (double)eob);
        printf("[xeno-port][boot] vlc.block paths ac13=%.1f%% esc=%.1f%% "
               "ac00=%.1f%% eob=%.1f%%\n",
               100.0 * tot_ac13 / codes, 100.0 * tot_esc / codes,
               100.0 * tot_ac00 / codes, 100.0 * eob / codes);
        printf("[xeno-port][boot] vlc.block ac_hist");
        for (i = 0; i < 64; i++) {
            if (ac_hist[i])
                printf(" %d:%d", i, ac_hist[i]);
        }
        printf("\n");
        fflush(stdout);
    }
    return nrl;
}

static int BootStrS10(uint16_t n)
{
    int v = n & 0x3FF;
    return (v & 0x200) ? v - 0x400 : v;
}

static void BootStrIdct(int* blk)
{
    float src[64], dst[64];
    int pass, i;

    for (i = 0; i < 64; i++)
        src[i] = (float)blk[i];
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < 8; i++) {
            float z10 = src[0 * 8 + i] + src[4 * 8 + i];
            float z11 = src[0 * 8 + i] - src[4 * 8 + i];
            float z13 = src[2 * 8 + i] + src[6 * 8 + i];
            float z12 = src[2 * 8 + i] - src[6 * 8 + i];
            float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, z5;
            z12 = 1.414213562f * z12 - z13;
            tmp0 = z10 + z13;
            tmp3 = z10 - z13;
            tmp1 = z11 + z12;
            tmp2 = z11 - z12;
            z13 = src[3 * 8 + i] + src[5 * 8 + i];
            z10 = src[3 * 8 + i] - src[5 * 8 + i];
            z11 = src[1 * 8 + i] + src[7 * 8 + i];
            z12 = src[1 * 8 + i] - src[7 * 8 + i];
            z5 = 1.847759065f * (z12 - z10);
            tmp7 = z11 + z13;
            tmp6 = 2.613125930f * z10 + z5 - tmp7;
            tmp5 = 1.414213562f * (z11 - z13) - tmp6;
            tmp4 = 1.082392200f * z12 - z5 + tmp5;
            dst[i * 8 + 0] = tmp0 + tmp7;
            dst[i * 8 + 7] = tmp0 - tmp7;
            dst[i * 8 + 1] = tmp1 + tmp6;
            dst[i * 8 + 6] = tmp1 - tmp6;
            dst[i * 8 + 2] = tmp2 + tmp5;
            dst[i * 8 + 5] = tmp2 - tmp5;
            dst[i * 8 + 4] = tmp3 + tmp4;
            dst[i * 8 + 3] = tmp3 - tmp4;
        }
        memcpy(src, dst, sizeof(src));
    }
    for (i = 0; i < 64; i++)
        blk[i] = (int)src[i];
}

static int BootStrRlBlock(const uint16_t* rl, int nrl, int idx, int* blk)
{
    int k, q_scale, val, n;

    memset(blk, 0, 64 * sizeof(int));
    while (idx < nrl && rl[idx] == 0xFE00)
        idx++;
    if (idx >= nrl)
        return idx;
    n = rl[idx++];
    q_scale = (n >> 10) & 0x3F;
    k = 0;
    val = BootStrS10((uint16_t)n) * s_iq[k];
    for (;;) {
        if (q_scale == 0)
            val = BootStrS10((uint16_t)n) * 2;
        if (val < -0x400)
            val = -0x400;
        if (val > 0x3FF)
            val = 0x3FF;
        /* fast_idct_core needs scalezag on the stored coefficient. */
        val = (int)((float)val * s_scalezag[k > 63 ? 63 : k]);
        if (q_scale > 0)
            blk[s_zagzig[k]] = val;
        else
            blk[k] = val;
        if (idx >= nrl)
            break;
        n = rl[idx++];
        k = k + ((n >> 10) & 0x3F) + 1;
        val = (BootStrS10((uint16_t)n) * s_iq[k > 63 ? 63 : k] * q_scale + 4) / 8;
        if (k > 63)
            break;
    }
    BootStrIdct(blk);
    return idx;
}

static int BootStrClamp(int v)
{
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return v;
}

static int BootStrSat8(int v)
{
    if (v < -128)
        return -128;
    if (v > 127)
        return 127;
    return v;
}

static void BootStrYuv(const int* cr, const int* cb, const int* y,
                       int destx, int desty, int ox, int oy, uint16_t* dst)
{
    int py, px;
    for (py = 0; py < 8; py++) {
        for (px = 0; px < 8; px++) {
            int x = destx + px;
            int yv = desty + py;
            int ci = ((px + ox) / 2) + ((py + oy) / 2) * 8;
            int R = cr[ci];
            int B = cb[ci];
            float Gf = (-0.3437f * (float)B) + (-0.7143f * (float)R);
            float Rf = 1.402f * (float)R;
            float Bf = 1.772f * (float)B;
            int Y = y[px + py * 8];
            int r = BootStrSat8((int)(Y + Rf)) + 128;
            int g = BootStrSat8((int)(Y + Gf)) + 128;
            int b = BootStrSat8((int)(Y + Bf)) + 128;
            /* Bit 15 (STP) keeps 16-bit 0 from being discarded as transparent. */
            dst[yv * BOOT_STR_W + x] = (uint16_t)(
                (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) | 0x8000);
        }
    }
}

static int BootStrDecodeBs(const uint8_t* bs, int bsLen)
{
    uint16_t qscale, ver;
    const uint16_t* words;
    int nwords, nrl, idx, mx, my, bi;
    /* 20x14 MBs * 6 blocks * 64 coeffs + EOB slack. 20k overflowed on
     * detailed Opening frames and left the right side of the picture gray. */
    static uint16_t rl[280 * 6 * 66];
    int cr[64], cb[64], y1[64], y2[64], y3[64], y4[64];

    if (bsLen < 16)
        return 0;
    memset(s_pixels, 0, sizeof(s_pixels));
    qscale = (uint16_t)(bs[4] | (bs[5] << 8));
    ver = (uint16_t)(bs[6] | (bs[7] << 8));
    (void)ver;
    words = (const uint16_t*)(bs + 8);
    nwords = (bsLen - 8) / 2;
    nrl = BootStrVlc(words, nwords, qscale, rl, (int)(sizeof(rl) / 2));
    idx = 0;
    bi = 0;
    /* BS v2 / MDEC: column-major 16x16 (ffmpeg mdec.c, jpsxdec). Y blocks
     * are TL, TR, BL, BR (psx-spx yuv_to_rgb 0,0 / 8,0 / 0,8 / 8,8). */
    for (mx = 0; mx < BOOT_STR_W / 16; mx++) {
        for (my = 0; my < BOOT_STR_H / 16; my++) {
            idx = BootStrRlBlock(rl, nrl, idx, cr);
            idx = BootStrRlBlock(rl, nrl, idx, cb);
            idx = BootStrRlBlock(rl, nrl, idx, y1);
            idx = BootStrRlBlock(rl, nrl, idx, y2);
            idx = BootStrRlBlock(rl, nrl, idx, y3);
            idx = BootStrRlBlock(rl, nrl, idx, y4);
            BootStrYuv(cr, cb, y1, mx * 16, my * 16, 0, 0, s_pixels);
            BootStrYuv(cr, cb, y2, mx * 16 + 8, my * 16, 8, 0, s_pixels);
            BootStrYuv(cr, cb, y3, mx * 16, my * 16 + 8, 0, 8, s_pixels);
            BootStrYuv(cr, cb, y4, mx * 16 + 8, my * 16 + 8, 8, 8, s_pixels);
            bi++;
        }
    }
    return bi;
}

static int BootStrReadSectors(int lba, int count, uint8_t* dst)
{
    return PcPort_BootDiscReadSectors(lba, count, dst);
}

int PcPort_BootStrLoad(void)
{
    const unsigned char* payload;
    int size;

    BootStrBuildVlc();
    if (!PcPort_BootAssetsLoad())
        return 0;
    payload = PcPort_BootIntroStrPayload(&size);
    if (!payload || size < 0x800)
        return 0;
    if ((payload[0] | (payload[1] << 8)) != 0x0160)
        return 0;
    s_lba = PcPort_BootIntroStrLba();
    s_size = PcPort_BootIntroStrFileSize();
    s_nsect = PcPort_BootIntroStrSectorCount();
    if (s_nsect <= 0)
        s_nsect = size / 0x800;
    s_curOff = 0;
    s_played = -1;
    s_atEnd = 0;
    s_frameCount = 0;
    s_loaded = 1;
    printf("[xeno-port][boot] intro STR feed file=%d lba=%d size=%d sectors=%d 320x224\n",
           BOOT_STR_ENTRY, s_lba, s_size, s_nsect);
    fflush(stdout);
    return s_loaded;
}

int PcPort_BootStrFrameCount(void)
{
    return s_frameCount;
}

int PcPort_BootStrLastSectors(void)
{
    return s_lastSectors;
}

int PcPort_BootStrFinished(void)
{
    return s_atEnd;
}

static int BootStrParsePictureHdr(const uint8_t* sec, int* ns, int* flen)
{
    unsigned mag = (unsigned)(sec[0] | (sec[1] << 8));
    unsigned typ = (unsigned)(sec[2] | (sec[3] << 8));
    unsigned sin = (unsigned)(sec[4] | (sec[5] << 8));
    unsigned n = (unsigned)(sec[6] | (sec[7] << 8));

    if (mag != 0x0160 || sin != 0 || typ != 0x8001)
        return 0;
    *ns = n > 0 ? (int)n : 1;
    *flen = (int)(sec[12] | (sec[13] << 8) | (sec[14] << 16) | (sec[15] << 24));
    return 1;
}

static void BootStrUploadBs(const uint8_t* assembled, int flen, int ns)
{
    RECT rect;

    BootStrDecodeBs(assembled, flen);
    s_lastSectors = ns;
    setRECT(&rect, BOOT_STR_TPAGE_X, BOOT_STR_TPAGE_Y, BOOT_STR_W, BOOT_STR_H);
    LoadImage(&rect, (u_long*)s_pixels);
    DrawSync(0);
}

/* Picture whose ns chunks are already contiguous in memory (title graphic
 * blob from boot_assets.c). */
static int BootStrDecodeAndUpload(const uint8_t* blob, int ns, int flen)
{
    uint8_t assembled[0x800 * 16];
    int pos = 0;
    int i;

    if (ns <= 0 || ns > 16)
        return 0;
    for (i = 0; i < ns; i++) {
        memcpy(assembled + pos, blob + i * 0x800 + 32, 0x800 - 32);
        pos += 0x800 - 32;
    }
    if (flen > pos)
        flen = pos;
    BootStrUploadBs(assembled, flen, ns);
    return 1;
}

/* Collect the ns chunks of the picture whose chunk-0 header is at sector
 * `off`, walking the disc sector by sector. The Opening STR interleaves an
 * XA audio sector (submode 0x64) every 8th sector, so a picture's chunks
 * are NOT ns consecutive sectors: e.g. picture 91 (frame_index 90) has
 * chunks 0-2 at +0..+2, audio at +3, chunks 3-8 at +4..+9. Reading ns
 * consecutive sectors copied that audio sector in as chunk 3 and dropped
 * chunk 8, which is why frames wider than three chunks broke mid-picture
 * (979 of 1680 EOBs on frame 90) and painted the rest mid-gray.
 * Same rule as silent-hill-decomp pc_port/src/fmv/str_demux.c: skip any
 * sector that is not a 0x0160/0x8001 video chunk, require chunk index ==
 * chunks seen and a matching picture number, stop after ns chunks.
 * Returns the number of bytes assembled, or -1; *span is the number of
 * sectors consumed from `off`. */
static int BootStrAssemblePicture(int off, int ns, uint8_t* assembled, int* span)
{
    uint8_t sec[0x800];
    unsigned pic = 0;
    int seen = 0, walked = 0, pos = 0;

    if (ns <= 0 || ns > 16)
        return -1;
    while (seen < ns) {
        unsigned mag, typ, chunk, num;
        /* One interleaved sector per 8 means at most 2 extra for 16 chunks;
         * allow a little more before calling the picture broken. */
        if (walked >= ns + 4 || off + walked >= s_nsect)
            return -1;
        if (BootStrReadSectors(s_lba + off + walked, 1, sec) != 0)
            return -1;
        walked++;
        mag = (unsigned)(sec[0] | (sec[1] << 8));
        typ = (unsigned)(sec[2] | (sec[3] << 8));
        if (mag != 0x0160 || typ != 0x8001)
            continue; /* XA audio or padding sector: not part of the picture */
        chunk = (unsigned)(sec[4] | (sec[5] << 8));
        num = (unsigned)(sec[8] | (sec[9] << 8) | (sec[10] << 16) |
                         ((unsigned)sec[11] << 24));
        if (seen == 0)
            pic = num;
        if (chunk != (unsigned)seen || num != pic)
            return -1;
        memcpy(assembled + pos, sec + 32, 0x800 - 32);
        pos += 0x800 - 32;
        seen++;
    }
    *span = walked;
    return pos;
}

int PcPort_BootStrPlayFrame(int frame_index)
{
    static uint8_t assembled[0x800 * 16];
    uint8_t hdr[0x800];
    int ns, flen, i;

    if (!s_loaded && !PcPort_BootStrLoad())
        return 0;
    if (frame_index < 0)
        return 0;
    if (s_played == frame_index && !s_atEnd)
        return 1;
    if (frame_index < s_played) {
        s_curOff = 0;
        s_played = -1;
        s_atEnd = 0;
    }
    while (s_played < frame_index) {
        if (s_curOff >= s_nsect) {
            s_atEnd = 1;
            return 0;
        }
        if (BootStrReadSectors(s_lba + s_curOff, 1, hdr) != 0) {
            s_atEnd = 1;
            return 0;
        }
        if (!BootStrParsePictureHdr(hdr, &ns, &flen)) {
            s_curOff++;
            continue;
        }
        if (ns <= 0)
            ns = 1;
        if (s_curOff + ns > s_nsect) {
            s_atEnd = 1;
            return 0;
        }
        if (s_played + 1 == frame_index) {
            int span = ns, pos;
            if (ns > 16) {
                s_curOff += ns;
                s_played = frame_index;
                if (s_frameCount <= s_played)
                    s_frameCount = s_played + 1;
                s_atEnd = (s_curOff >= s_nsect);
                return !s_atEnd;
            }
            pos = BootStrAssemblePicture(s_curOff, ns, assembled, &span);
            if (pos < 0)
                return 0;
            if (flen > pos)
                flen = pos;
            BootStrUploadBs(assembled, flen, ns);
            s_curOff += span;
            s_played = frame_index;
            if (s_frameCount <= s_played)
                s_frameCount = s_played + 1;
            s_atEnd = (s_curOff >= s_nsect);
            if ((s_played % 30) == 0) {
                int nz = 0;
                for (i = 0; i < BOOT_STR_W * BOOT_STR_H; i++) {
                    if (s_pixels[i] != 0)
                        nz++;
                }
                printf("[xeno-port][boot] intro STR feed file=%d frame=%d off=%d sectors=%d 320x224 nz=%d\n",
                       BOOT_STR_ENTRY, s_played, s_curOff - span, ns, nz);
                fflush(stdout);
            }
            return 1;
        }
        s_curOff += ns;
        s_played++;
    }
    return 1;
}

int PcPort_BootStrPlayTitleFrame(void)
{
    const unsigned char* title;
    int size = 0;
    int ns, flen, i, nz;

    if (!s_loaded && !PcPort_BootStrLoad())
        return 0;
    title = PcPort_BootTitleGraphicPayload(&size);
    ns = PcPort_BootTitleFrameSectors();
    flen = PcPort_BootTitleFrameLen();
    if (!title || size < 0x800 || ns <= 0)
        return 0;
    if (!BootStrDecodeAndUpload(title, ns, flen))
        return 0;
    nz = 0;
    for (i = 0; i < BOOT_STR_W * BOOT_STR_H; i++) {
        if (s_pixels[i] != 0)
            nz++;
    }
    printf("[xeno-port][boot] intro STR title frame sectors=%d 320x224 nz=%d\n",
           ns, nz);
    fflush(stdout);
    return 1;
}

void PcPort_BootStrDrawPrims(void* ot)
{
    static POLY_FT4 quad[2];
    u_short tp0 = GetTPage(2, 0, BOOT_STR_TPAGE_X, BOOT_STR_TPAGE_Y);
    u_short tp1 = GetTPage(2, 0, BOOT_STR_TPAGE_X + 256, BOOT_STR_TPAGE_Y);
    int i;

    if (!s_loaded)
        return;
    for (i = 0; i < 2; i++) {
        int x = i * 256;
        int w = (i == 0) ? 256 : (BOOT_STR_W - 256);
        SetPolyFT4(&quad[i]);
        setShadeTex(&quad[i], 1);
        setXYWH(&quad[i], x, 0, w, BOOT_STR_H);
        setUVWH(&quad[i], 0, 0, w - 1, BOOT_STR_H - 1);
        setRGB0(&quad[i], 128, 128, 128);
        quad[i].tpage = (i == 0) ? tp0 : tp1;
        quad[i].clut = 0;
        if (ot)
            AddPrim(ot, &quad[i]);
        else
            DrawPrim(&quad[i]);
    }
}

int PcPort_BootStrWriteBmp(const char* path)
{
    FILE* f;
    unsigned char hdr[54];
    int w = BOOT_STR_W, h = BOOT_STR_H;
    int rowb = w * 3;
    int pad = (4 - (rowb & 3)) & 3;
    int imgsz = (rowb + pad) * h;
    int filesz = 54 + imgsz;
    int y, x;

    if (!path || !s_loaded)
        return 0;
    f = fopen(path, "wb");
    if (!f)
        return 0;
    memset(hdr, 0, 54);
    hdr[0] = 'B';
    hdr[1] = 'M';
    memcpy(hdr + 2, &filesz, 4);
    hdr[10] = 54;
    hdr[14] = 40;
    memcpy(hdr + 18, &w, 4);
    memcpy(hdr + 22, &h, 4);
    hdr[26] = 1;
    hdr[28] = 24;
    memcpy(hdr + 34, &imgsz, 4);
    fwrite(hdr, 1, 54, f);
    for (y = h - 1; y >= 0; y--) {
        unsigned char row[320 * 3 + 4];
        memset(row, 0, sizeof(row));
        for (x = 0; x < w; x++) {
            uint16_t p = s_pixels[y * w + x];
            int r = (p & 31) << 3;
            int g = ((p >> 5) & 31) << 3;
            int b = ((p >> 10) & 31) << 3;
            row[x * 3 + 0] = (unsigned char)b;
            row[x * 3 + 1] = (unsigned char)g;
            row[x * 3 + 2] = (unsigned char)r;
        }
        fwrite(row, 1, rowb + pad, f);
    }
    fclose(f);
    return 1;
}

void PcPort_BootStrUnload(void)
{
    s_loaded = 0;
    s_frameCount = 0;
    s_curOff = 0;
    s_played = -1;
    s_atEnd = 0;
    s_nsect = 0;
}
