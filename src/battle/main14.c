#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800CCCEC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD3E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDC8[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDCB[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD40[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD41[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD42[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD15[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD45[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD43[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD44[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD46[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD47[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD48[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD49[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD4C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD4D[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD4E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD4F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE24[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD9E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE28[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE29[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE2A[];
#endif

/* func_80079ED8.s: jump-table (jtbl_8006FB7C) dispatch on a1 picking one of
 * 24 D_800CCxx bases; the byte at base + 368*a0 is returned when a3 != 0,
 * else overwritten with a2.  Retail hoists nothing: the 368-byte actor
 * stride is formed inside every case, there is no default (a1 >= 24
 * dereferences an uninitialised p; callers never pass it), and the store
 * path returns an uninitialised ret (callers ignore v0 there). */
u8 func_80079ED8(u8 a0, u8 a1, u8 a2, u8 a3) {
    u8* p;
    u8 ret;
    switch (a1) {
    case 0: p = D_800CCCEC + a0 * 368; break;
    case 1: p = D_800CCD3E + a0 * 368; break;
    case 2: p = D_800CCDC8 + a0 * 368; break;
    case 3: p = D_800CCDCB + a0 * 368; break;
    case 4: p = D_800CCD40 + a0 * 368; break;
    case 5: p = D_800CCD41 + a0 * 368; break;
    case 6: p = D_800CCD42 + a0 * 368; break;
    case 7: p = D_800CCD15 + a0 * 368; break;
    case 8: p = D_800CCD45 + a0 * 368; break;
    case 9: p = D_800CCD43 + a0 * 368; break;
    case 10: p = D_800CCD44 + a0 * 368; break;
    case 11: p = D_800CCD46 + a0 * 368; break;
    case 12: p = D_800CCD47 + a0 * 368; break;
    case 13: p = D_800CCD48 + a0 * 368; break;
    case 14: p = D_800CCD49 + a0 * 368; break;
    case 15: p = D_800CCD4C + a0 * 368; break;
    case 16: p = D_800CCD4D + a0 * 368; break;
    case 17: p = D_800CCD4E + a0 * 368; break;
    case 18: p = D_800CCD4F + a0 * 368; break;
    case 19: p = D_800CCE24 + a0 * 368; break;
    case 20: p = D_800CCD9E + a0 * 368; break;
    case 21: p = D_800CCE28 + a0 * 368; break;
    case 22: p = D_800CCE29 + a0 * 368; break;
    case 23: p = D_800CCE2A + a0 * 368; break;
    }
    if (a3 == 0) {
        *p = a2;
    } else {
        ret = *p;
    }
    return ret;
}
#ifndef XENO_PC_PORT
extern u8 D_800CCD36[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD64[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD66[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD68[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD6A[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD6C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD6E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD70[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD72[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD74[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD76[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDF8[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDFC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDFE[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE08[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE0A[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE0C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE0E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE10[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE12[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD1C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD1E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD20[];
#endif

/* func_8007A280.s: halfword twin of func_80079ED8 over jtbl_8006FBDC. */
u16 func_8007A280(u8 a0, u8 a1, u16 a2, u8 a3) {
    u8* p;
    u16 ret;
    switch (a1) {
    case 0: p = D_800CCD36 + a0 * 368; break;
    case 1: p = D_800CCD34 + a0 * 368; break;
    case 2: p = D_800CCD64 + a0 * 368; break;
    case 3: p = D_800CCD66 + a0 * 368; break;
    case 4: p = D_800CCD68 + a0 * 368; break;
    case 5: p = D_800CCD6A + a0 * 368; break;
    case 6: p = D_800CCD6C + a0 * 368; break;
    case 7: p = D_800CCD6E + a0 * 368; break;
    case 8: p = D_800CCD70 + a0 * 368; break;
    case 9: p = D_800CCD72 + a0 * 368; break;
    case 10: p = D_800CCD74 + a0 * 368; break;
    case 11: p = D_800CCD76 + a0 * 368; break;
    case 12: p = D_800CCDF8 + a0 * 368; break;
    case 13: p = D_800CCDFC + a0 * 368; break;
    case 14: p = D_800CCDFE + a0 * 368; break;
    case 15: p = D_800CCE08 + a0 * 368; break;
    case 16: p = D_800CCE0A + a0 * 368; break;
    case 17: p = D_800CCE0C + a0 * 368; break;
    case 18: p = D_800CCE0E + a0 * 368; break;
    case 19: p = D_800CCE10 + a0 * 368; break;
    case 20: p = D_800CCE12 + a0 * 368; break;
    case 21: p = D_800CCD1C + a0 * 368; break;
    case 22: p = D_800CCD1E + a0 * 368; break;
    case 23: p = D_800CCD20 + a0 * 368; break;
    }
    if (a3 == 0) {
        *(u16*)p = a2;
    } else {
        ret = *(u16*)p;
    }
    return ret;
}
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3EB7[];
#endif

/* func_8007A628.s: gate on D_800D2DCC[a0], D_800C3EB7[a0*28],
 * D_800CCD64[a0*368] & 0xC002. If a1 != 0 the result is 1, else it is
 * ((D_800CCD6C[a0*368] & 0x20) == 0). */
u32 func_8007A628(u8 a0, u8 a1) {
    u8 ret = 0;
    /* The 368-byte stride is spelled out twice; retail's CSE carries it. */
    if (D_800D2DCC[a0] != 0 && D_800C3EB7[a0 * 28] == 0 &&
        (*(u16*)(D_800CCD64 + a0 * 368) & 0xC002) == 0) {
        if (a1) {
            ret = 1;
        } else {
            /* Tested through a temporary: a direct single-bit `== 0` makes
             * cc1 emit a shift/xor extract where retail has andi/sltiu. */
            u32 bit = *(u16*)(D_800CCD6C + a0 * 368) & 0x20;
            ret = bit == 0;
        }
    }
    return ret;
}

/* func_8007A6C8.s: same shape as func_8007A628 without the D_800C3EB7
 * gate. */
u32 func_8007A6C8(u8 a0, u8 a1) {
    u8 ret = 0;
    if (D_800D2DCC[a0] != 0 && (*(u16*)(D_800CCD64 + a0 * 368) & 0xC002) == 0) {
        if (a1) {
            ret = 1;
        } else {
            /* Tested through a temporary: a direct single-bit `== 0` makes
             * cc1 emit a shift/xor extract where retail has andi/sltiu. */
            u32 bit = *(u16*)(D_800CCD6C + a0 * 368) & 0x20;
            ret = bit == 0;
        }
    }
    return ret;
}

/* func_8007A744.s: gate on D_800D2DCC[a0] and D_800C3EB7[a0*28];
 * result is ((D_800CCD64[a0*368] & 0xC000) == 0). */
u32 func_8007A744(u8 a0) {
    u8 ret = 0;
    if (D_800D2DCC[a0] != 0 && D_800C3EB7[a0 * 28] == 0) {
        ret = (*(u16*)(D_800CCD64 + a0 * 368) & 0xC000) == 0;
    }
    return ret;
}

/* func_8007A7BC.s: dst = a1 + 8*a3; dst[0] = 0x80, dst[1..4] = (*a0)[0..3]
 * (retail reloads *a0 for every byte: the dst stores may alias it);
 * returns a3 + 1. a2 unused. */
u8 func_8007A7BC(u8** a0, u8* a1, u8 a2, u8 a3) {
    /* a1 + a3*8 is respelled per store (retail CSEs it and keeps a1 live). */
    a1[a3 * 8] = 0x80;
    a1[a3 * 8 + 1] = (*a0)[0];
    a1[a3 * 8 + 2] = (*a0)[1];
    a1[a3 * 8 + 3] = (*a0)[2];
    a1[a3 * 8 + 4] = (*a0)[3];
    return a3 + 1;
}


/* func_8007A828.s */
u32 func_8007A828(u8** ppBoard, u8* dst, u8 a2) {
    u8* p = *ppBoard;
    u8 v = a2;

    dst[((a2 & 0xFF) << 3) + p[1]] = p[2];
    if ((*ppBoard)[1] == 0) {
        v = a2 + 1;
    }
    return v & 0xFF;
}
