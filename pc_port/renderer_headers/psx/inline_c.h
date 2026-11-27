#ifndef XENO_RENDERER_GEOMETRY_TRANSFERS_H
#define XENO_RENDERER_GEOMETRY_TRANSFERS_H

#include <stdint.h>

/* Host exports from PsyCross src/psx/INLINE_C.C. The transfer interface
 * addresses 32-bit PS1 words even when host long or pointers are wider.
 * Register numbers and command fields follow the PSX-SPX GTE specification:
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/ */
#ifdef __cplusplus
extern "C" {
#endif
unsigned int MFC2(int index);
int MFC2_S(int index);
void MTC2(unsigned int value, int index);
void MTC2_S(int value, int index);
unsigned int CFC2(int index);
int CFC2_S(int index);
void CTC2(unsigned int value, int index);
void CTC2_S(int value, int index);
int doCOP2(int command);
#ifdef __cplusplus
}
#endif

/* Packet fields can be unaligned and can belong to unrelated struct types. */
static inline uint32_t xeno_gte_load_word(const void *address)
{
    uint32_t word;
    __builtin_memcpy(&word, address, sizeof(word));
    return word;
}

static inline void xeno_gte_store_word(void *address, uint32_t word)
{
    __builtin_memcpy(address, &word, sizeof(word));
}

static inline void xeno_gte_load_data(const void *address, int first, int count)
{
    const unsigned char *bytes = (const unsigned char *)address;
    for (int i = 0; i < count; ++i)
        MTC2(xeno_gte_load_word(bytes + 4 * i), first + i);
}

static inline void xeno_gte_store_data(void *address, int first, int count)
{
    unsigned char *bytes = (unsigned char *)address;
    for (int i = 0; i < count; ++i)
        xeno_gte_store_word(bytes + 4 * i, MFC2(first + i));
}

static inline void gte_ldv0(const void *vertex) { xeno_gte_load_data(vertex, 0, 2); }
static inline void gte_ldv1(const void *vertex) { xeno_gte_load_data(vertex, 2, 2); }

static inline void gte_ldv3(const void *first, const void *second, const void *third)
{
    gte_ldv0(first);
    gte_ldv1(second);
    xeno_gte_load_data(third, 4, 2);
}

static inline void gte_ldlvl(const void *vector) { xeno_gte_load_data(vector, 9, 3); }
static inline void gte_ldrgb(const void *color) { MTC2(xeno_gte_load_word(color), 6); }
static inline void gte_lddp(const void *cue) { MTC2(xeno_gte_load_word(cue), 8); }

static inline void gte_stlvnl(void *vector) { xeno_gte_store_data(vector, 25, 3); }
static inline void gte_stlvl(void *vector) { xeno_gte_store_data(vector, 9, 3); }

static inline void gte_stsv(void *vector)
{
    unsigned char *bytes = (unsigned char *)vector;
    for (int i = 0; i < 3; ++i) {
        uint16_t component = (uint16_t)MFC2(9 + i);
        __builtin_memcpy(bytes + 2 * i, &component, sizeof(component));
    }
}

static inline void gte_stotz(void *z) { xeno_gte_store_word(z, MFC2(7)); }
static inline void gte_stdp(void *cue) { xeno_gte_store_word(cue, MFC2(8)); }
static inline void gte_stsxy0(void *xy) { xeno_gte_store_word(xy, MFC2(12)); }
static inline void gte_stsxy1(void *xy) { xeno_gte_store_word(xy, MFC2(13)); }
static inline void gte_stsxy2(void *xy) { xeno_gte_store_word(xy, MFC2(14)); }
static inline void gte_stsxy(void *xy) { gte_stsxy2(xy); }
static inline void gte_stsz(void *z) { xeno_gte_store_word(z, MFC2(19)); }
static inline void gte_stszotz(void *z) { xeno_gte_store_word(z, MFC2(19) >> 2); }
static inline void gte_strgb(void *color) { xeno_gte_store_word(color, MFC2(22)); }
static inline void gte_stopz(void *area) { xeno_gte_store_word(area, MFC2(24)); }
static inline void gte_stflg(void *flags) { xeno_gte_store_word(flags, CFC2(31)); }

