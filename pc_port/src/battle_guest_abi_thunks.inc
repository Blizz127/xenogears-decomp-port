/* Typed bridge thunks for the battle guest-ABI leaves (M-a; design doc
 * battle-abi-design.md section 2, "Typed bridge thunks").
 *
 * Included by pc_port/src/battle_mips_runtime.c.  Each entry declares, for one
 * adopted leaf whose matched C uses include/battle_guest_abi.h, exactly what
 * the interpreter->native bridge hands it and takes back:
 *
 *   BG_THUNK_ARG_W32    32-bit integer: the raw guest register word
 *   BG_THUNK_ARG_GUEST  32-bit guest address held in an integer: the raw word,
 *                       never translated (the body resolves it with GPTR)
 *   BG_THUNK_ARG_U8/S8/U16/S16  narrow integer: the word truncated (and
 *                       sign-extended for S*) to the declared width
 *   BG_THUNK_ARG_HOST   declared pointer (incl. GSLOT(T) = u32*): the host
 *                       pointer the runtime's resolver gives the guest word
 *
 *   BG_THUNK_RET_VOID   v0 is left as it was
 *   BG_THUNK_RET_W32 / RET_GUEST   v0 = the low 32 bits of the return
 *   BG_THUNK_RET_U8/S8/U16/S16     v0 = the return widened explicitly
 *   BG_THUNK_RET_HOST   v0 = the guest form (bg_addr) of a returned pointer
 *
 * No entry is ever subject to the legacy translate_argument() heuristic.
 * Every adopted leaf whose port-visible body uses a guest-ABI macro must have
 * an entry here, and every entry must agree with the C prototype (arity,
 * integer/pointer kind, narrow widths, return kind); both rules are checked by
 * `tools/scripts/battle_overlay_host_tus.py --check-abi-thunks`, which the
 * port build and the overlay differential run.  Leaves without an entry keep
 * the legacy generic bridge unchanged.
 *
 * BG_THUNK(guest address, name, return kind, argc, argument kinds...) */
BG_THUNK(0x800a5a48u, "func_800A5A48", BG_THUNK_RET_W32, 4,
         BG_THUNK_ARG_GUEST, BG_THUNK_ARG_GUEST, BG_THUNK_ARG_GUEST, BG_THUNK_ARG_GUEST)
BG_THUNK(0x800a579cu, "func_800A579C", BG_THUNK_RET_W32, 1,
         BG_THUNK_ARG_GUEST)
BG_THUNK(0x800a5d54u, "func_800A5D54", BG_THUNK_RET_W32, 3,
         BG_THUNK_ARG_GUEST, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800a5914u, "func_800A5914", BG_THUNK_RET_W32, 4,
         BG_THUNK_ARG_GUEST, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800a5be8u, "func_800A5BE8", BG_THUNK_RET_VOID, 5,
         BG_THUNK_ARG_GUEST, BG_THUNK_ARG_GUEST, BG_THUNK_ARG_GUEST, BG_THUNK_ARG_GUEST,
         BG_THUNK_ARG_HOST)
BG_THUNK(0x800a5870u, "func_800A5870", BG_THUNK_RET_W32, 3,
         BG_THUNK_ARG_GUEST, BG_THUNK_ARG_W32, BG_THUNK_ARG_GUEST)
