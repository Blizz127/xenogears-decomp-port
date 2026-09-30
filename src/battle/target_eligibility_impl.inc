#ifndef XBT_ELIGIBILITY_CUSTOM_BINDINGS
/* Legacy expressions intentionally retain the exact retail compiler shape.
 * Custom bindings supply unsigned byte/halfword reads at these same lazy
 * access points. Matrix owns both group reads and its packed-pointer lookup;
 * it must not be evaluated on absent, blocked, or alternate-status paths.
 * Arguments are side-effect-free masked IDs. Binding failures must not
 * return a fabricated eligibility value. See the live-RAM adoption contract. */
#define XBT_PRESENT(target) D_800D2DCC[target]
#define XBT_BLOCKED(target) D_800C3EB7[target][0]
#define XBT_ALTERNATE(target) D_800D32A1[target][0]
#define XBT_MATRIX(actor, target) \
    (D_800D3364 + (D_800C3EB4[actor][0] * 64 + D_800C3EB4[target][0] * 8))[0x140]
#define XBT_NORMAL_STATUS(target) D_800CCD64[target][0]
#define XBT_ALTERNATE_STATUS(target) D_800CCE08[target][0]
#endif

#ifndef XBT_ELIGIBILITY_SIGNATURE
#define XBT_ELIGIBILITY_SIGNATURE u32 func_80083FF4(u32 arg0, u32 arg1)
#endif
XBT_ELIGIBILITY_SIGNATURE {
    u8 result = 0;
    u32 target = arg1 & 0xFF;
    if (XBT_PRESENT(target) != 0 && XBT_BLOCKED(target) == 0) {
        if (XBT_ALTERNATE(target) == 0) {
            if (XBT_MATRIX(arg0 & 0xFF, target) == 0) {
                result = (XBT_NORMAL_STATUS(target) & 0xC001) == 0;
            }
        } else {
            if ((XBT_ALTERNATE_STATUS(target) & 0xC001) == 0) result = 1;
        }
    }
    return result;
}

#undef XBT_PRESENT
#undef XBT_BLOCKED
#undef XBT_ALTERNATE
#undef XBT_MATRIX
#undef XBT_NORMAL_STATUS
#undef XBT_ALTERNATE_STATUS
#undef XBT_ELIGIBILITY_CUSTOM_BINDINGS
#undef XBT_ELIGIBILITY_SIGNATURE
