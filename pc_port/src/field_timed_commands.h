#ifndef XENO_FIELD_TIMED_COMMANDS_H
#define XENO_FIELD_TIMED_COMMANDS_H
#include <stdint.h>
/* Exact E5D44 control/data flow, with CPU provenance made mandatory.
 * inheritedStack50 is the halfword at retail entry SP-0x38, not a mode
 * invented by the port. retailObjectAddress supplies E0A00's incoming s1.
 * Neither context value has a default; callers must establish both. */
void PcPort_FieldTimedCommandsWithContext(uint8_t* object, void* pool,
    int32_t frame, uint32_t retailObjectAddress, uint16_t inheritedStack50);
#endif
