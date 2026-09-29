/* data_heap.c -- heap bookkeeping arrays whose element type is a POINTER, so
 * the retail byte size is not the host byte size.
 *
 * g_HeapUserContentNames is declared `char** g_HeapUserContentNames[HEAP_NUM_USERS]`
 * (include/system/memory.h, HEAP_NUM_USERS = 0xA) and retail reserves 0x28
 * bytes for it (0x80059FA4, asm/slus_006.64/data/49AC0.bss.s) -- 10 entries of
 * a 4-byte PSX pointer. On the host a pointer is 8 bytes, so the same 10
 * entries need 80 bytes.
 *
 * The auto-generated stub reserved 32 bytes (its minimum), i.e. only 4 host
 * entries. HeapChangeCurrentUser writes g_HeapUserContentNames[userTag] for
 * userTag up to 9 -- MovieMain uses userTag 4 (src/movie/main.c:1029) -- so
 * every call with userTag >= 4 wrote past the object into the next global
 * (ASan: global-buffer-overflow, WRITE of size 8, 0 bytes after
 * g_HeapUserContentNames, at memory.c:492).
 *
 * Defining it here with the declared element type gives it the correct host
 * size. Unlike the aliased blobs (data_game_state.c, data_controller.c) this
 * symbol is only ever accessed by name and index, never through a neighbouring
 * symbol's offset, so no .set aliasing is needed -- just the right extent.
 *
 * Extent is HEAP_USER_TEST + 1 (0xC entries), not HEAP_NUM_USERS (0xA),
 * because the game writes user tags above the retail reservation:
 * HeapResetUser() passes HEAP_USER_UNKNOWN = 0xA (main_loop.c:164, every
 * MainLoop iteration). On retail that store lands at 0x80059FA4 + 0xA*4 =
 * 0x80059FCC, i.e. on g_HeapDelayedFreeBlocksHead's first word -- harmless
 * there only because it happens immediately around HeapReset/HeapRelocate,
 * while that list is being reset. Host pointers are twice as wide, so that
 * incidental retail aliasing cannot be reproduced by layout anyway; giving the
 * array a slot for every tag the code writes keeps the store in-bounds instead
 * of corrupting whichever global the host linker placed next. Tags are only
 * ever written here and read back per-block in HeapPrintBlocks (memory.c:585),
 * so no retail-visible behaviour depends on the clobber.
 *
 * Defined without including system/memory.h on purpose: that header declares
 * the array as [HEAP_NUM_USERS] (0xA), and a 0xC-entry definition in the same
 * TU is a conflicting type. HEAP_NUM_USERS itself is NOT widened -- it belongs
 * to the matching build, where it must keep retail's 0x28 reservation.
 *
 * NOTE: the same retail-bytes-vs-host-pointer-width mismatch can affect any
 * stubbed array of PSX pointers; the generator's 32-byte floor hides the small
 * ones. Fix them here as ASan finds them. */

/* 0xC = HEAP_USER_TEST + 1 (include/system/memory.h). */
char** g_HeapUserContentNames[0xC];
