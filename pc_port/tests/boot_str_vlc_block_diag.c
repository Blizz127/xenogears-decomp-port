/* Per-block VLC path dump for Opening STR frames.
 * Same Huffman tables / peek order as pc_port/src/boot_str.c.
 *
 *   vlc_block_diag [disc] [frame|all] [interleave|contig]
 *
 * Frame assembly modes:
 *   contig     - the assembly boot_str.c used before 2026-09-01: the frame's
 *                ns chunks are assumed to be ns consecutive sectors.
 *   interleave - walk sectors, skip anything that is not a 0x0160/0x8001
 *                video chunk of this frame (the XA audio sector that the
 *                Opening interleaves every 8th sector), require chunk index
 *                == chunks seen (silent-hill-decomp str_demux.c:67-124).
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

static void build_vlc(void)
{
    uint32_t tmp[8192 + 512];
    int i, o = 0, r;
    for (i = 0; i < VLC_TABLE_LEN; i++) {
        uint32_t v = s_vlcCompressed[i];
        uint32_t val = v & 0x001fffff;
        int reps = (int)(v >> 21) + 1;
        for (r = 0; r < reps && o < (int)(sizeof(tmp) / sizeof(tmp[0])); r++)
            tmp[o++] = val;
    }
    memcpy(s_ac, tmp, sizeof(s_ac));
    memcpy(s_ac00, tmp + 8192, sizeof(s_ac00));
}

typedef struct {
    const uint16_t* words;
    int nwords;
    int bitpos;
} StrBits;

static uint32_t peek(StrBits* b, int length)
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

static void adv(StrBits* b, int n) { b->bitpos += n; }


enum { WANT_EOB = 20 * 14 * 6 };

typedef struct {
    int ac;
    int bits;
    int n_ac13;
    int n_esc;
    int n_ac00;
    int dc;
} Blk;

static int disc_read(const char* path, int lba, int count, unsigned char* dst)
{
    FILE* f;
    struct stat st;
    int sec_size = 2048, data_off = 0, i;
    if (stat(path, &st) != 0)
        return -1;
    if (st.st_size % 2352 == 0 && st.st_size / 2352 > 1000) {
        sec_size = 2352;
        data_off = 24;
    }
    f = fopen(path, "rb");
    if (!f)
        return -1;
    for (i = 0; i < count; i++) {
        if (fseek(f, (long)(lba + i) * sec_size + data_off, SEEK_SET) != 0 ||
            fread(dst + i * 0x800, 1, 0x800, f) != 0x800) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

static const char* blkname(int i)
{
    static const char* n[6] = { "Cr", "Cb", "Y1", "Y2", "Y3", "Y4" };
    return n[i % 6];
}

static int s10(unsigned v)
{
    v &= 0x3FF;
    return (v & 0x200) ? (int)v - 0x400 : (int)v;
}

static int hdr_video(const unsigned char* sec)
{
    return (sec[0] | (sec[1] << 8)) == 0x0160 && (sec[2] | (sec[3] << 8)) == 0x8001;
}

/* Assemble one frame starting at sector `off` (its chunk 0 header).
 * Returns bytes assembled (>= 0) or -1. *span = sectors consumed,
 * *skipped = non-video sectors encountered inside the span. */
static int assemble(const char* disc, int lba, int off, int nsect, int contig,
                    unsigned char* assembled, int* ns_out, int* flen_out,
                    int* span, int* skipped, int* bad)
{
    unsigned char sec[0x800];
    int ns, flen, seen = 0, pos = 0, walked = 0;
    unsigned fnum;

    *skipped = 0;
    *bad = 0;
    if (disc_read(disc, lba + off, 1, sec) != 0)
        return -1;
    if (!hdr_video(sec) || (sec[4] | (sec[5] << 8)) != 0)
        return -1;
    ns = sec[6] | (sec[7] << 8);
    if (ns <= 0)
        ns = 1;
    flen = sec[12] | (sec[13] << 8) | (sec[14] << 16) | (sec[15] << 24);
    fnum = (unsigned)(sec[8] | (sec[9] << 8) | (sec[10] << 16) | ((unsigned)sec[11] << 24));
    *ns_out = ns;
    *flen_out = flen;
    if (ns > 16)
        return -1;
    if (contig) {
        int i;
        for (i = 0; i < ns; i++) {
            if (off + i >= nsect || disc_read(disc, lba + off + i, 1, sec) != 0)
                return -1;
            if (!hdr_video(sec))
                (*skipped)++; /* copied anyway, as boot_str.c did */
            else if ((sec[4] | (sec[5] << 8)) != i)
                (*bad)++;
            memcpy(assembled + pos, sec + 32, 0x800 - 32);
            pos += 0x800 - 32;
        }
        *span = ns;
    } else {
        while (seen < ns && walked < ns + 8) {
            if (off + walked >= nsect ||
                disc_read(disc, lba + off + walked, 1, sec) != 0)
                return -1;
            walked++;
            if (!hdr_video(sec)) {
                (*skipped)++;
                continue;
            }
            if ((sec[4] | (sec[5] << 8)) != seen ||
                (unsigned)(sec[8] | (sec[9] << 8) | (sec[10] << 16) | ((unsigned)sec[11] << 24)) != fnum) {
                (*bad)++;
                return -1;
            }
            memcpy(assembled + pos, sec + 32, 0x800 - 32);
            pos += 0x800 - 32;
            seen++;
        }
        if (seen < ns)
            return -1;
        *span = walked;
    }
    return pos;
}

