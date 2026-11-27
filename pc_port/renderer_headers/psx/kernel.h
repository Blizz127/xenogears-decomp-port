#ifndef XENO_RENDERER_KERNEL_H
#define XENO_RENDERER_KERNEL_H

/* LIBAPI's counter-2 dispatcher and the native card backend pass these
 * identifiers to DeliverEvent. The dispatcher selects callbacks at 0x1000. */
#define RCntCNT2 0xf2000002U
#define SwCARD   0xf4000001u
enum {
    EvSpINT = 2,
    EvSpIOE = 4,
    EvSpTIMOUT = 256,
    EvMdINTR = 0x1000
};

/* MemCardGetDirentry writes an array of these host records. firstfile and
 * nextfile fill the same record in pc_port/src/memcard_port.c. */
enum { XenoCardNameBytes = 20, XenoCardSystemBytes = 4 };
struct DIRENTRY {
    char name[XenoCardNameBytes];
    int attr, size;
    struct DIRENTRY *next;
    int head;
    char system[XenoCardSystemBytes];
};

#endif
