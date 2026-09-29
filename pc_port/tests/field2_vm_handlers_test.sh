#!/usr/bin/env bash
# Focused source regression for the first Field 2 VM handlers.  The handlers
# are retail-backed C bodies; this rejects the old generated no-op functions
# and checks the control/data effects which are observable without claiming a
# renderer pass.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/src/field/main/misc6.c"

test -f "$SRC"

party="$(awk '/^void func_800A0228\(void\)/{on=1} /^void func_800A0524\(/ {on=0} on' "$SRC")"
object="$(awk '/^void func_800A0FD8\(void\)/{on=1} /^\/\* Object-sprite load opcode/{on=0} on' "$SRC")"

test -n "$party"
test -n "$object"
! grep -q 'INCLUDE_ASM' <<<"$party"
! grep -q 'INCLUDE_ASM' <<<"$object"
! grep -q 'xeno_port_stub' <<<"$party$object"

# Party/gear path and its non-gear fallback.
grep -q 'GameCharacterGetGearID' <<<"$party"
grep -q 'g_GamePartyMembers\[partySlot\]' <<<"$party"
grep -q 'func_800A0158(partySlot' <<<"$party"
grep -q 'func_8009E574' <<<"$party"
grep -q 'actor->scriptFlags.flags |= 0x200000' <<<"$party"
grep -q 'func_800A0D3C' <<<"$party"
grep -q 'scriptInstructionPointer += 3' <<<"$party"

# Object sub-ops: stop, queue two archive files, and consume/finalize them.
grep -q 'subOp == 0' <<<"$object"
grep -q 'subOp == 1' <<<"$object"
grep -q 'subOp == 2' <<<"$object"
grep -Fq 'ArchiveSetIndex(4, 0)' <<<"$object"
grep -q 'ArchiveCdDataSync(1)' <<<"$object"
grep -Fq 'FieldScriptVMGetArgument(2)' <<<"$object"
grep -q 'func_801E742C' <<<"$object"
grep -q 'D_800B21DC\[slot\]' <<<"$object"
grep -Fq 's32 w = (s16)(slot + 0xFC);' <<<"$object"
grep -Fq 'D_800B220C[slot] = *(s16*)((u8*)object + 0x1C);' <<<"$object"
grep -q 'actor->flags |= 0x2000' <<<"$object"

echo 'field 2 VM handlers structure: PASS'