typedef struct {
    int nblk;
    int bitpos;
    int nbits;
    int miss; /* 0 none, 1 ac13, 2 ac00 */
    int miss_idx;
} WalkResult;

/* Same walk as boot_str.c BootStrVlc(). */
static WalkResult walk(const uint16_t* words, int nwords, Blk* st, FILE* per)
{
    StrBits b;
    WalkResult r;
    int coeff = 0, blk_start = 0, dc = 0;
    int ac = 0, n_ac13 = 0, n_esc = 0, n_ac00 = 0;

    memset(&r, 0, sizeof(r));
    b.words = words;
    b.nwords = nwords;
    b.bitpos = 0;
    r.nbits = nwords * 16;
    while (r.nblk < WANT_EOB) {
        if (!coeff) {
            dc = s10(peek(&b, 10));
            adv(&b, 10);
            coeff = 1;
            blk_start = b.bitpos - 10;
            ac = n_ac13 = n_esc = n_ac00 = 0;
            continue;
        }
        if (peek(&b, 2) == 2) {
            Blk* s = &st[r.nblk];
            adv(&b, 2);
            s->ac = ac;
            s->bits = b.bitpos - blk_start;
            s->n_ac13 = n_ac13;
            s->n_esc = n_esc;
            s->n_ac00 = n_ac00;
            s->dc = dc;
            if (per) {
                int mb = r.nblk / 6;
                fprintf(per, "%d %d %d %d %s %d %d %d %d %d %d\n", r.nblk, mb,
                        mb / 14, mb % 14, blkname(r.nblk), s->dc, s->ac, s->bits,
                        s->n_ac13, s->n_esc, s->n_ac00);
            }
            r.nblk++;
            coeff = 0;
            continue;
        }
        if (peek(&b, 6) == 1) {
            adv(&b, 22);
            ac++;
            n_esc++;
            continue;
        }
        if (peek(&b, 8) != 0) {
            uint32_t idx = peek(&b, 13) & 8191;
            uint32_t value = s_ac[idx];
            int len = (int)(value >> 16);
            if (len <= 0 || len > 13) {
                r.miss = 1;
                r.miss_idx = (int)idx;
                break;
            }
            adv(&b, len);
            ac++;
            n_ac13++;
        } else {
            uint32_t p17 = peek(&b, 17);
            uint32_t value;
            int len;
            if (p17 >= 512) {
                r.miss = 2;
                r.miss_idx = (int)p17;
                break;
            }
            value = s_ac00[p17];
            len = (int)(value >> 16);
            if (len <= 0 || len > 17) {
                r.miss = 2;
                r.miss_idx = (int)p17;
                break;
            }
            adv(&b, len);
            ac++;
            n_ac00++;
        }
    }
    r.bitpos = b.bitpos;
    return r;
}

static int find_frame(const char* disc, int lba, int nsect, int want, int* off_out)
{
    unsigned char hdr[0x800];
    int off = 0, frame = 0;
    while (off < nsect) {
        if (disc_read(disc, lba + off, 1, hdr) != 0)
            return 0;
        if (hdr_video(hdr) && (hdr[4] | (hdr[5] << 8)) == 0) {
            if (frame == want) {
                *off_out = off;
                return 1;
            }
            frame++;
        }
        off++;
    }
    return 0;
}

