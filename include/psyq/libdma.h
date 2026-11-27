#ifndef XENO_DISC_AND_SOUND_DMA_H
#define XENO_DISC_AND_SOUND_DMA_H

/* DMA register layout: https://psx-spx.consoledev.net/dmachannels/
 * US Disc 1, disc/SLUS_006.64, SHA-256
 * dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119.
 * CD_getsector / CD_getsector2 (80042AA8 / 80042BA8) program channel 3.
 * The chopped transfer grants the CPU a 16-cycle window. */
enum {
    DMA_CHCR_START_BUSY_ENABLED = 1u << 24,
    DMA_CHCR_CDROM_NORMAL = DMA_CHCR_START_BUSY_ENABLED | (1u << 28),
    DMA_CHCR_CDROM_CHOPPED = DMA_CHCR_CDROM_NORMAL | (4u << 20) | (1u << 8)
};

/* _spu_t (8004CCA8) uses 16 words per block and request-mode DMA. */
enum {
    DMA_BLOCK_SIZE = 64,
    DMA_CHCR_SPU_READ = DMA_CHCR_START_BUSY_ENABLED | (1u << 9),
    DMA_CHCR_SPU_WRITE = DMA_CHCR_SPU_READ | 1u,
    DMA_TRANSFER_TIMEOUT = -2,
    DMA_TRANSFER_SUCCESS = 0
};

/* Both signed and unsigned source counters use an unsigned comparison. */
#define DMA_TIMEOUT 3840u

/* _spu_init (8004C6DC) and _spu_FsetPCR (8004D150) use channel 4.
 * Each DMA channel occupies four bits in DPCR. */
enum { DMA_DPCR_DMA4_PRIORITY_SHIFT = 4 * 4 };
#define XENO_SPU_DMA_FIELD(value) ((value) << DMA_DPCR_DMA4_PRIORITY_SHIFT)

enum {
    DMA_DPCR_MASK_DMA4_PRIORITY = XENO_SPU_DMA_FIELD(7u),
    DMA_DPCR_MASK_DMA4_ENABLE = XENO_SPU_DMA_FIELD(8u),
    DMA_PRIORITY_HIGH = 3,
    DMA_PRIORITY_MEDIUM = 5,
    DMA_DPCR_SPU_PRIORITY_HIGH = XENO_SPU_DMA_FIELD(DMA_PRIORITY_HIGH)
};
#undef XENO_SPU_DMA_FIELD

#endif
