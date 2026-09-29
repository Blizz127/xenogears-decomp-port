#ifndef PC_PORT_BATTLE_TARGET_BOUNDS_H
#define PC_PORT_BATTLE_TARGET_BOUNDS_H
#include <stdint.h>
typedef struct PcPortBattleTargetPoint {
    uint8_t suppressed;
    const int32_t *position; /* Three signed words; NULL means absent. */
} PcPortBattleTargetPoint;
typedef struct PcPortBattleTargetBounds {
    uint32_t count;
    int32_t center[3];
} PcPortBattleTargetBounds;
/* Staged BC460 integer selection/center stage, not a full BC460 replacement.
 * Eleven stable, already-resolved inputs; output must not alias input data.
 * Keeps retail wrapping sums, floor shifts and mean-seeded extrema. Zero
 * contributors produce zero center and count. Returns -1 for NULL arguments.
 * Does not store C3678, model guest frames, invoke matrices, or render. */
int PcPortBattleComputeTargetBounds(uint32_t mask,
    const PcPortBattleTargetPoint points[11], PcPortBattleTargetBounds *result);
/* BC7FC..BC858 radius accumulation for an already-projected packed screen
 * point. Returns the signed maximum of previous bits and the wrapped square
 * sum. This is not projection, square root, or final camera distance. */
uint32_t PcPortBattleAccumulateTargetRadius(uint32_t maximum,
    uint16_t screen_x, uint16_t screen_y);
#endif