int main(int argc, char** argv)
{
    const char* disc = argc > 1 ? argv[1] : "disc/disc1.bin";
    const char* frame_arg = argc > 2 ? argv[2] : "0";
    int contig = argc > 3 && strcmp(argv[3], "contig") == 0;
    const char* mode = contig ? "contig" : "interleave";
    unsigned char table[0x10 * 0x800];
    unsigned char* e;
    int lba, next_lba, nsect;
    static unsigned char assembled[0x800 * 16];
    static Blk st[WANT_EOB];
    int want_frame, off, ns, flen, span, skipped, bad, pos, i;
    int qscale, ver, nwords;
    WalkResult r;
    FILE* per;
    char path[256];

    build_vlc();
    if (disc_read(disc, 0x18, 0x10, table) != 0) {
        fprintf(stderr, "cannot read archive table from %s\n", disc);
        return 1;
    }
    e = table + (15 - 1) * 7;
    lba = e[0] | (e[1] << 8) | (e[2] << 16);
    e = table + 15 * 7;
    next_lba = e[0] | (e[1] << 8) | (e[2] << 16);
    nsect = next_lba - lba;

    if (strcmp(frame_arg, "all") == 0) {
        int frame = 0, ok = 0, total = 0, max_slack = 0, min_slack = 1 << 30;
        int first_bad = -1;
        printf("opening lba=%d nsect=%d mode=%s sweep\n", lba, nsect, mode);
        printf("frame off ns span flen skipped eob bitpos nbits slack miss\n");
        off = 0;
        for (;;) {
            unsigned char hdr[0x800];
            int slack;
            /* scan to the next chunk-0 header */
            while (off < nsect) {
                if (disc_read(disc, lba + off, 1, hdr) != 0)
                    return 1;
                if (hdr_video(hdr) && (hdr[4] | (hdr[5] << 8)) == 0)
                    break;
                off++;
            }
            if (off >= nsect)
                break;
            pos = assemble(disc, lba, off, nsect, contig, assembled, &ns, &flen,
                           &span, &skipped, &bad);
            if (pos < 0) {
                printf("%d %d %d - %d - assemble-failed (skipped=%d bad=%d)\n",
                       frame, off, ns, flen, skipped, bad);
                if (first_bad < 0)
                    first_bad = frame;
                off += ns > 0 ? ns : 1;
                frame++;
                total++;
                continue;
            }
            if (flen > pos)
                flen = pos;
            nwords = (flen - 8) / 2;
            r = walk((const uint16_t*)(assembled + 8), nwords, st, NULL);
            slack = r.nbits - r.bitpos;
            if (r.nblk == WANT_EOB && !r.miss) {
                ok++;
                if (slack > max_slack)
                    max_slack = slack;
                if (slack < min_slack)
                    min_slack = slack;
            } else if (first_bad < 0) {
                first_bad = frame;
            }
            if (r.nblk != WANT_EOB || r.miss || skipped || (frame % 100) == 0)
                printf("%d %d %d %d %d %d %d %d %d %d %s\n", frame, off, ns, span,
                       flen, skipped, r.nblk, r.bitpos, r.nbits, slack,
                       r.miss == 0 ? "-" : (r.miss == 1 ? "ac13" : "ac00"));
            total++;
            off += span;
            frame++;
        }
        printf("sweep mode=%s frames=%d full_1680_no_miss=%d first_bad=%d "
               "end_slack_bits[min=%d max=%d]\n",
               mode, total, ok, first_bad, min_slack, max_slack);
        return ok == total ? 0 : 2;
    }

    want_frame = atoi(frame_arg);
    printf("opening lba=%d nsect=%d want_frame=%d mode=%s\n", lba, nsect,
           want_frame, mode);
    if (!find_frame(disc, lba, nsect, want_frame, &off)) {
        fprintf(stderr, "frame %d not found\n", want_frame);
        return 1;
    }
    pos = assemble(disc, lba, off, nsect, contig, assembled, &ns, &flen, &span,
                   &skipped, &bad);
    if (pos < 0) {
        fprintf(stderr, "frame %d assemble failed (skipped=%d bad=%d)\n",
                want_frame, skipped, bad);
        return 1;
    }
    if (flen > pos)
        flen = pos;
    qscale = assembled[4] | (assembled[5] << 8);
    ver = assembled[6] | (assembled[7] << 8);
    nwords = (flen - 8) / 2;
    printf("frame=%d off=%d ns=%d span=%d nonvideo_in_span=%d flen=%d qscale=%d "
           "ver=%d nwords=%d\n",
           want_frame, off, ns, span, skipped, flen, qscale, ver, nwords);

    snprintf(path, sizeof(path),
             "scratchpad/boot_fix_20260901/vlc_block_per_f%d_%s.log", want_frame,
             mode);
    per = fopen(path, "w");
    if (per)
        fprintf(per, "blk mb mx my type dc ac bits n_ac13 n_esc n_ac00\n");
    r = walk((const uint16_t*)(assembled + 8), nwords, st, per);
    if (per)
        fclose(per);
    if (r.miss)
        printf("MISS table=%s idx=0x%x at block %d bitpos=%d\n",
               r.miss == 1 ? "ac13" : "ac00", (unsigned)r.miss_idx, r.nblk,
               r.bitpos);
    printf("eob_blocks=%d/%d bitpos=%d/%d (slack %d bits)\n", r.nblk, WANT_EOB,
           r.bitpos, r.nbits, r.nbits - r.bitpos);

    {
        int ac_hist[64];
        long tot_ac13 = 0, tot_esc = 0, tot_ac00 = 0, tot_ac = 0, tot_bits = 0;
        int bits_min = 1 << 30, bits_max = 0;
        int n0 = 0, n1_16 = 0, n17 = 0;
        /* per type: 0 Cr, 1 Cb, 2 Y */
        long t_n[3] = { 0, 0, 0 }, t_ac[3] = { 0, 0, 0 }, t_bits[3] = { 0, 0, 0 };
        long t_dc0[3] = { 0, 0, 0 }, t_withac[3] = { 0, 0, 0 };
        int t_dcmin[3] = { 9999, 9999, 9999 }, t_dcmax[3] = { -9999, -9999, -9999 };
        long t_dcsum[3] = { 0, 0, 0 };
        int nblk = r.nblk;
        int t;

        memset(ac_hist, 0, sizeof(ac_hist));
        for (i = 0; i < nblk; i++) {
            int a = st[i].ac;
            t = (i % 6) < 2 ? (i % 6) : 2;
            tot_ac += a;
            tot_ac13 += st[i].n_ac13;
            tot_esc += st[i].n_esc;
            tot_ac00 += st[i].n_ac00;
            tot_bits += st[i].bits;
            ac_hist[a < 64 ? a : 63]++;
            if (a == 0)
                n0++;
            else if (a <= 16)
                n1_16++;
            else
                n17++;
            if (st[i].bits < bits_min)
                bits_min = st[i].bits;
            if (st[i].bits > bits_max)
                bits_max = st[i].bits;
            t_n[t]++;
            t_ac[t] += a;
            t_bits[t] += st[i].bits;
            if (a == 0)
                t_dc0[t]++;
            else
                t_withac[t]++;
            if (st[i].dc < t_dcmin[t])
                t_dcmin[t] = st[i].dc;
            if (st[i].dc > t_dcmax[t])
                t_dcmax[t] = st[i].dc;
            t_dcsum[t] += st[i].dc;
        }
        if (nblk == 0)
            return 2;
        printf("\n=== frame %d  mode=%s  %d blocks  codes ===\n", want_frame, mode, nblk);
        printf("path  ac13(13-bit table)  %ld\n", tot_ac13);
        printf("path  escape(22-bit)      %ld\n", tot_esc);
        printf("path  ac00(17-bit table)  %ld\n", tot_ac00);
        printf("path  EOB(2-bit)          %d\n", nblk);
        printf("total AC coefficients     %ld  (mean %.2f / block)\n", tot_ac,
               tot_ac / (double)nblk);
        printf("bits per block            min %d max %d mean %.1f (10-bit DC included)\n",
               bits_min, bits_max, tot_bits / (double)nblk);
        printf("\n=== blocks by AC count ===\n");
        printf("0 AC: %d   1-16 AC: %d   17+ AC: %d\n", n0, n1_16, n17);
        printf("ac_hist");
        for (i = 0; i < 64; i++)
            if (ac_hist[i])
                printf(" %d:%d", i, ac_hist[i]);
        printf("\n\n=== by block type ===\n");
        printf("type blocks dc_only with_ac mean_ac mean_bits dc_min dc_max dc_mean\n");
        for (t = 0; t < 3; t++) {
            static const char* tn[3] = { "Cr", "Cb", "Y" };
            if (!t_n[t])
                continue;
            printf("%-4s %6ld %7ld %7ld %7.2f %9.1f %6d %6d %7.1f\n", tn[t], t_n[t],
                   t_dc0[t], t_withac[t], t_ac[t] / (double)t_n[t],
                   t_bits[t] / (double)t_n[t], t_dcmin[t], t_dcmax[t],
                   t_dcsum[t] / (double)t_n[t]);
        }
        {
            /* where the AC lives: bounding box of blocks with AC, by type */
            int mnx = 99, mxx = -1, mny = 99, mxy = -1;
            for (i = 0; i < nblk; i++) {
                int mb = i / 6, mx = mb / 14, my = mb % 14;
                if (st[i].ac == 0)
                    continue;
                if (mx < mnx) mnx = mx;
                if (mx > mxx) mxx = mx;
                if (my < mny) mny = my;
                if (my > mxy) mxy = my;
            }
            if (mxx >= 0)
                printf("AC-bearing blocks bounding box: mx %d..%d  my %d..%d\n",
                       mnx, mxx, mny, mxy);
        }
    }
    return r.nblk == WANT_EOB && !r.miss ? 0 : 2;
}