BG_THUNK(0x8007ae38u, "func_8007AE38", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007e8e0u, "func_8007E8E0", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007e154u, "func_8007E154", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_W32)
BG_THUNK(0x8007fec4u, "func_8007FEC4", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80070e2cu, "func_80070E2C", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80070eb0u, "func_80070EB0", BG_THUNK_RET_VOID, 1, BG_THUNK_ARG_W32)
BG_THUNK(0x8009892cu, "func_8009892C", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x800723e0u, "func_800723E0", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80071b94u, "func_80071B94", BG_THUNK_RET_VOID, 1, BG_THUNK_ARG_W32)
BG_THUNK(0x800742a0u, "func_800742A0", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80085350u, "func_80085350", BG_THUNK_RET_VOID, 0)
/* Codemod sweep batch 1: ppBoard family (main19, main30). */
BG_THUNK(0x8007ae98u, "func_8007AE98", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007affcu, "func_8007AFFC", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b040u, "func_8007B040", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b084u, "func_8007B084", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b0c8u, "func_8007B0C8", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b134u, "func_8007B134", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007e954u, "func_8007E954", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007e98cu, "func_8007E98C", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007e9d0u, "func_8007E9D0", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007eac8u, "func_8007EAC8", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
/* Wrap-faithful multiply and bare-DIVU siblings (main19). */
BG_THUNK(0x8007af5cu, "func_8007AF5C", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007afacu, "func_8007AFAC", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b198u, "func_8007B198", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b208u, "func_8007B208", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007b264u, "func_8007B264", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
/* Codemod sweep batch 2: main30 compare leaves (main30). */
BG_THUNK(0x8007ea08u, "func_8007EA08", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ea4cu, "func_8007EA4C", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ea84u, "func_8007EA84", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007eb08u, "func_8007EB08", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007eb50u, "func_8007EB50", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007eb90u, "func_8007EB90", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ebd8u, "func_8007EBD8", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ec10u, "func_8007EC10", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ec54u, "func_8007EC54", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ec94u, "func_8007EC94", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
/* Codemod sweep batch 3: main30 compare leaves (main30). */
BG_THUNK(0x8007ecdcu, "func_8007ECDC", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ed14u, "func_8007ED14", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ed58u, "func_8007ED58", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ed98u, "func_8007ED98", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ede0u, "func_8007EDE0", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007ee28u, "func_8007EE28", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
/* Codemod sweep batch 3: main22 row copies (and its adopted clears, now in a
 * strict guest-ABI TU), main14 board store. */
BG_THUNK(0x8007b98cu, "func_8007B98C", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_W32)
BG_THUNK(0x8007b9c8u, "func_8007B9C8", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_W32)
BG_THUNK(0x8007ba88u, "func_8007BA88", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8007bab8u, "func_8007BAB8", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8007a828u, "func_8007A828", BG_THUNK_RET_W32, 3,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8)
BG_THUNK(0x8007a7bcu, "func_8007A7BC", BG_THUNK_RET_U8, 4,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_HOST, BG_THUNK_ARG_U8, BG_THUNK_ARG_U8)
/* Opening-battle hot set: the sprite floor query (mainl110), also reached
 * from main-executable func_80022B2C. */
BG_THUNK(0x800ba8f4u, "func_800BA8F4", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_HOST)
BG_THUNK(0x800bb13cu, "func_800BB13C", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_HOST)
BG_THUNK(0x800bac50u, "func_800BAC50", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_HOST)
BG_THUNK(0x80076ac8u, "func_80076AC8", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_HOST)
BG_THUNK(0x80076c34u, "func_80076C34", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_HOST)
BG_THUNK(0x800aaa20u, "func_800AAA20", BG_THUNK_RET_W32, 5,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32)
BG_THUNK(0x800a3e98u, "func_800A3E98", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_HOST, BG_THUNK_ARG_W32)
/* Lifted port bodies (include/battle_lift.h): every argument is the raw
 * guest register word. */
BG_THUNK(0x800a0838u, "func_800A0838", BG_THUNK_RET_W32, 4,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800aff9cu, "func_800AFF9C", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
BG_THUNK(0x8009ef3cu, "func_8009EF3C", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x8009f1c4u, "func_8009F1C4", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800aad54u, "func_800AAD54", BG_THUNK_RET_VOID, 5,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32)
BG_THUNK(0x800ae2a4u, "func_800AE2A4", BG_THUNK_RET_VOID, 3,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
/* Battle frame input dispatch closure (main43, main42, main.c): u8 mode /
 * sfx-id arguments, every one void. */
BG_THUNK(0x8008a9c0u, "func_8008A9C0", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8008a684u, "func_8008A684", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8008a3ecu, "func_8008A3EC", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8008a274u, "func_8008A274", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8008a144u, "func_8008A144", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80089cccu, "func_80089CCC", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8008aa40u, "func_8008AA40", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_U8)
BG_THUNK(0x8007171cu, "func_8007171C", BG_THUNK_RET_VOID, 0)
/* h-lift experiment: lifted port bodies take and return raw guest words. */
BG_THUNK(0x80077364u, "func_80077364", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x80076c78u, "func_80076C78", BG_THUNK_RET_VOID, 6,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x80076ce8u, "func_80076CE8", BG_THUNK_RET_VOID, 7,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800780a8u, "func_800780A8", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800b3b94u, "func_800B3B94", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-frontier step1: lifted bodies take raw guest words. */
BG_THUNK(0x800beb04u, "func_800BEB04", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x800b8354u, "func_800B8354", BG_THUNK_RET_VOID, 0)
/* h-frontier step2: lifted bodies take raw guest words (800780A8 and
 * 80077364: the h-lift rows above). */
BG_THUNK(0x8007819cu, "func_8007819C", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x8008f8f4u, "func_8008F8F4", BG_THUNK_RET_VOID, 7,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x80077454u, "func_80077454", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
BG_THUNK(0x8008f6e4u, "func_8008F6E4", BG_THUNK_RET_VOID, 5,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32)
BG_THUNK(0x8008dc34u, "func_8008DC34", BG_THUNK_RET_VOID, 5,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32)
BG_THUNK(0x8008de04u, "func_8008DE04", BG_THUNK_RET_VOID, 4,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x8008e430u, "func_8008E430", BG_THUNK_RET_VOID, 5,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32)
BG_THUNK(0x8008ea70u, "func_8008EA70", BG_THUNK_RET_VOID, 4,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x8008f0a8u, "func_8008F0A8", BG_THUNK_RET_VOID, 5,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32)
BG_THUNK(0x80076b00u, "func_80076B00", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-frontier step3: lifted bodies take raw guest words. */
BG_THUNK(0x8009f844u, "func_8009F844", BG_THUNK_RET_VOID, 7,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800a2e88u, "func_800A2E88", BG_THUNK_RET_W32, 2,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800a7948u, "func_800A7948", BG_THUNK_RET_VOID, 7,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
/* h-frontier step5: 800BB248's lifted body takes the task as a raw guest word. */
BG_THUNK(0x800bb248u, "func_800BB248", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-hotpath batch 1: lifted bodies take raw guest words. */
BG_THUNK(0x800aa514u, "func_800AA514", BG_THUNK_RET_W32, 3,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800c11ccu, "func_800C11CC", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-hotpath batch 2: lifted bodies take raw guest words. */
BG_THUNK(0x800bb844u, "func_800BB844", BG_THUNK_RET_VOID, 4,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
BG_THUNK(0x800bd810u, "func_800BD810", BG_THUNK_RET_VOID, 2,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
/* h-frontier step7: lifted bodies take raw guest words. */
BG_THUNK(0x800ba984u, "func_800BA984", BG_THUNK_RET_W32, 14,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32, BG_THUNK_ARG_W32,
         BG_THUNK_ARG_W32, BG_THUNK_ARG_W32)
/* h-frontier step8: the work list passes the entry's raw (host-shaped)
 * word; the lifted body resolves it as the interpreter's bus does. */
BG_THUNK(0x800bab0cu, "func_800BAB0C", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
BG_THUNK(0x800babdcu, "func_800BABDC", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-hotpath batch 3: the lifted body takes a raw guest word. */
BG_THUNK(0x800bc460u, "func_800BC460", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-hotpath batch 4: no arguments. */
BG_THUNK(0x80075168u, "func_80075168", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80075938u, "func_80075938", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x800745ecu, "func_800745EC", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x800743a4u, "func_800743A4", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x800744bcu, "func_800744BC", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x80074554u, "func_80074554", BG_THUNK_RET_VOID, 0)
BG_THUNK(0x800bb760u, "func_800BB760", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
/* h-frontier step11: lifted bodies take raw guest words. */
BG_THUNK(0x800bb6e0u, "func_800BB6E0", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
BG_THUNK(0x800bb690u, "func_800BB690", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
BG_THUNK(0x800a9540u, "func_800A9540", BG_THUNK_RET_VOID, 1,
         BG_THUNK_ARG_W32)
