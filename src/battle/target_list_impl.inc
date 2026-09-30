/* Custom entry/count bindings are unsigned-byte lvalues. Group reads return
 * unsigned bytes and rank reads unsigned halfwords. Evaluate indices once:
 * list writes use output++. LOCALS supplies candidates/matching storage,
 * each at least 12 bytes; host arrays alone do not model retail guest frames.
 * Custom bindings must preserve accesses/aliasing or decline before dispatch.
 * This seam does not authorize native adoption of the nonmatching list. */
#ifndef XBT_LIST_CUSTOM_BINDINGS
#define XBT_LIST_COUNT D_800D3274
#define XBT_LIST_ENTRY(i) D_800C3E90[i]
#define XBT_LIST_GROUP(i) D_800C3EB4[i][0]
#define XBT_LIST_RANK(i) D_800CCD34[i][0]
#define XBT_LIST_ELIGIBLE(a, t) func_80083FF4(a, t)
#define XBT_LIST_LOCALS u8 candidates[12]; u8 matching[12];
#endif
#ifndef XBT_LIST_CANDIDATE
#define XBT_LIST_CANDIDATE(i) candidates[i]
#endif
#ifndef XBT_LIST_MATCHING
#define XBT_LIST_MATCHING(i) matching[i]
#endif
#ifndef XBT_LIST_SIGNATURE
#define XBT_LIST_SIGNATURE u32 func_800841E0(u32 arg0)
#endif

XBT_LIST_SIGNATURE {
    XBT_LIST_LOCALS
    s32 i, total, output;
    XBT_LIST_COUNT = 0;
    for (i = 0; i < 12; i++) {
        XBT_LIST_CANDIDATE(i) = 0xFF;
        XBT_LIST_ENTRY(i) = 0xFF;
        XBT_LIST_MATCHING(i) = 0;
    }
    i = 3;
    if ((arg0 & 0xFF) < 3) {
        total = 0;
        for (; i < 11;) {
            u8 target = i;
            u32 eligible = XBT_LIST_ELIGIBLE(arg0 & 0xFF, target & 0xFF);
            i++;
            if (eligible & 0xFF) {
                XBT_LIST_CANDIDATE(total) = target;
                total++;
                XBT_LIST_COUNT++;
            }
        }
    } else {
        i = 0;
        total = 0;
        for (; i < 3;) {
            u8 target = i;
            u32 eligible = XBT_LIST_ELIGIBLE(arg0 & 0xFF, target & 0xFF);
            i++;
            if (eligible & 0xFF) {
                XBT_LIST_CANDIDATE(total) = target;
                total++;
                XBT_LIST_COUNT++;
            }
        }
    }
    output = 0;
    for (i = 0; i < total; i++) {
        if (XBT_LIST_GROUP(XBT_LIST_CANDIDATE(i)) == XBT_LIST_GROUP(arg0 & 0xFF)) {
            XBT_LIST_ENTRY(output) = XBT_LIST_CANDIDATE(i);
            XBT_LIST_CANDIDATE(i) = 0xFF;
            XBT_LIST_MATCHING(output++) = 1;
        }
    }
    for (i = 0; i < total; i++) {
        if (XBT_LIST_CANDIDATE(i) != 0xFF) XBT_LIST_ENTRY(output++) = XBT_LIST_CANDIDATE(i);
    }
    if (XBT_LIST_MATCHING(0)) {
        for (i = 1; i < total; i++) {
            if (XBT_LIST_MATCHING(i) && XBT_LIST_RANK(XBT_LIST_ENTRY(0)) > XBT_LIST_RANK(XBT_LIST_ENTRY(i))) {
                u32 temp = XBT_LIST_ENTRY(0);
                XBT_LIST_ENTRY(0) = XBT_LIST_ENTRY(i);
                XBT_LIST_ENTRY(i) = temp;
            }
        }
    } else {
        for (i = 1; i < total; i++) {
            if (XBT_LIST_RANK(XBT_LIST_ENTRY(0)) > XBT_LIST_RANK(XBT_LIST_ENTRY(i))) {
                u32 temp = XBT_LIST_ENTRY(0);
                XBT_LIST_ENTRY(0) = XBT_LIST_ENTRY(i);
                XBT_LIST_ENTRY(i) = temp;
            }
        }
    }
    return XBT_LIST_ENTRY(0);
}

#undef XBT_LIST_COUNT
#undef XBT_LIST_ENTRY
#undef XBT_LIST_GROUP
#undef XBT_LIST_RANK
#undef XBT_LIST_ELIGIBLE
#undef XBT_LIST_LOCALS
#undef XBT_LIST_SIGNATURE
#undef XBT_LIST_CUSTOM_BINDINGS
#undef XBT_LIST_CANDIDATE
#undef XBT_LIST_MATCHING
