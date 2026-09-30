/* VALUE is a byte lvalue resolved from the current context owner. Do not
 * retain that owner across BUILD. TARGET/COUNT/ENTRY return unsigned bytes.
 * These are source access points, not proof of retail load count/order:
 * custom RAM integration still needs the guest-frame and access-trace gates. */
#ifndef XBT_SELECTION_CUSTOM_BINDINGS
#define XBT_SELECTION_VALUE ((u8 *)D_800C3EAC)[0x2E8]
#define XBT_SELECTION_TARGET(a) D_800C3EAC->rows[a].target
#define XBT_SELECTION_COUNT D_800D3274
#define XBT_SELECTION_ENTRY(i) D_800C3E90[i]
#define XBT_SELECTION_BUILD(a) func_800841E0(a)
#endif
#ifndef XBT_SELECTION_SIGNATURE
#define XBT_SELECTION_SIGNATURE void func_80084A7C(u32 arg0)
#endif
#ifndef XBT_SELECTION_AFTER_BUILD
#define XBT_SELECTION_AFTER_BUILD
#endif
#ifndef XBT_SELECTION_BEFORE_FALLBACK
#define XBT_SELECTION_BEFORE_FALLBACK
#endif
#ifndef XBT_SELECTION_STORE
#define XBT_SELECTION_STORE(value) XBT_SELECTION_VALUE = value
#endif
#ifndef XBT_SELECTION_LOCALS
#define XBT_SELECTION_LOCALS s32 found = 0;
#endif
#ifndef XBT_SELECTION_FOUND
#define XBT_SELECTION_FOUND found
#endif

XBT_SELECTION_SIGNATURE {
    XBT_SELECTION_LOCALS
    s32 i;
    u32 actor = arg0 & 0xFF;
    XBT_SELECTION_STORE(XBT_SELECTION_TARGET(actor));
    XBT_SELECTION_BUILD(actor);
    XBT_SELECTION_AFTER_BUILD
    for (i = 0; i < XBT_SELECTION_COUNT; i++) {
        if (XBT_SELECTION_ENTRY(i) == XBT_SELECTION_TARGET(actor)) {
            XBT_SELECTION_FOUND++;
            break;
        }
    }
    if (XBT_SELECTION_FOUND == 0) {
        XBT_SELECTION_BEFORE_FALLBACK
        XBT_SELECTION_STORE(XBT_SELECTION_ENTRY(0));
    }
}

#undef XBT_SELECTION_VALUE
#undef XBT_SELECTION_TARGET
#undef XBT_SELECTION_COUNT
#undef XBT_SELECTION_ENTRY
#undef XBT_SELECTION_BUILD
#undef XBT_SELECTION_SIGNATURE
#undef XBT_SELECTION_CUSTOM_BINDINGS
#undef XBT_SELECTION_AFTER_BUILD
#undef XBT_SELECTION_BEFORE_FALLBACK
#undef XBT_SELECTION_STORE
#undef XBT_SELECTION_LOCALS
#undef XBT_SELECTION_FOUND
