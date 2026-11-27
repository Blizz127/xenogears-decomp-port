#ifndef XENO_MENU_CHOICE_REVEAL_H
#define XENO_MENU_CHOICE_REVEAL_H

#include <stdint.h>

typedef uint32_t (*PcMenuChoiceFrame)(void *owner);

/* The item and Gear shops use the same two-frame label reveal, with their
 * respective screen's frame callback. A nonzero result stops at that frame. */
uint32_t PcPort_ShopMenuRevealChoices(int stages, const int *textureIds,
                                     PcMenuChoiceFrame frame, void *owner);

/* The category strip contains four cursor/label pairs per shop mode. The
 * original waits two complete frames for each visible cursor or label. */
uint32_t PcPort_ShopMenuRevealCategories(unsigned char variant, const int *tables,
                                        PcMenuChoiceFrame frame, void *owner);

#endif