static inline void gte_stsxy3(void *first, void *second, void *third)
{
    gte_stsxy0(first);
    gte_stsxy1(second);
    gte_stsxy2(third);
}

static inline void gte_stsz3(void *first, void *second, void *third)
{
    xeno_gte_store_word(first, MFC2(17));
    xeno_gte_store_word(second, MFC2(18));
    gte_stsz(third);
}

static inline void xeno_gte_load_controls(const void *source, int first, int count)
{
    const unsigned char *bytes = (const unsigned char *)source;
    for (int i = 0; i < count; ++i)
        CTC2(xeno_gte_load_word(bytes + 4 * i), first + i);
}

static inline void xeno_gte_store_controls(void *destination, int first, int count)
{
    unsigned char *bytes = (unsigned char *)destination;
    for (int i = 0; i < count; ++i)
        xeno_gte_store_word(bytes + 4 * i, CFC2(first + i));
}

static inline void gte_SetRotMatrix(const void *matrix) { xeno_gte_load_controls(matrix, 0, 5); }
static inline void gte_SetLightMatrix(const void *matrix) { xeno_gte_load_controls(matrix, 8, 5); }
static inline void gte_SetColorMatrix(const void *matrix) { xeno_gte_load_controls(matrix, 16, 5); }
static inline void gte_ReadRotMatrix(void *matrix) { xeno_gte_store_controls(matrix, 0, 8); }
static inline void gte_sttr(void *translation) { xeno_gte_store_controls(translation, 5, 3); }

static inline void gte_SetTransMatrix(const void *matrix)
{
    xeno_gte_load_controls((const unsigned char *)matrix + 20, 5, 3);
}

static inline void xeno_gte_color_controls(unsigned int r, unsigned int g, unsigned int b, int first)
{
    CTC2(r << 4, first);
    CTC2(g << 4, first + 1);
    CTC2(b << 4, first + 2);
}

static inline void gte_SetBackColor(unsigned int r, unsigned int g, unsigned int b)
{
    xeno_gte_color_controls(r, g, b, 13);
}

static inline void gte_SetFarColor(unsigned int r, unsigned int g, unsigned int b)
{
    xeno_gte_color_controls(r, g, b, 21);
}

static inline void gte_mvmva(unsigned int shift, unsigned int matrix, unsigned int vector,
                             unsigned int translation, unsigned int limit)
{
    doCOP2(0x400012 | (shift << 19) | (matrix << 17) | (vector << 15)
                    | (translation << 13) | (limit << 10));
}

static inline void gte_rt(void) { gte_mvmva(1, 0, 0, 0, 0); }
static inline void gte_rtv0tr(void) { gte_rt(); }
static inline void gte_rtv0(void) { gte_mvmva(1, 0, 0, 3, 0); }
static inline void gte_rtir(void) { gte_mvmva(1, 0, 3, 3, 0); }
static inline void gte_rtir_sf0(void) { gte_mvmva(0, 0, 3, 3, 0); }

static inline void gte_rtps(void) { doCOP2(0x180001); }
static inline void gte_rtpt(void) { doCOP2(0x280030); }
static inline void gte_nclip(void) { doCOP2(0x1400006); }
static inline void gte_avsz3(void) { doCOP2(0x158002d); }
static inline void gte_avsz4(void) { doCOP2(0x168002e); }

static inline void gte_dpcs(void) { doCOP2(0x780010); }
static inline void gte_ncs(void) { doCOP2(0xc8041e); }
static inline void gte_nct(void) { doCOP2(0xd80420); }
static inline void gte_nccs(void) { doCOP2(0x108041b); }
static inline void gte_ncct(void) { doCOP2(0x118043f); }

static inline void gte_gpf0(void) { doCOP2(0x190003d); }
static inline void gte_gpf12(void) { doCOP2(0x198003d); }
static inline void gte_gpl12(void) { doCOP2(0x1a8003e); }
static inline void gte_op0(void) { doCOP2(0x170000c); }
static inline void gte_op12(void) { doCOP2(0x178000c); }
static inline void gte_sqr0(void) { doCOP2(0xa00428); }

#endif
