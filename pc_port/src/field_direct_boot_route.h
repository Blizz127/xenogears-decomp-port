#ifndef XENO_PC_FIELD_DIRECT_BOOT_ROUTE_H
#define XENO_PC_FIELD_DIRECT_BOOT_ROUTE_H

/* Retail 800198C0..800198D4: D_8004FE45 is the movie selector, not
 * the disc number. D_8004FE44 selects movie stream type 1 separately. */
static inline unsigned char PcPort_SelectBootMovie(int discNumber)
{
    return discNumber == 1 ? 16u : 7u;
}

/* XENO_FIELD_TEST without a selector intentionally opens the developer
 * KernelMenu.  A forced selection of Field (0), however, is a harness route:
 * entering FieldMain directly prevents a rendered debug-menu page from being
 * recycled by later retail field transitions. */
static inline unsigned int PcPort_SelectBootState(
    const char *fieldTest, const char *kernelSelection)
{
    if (fieldTest == 0 || fieldTest[0] != '1') {
        return 6u;
    }
    if (kernelSelection != 0 && kernelSelection[0] == '0' &&
        kernelSelection[1] == '\0') {
        return 1u;
    }
    return 0u;
}

#endif
