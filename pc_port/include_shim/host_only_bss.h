#ifndef HOST_ONLY_BSS_H
#define HOST_ONLY_BSS_H

/* Large zeroed arrays that hold host state only (never a GPU packet, guest
 * memory or anything a game routine links) go after all other data. The
 * non-PIE port stores host addresses in the 24-bit links of GPU packets
 * (USE_EXTENDED_PRIM_POINTERS=0), so the game data a packet may live in must
 * stay below 0x1000000; tools/analysis/field_render_address_guard.py checks
 * that at link time. The 64-bit ELF and PE layouts put .lbss after compact data, followed
 * by .lrodata for immutable, pointer-free host tables. */
#if (defined(__ELF__) || defined(_WIN32)) && defined(__x86_64__)
#define HOST_ONLY_BSS __attribute__((section(".lbss")))
#define HOST_ONLY_RODATA __attribute__((section(".lrodata")))
#else
#define HOST_ONLY_BSS
#define HOST_ONLY_RODATA
#endif

#endif
