#!/usr/bin/env bash
# Retail field/model records contain 32-bit pointer slots.  On the LP64 host,
# reading or writing those slots through void* or u8** changes the record
# layout and consumes the adjacent word.  This focused source regression keeps
# the audited functions on explicit four-byte accesses while retaining
# independent source/destination actor indices.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MODEL_SRC="$ROOT/src/slus_006.64/system/temp2.c"
FIELD_SRC="$ROOT/src/field/main/misc6.c"

test -f "$MODEL_SRC"
test -f "$FIELD_SRC"

model_fn="$(awk '/^void func_8002C8CC\(/{on=1} /^void func_8002CB54\(/{on=0} on' "$MODEL_SRC")"
teardown_fn="$(awk '/^void func_8002CBBC\(/{on=1} /^extern s32 D_80059310/{on=0} on' "$MODEL_SRC")"
field_fn="$(awk '/^void func_800A0524\(/{on=1} /^extern void func_800A0C94/{on=0} on' "$FIELD_SRC")"

test -n "$model_fn"
test -n "$teardown_fn"
test -n "$field_fn"

# func_8002C8CC's packed model slot +0x18 is a retail sw/lw slot.
! grep -Eq '\*\(void\*\*\)\(s0 \+ 0x18\)' <<<"$model_fn"
grep -Eq '\*\(u32\*\)\(s0 \+ 0x18\) = \(u32\)\(uintptr_t\)HeapAlloc' <<<"$model_fn"
! grep -Eq '\*\(void\*\*\).*0x18|0x18.*\*\(void\*\*\)' <<<"$teardown_fn"
grep -Fq 'HeapFree((void*)(uintptr_t)*(u32*)(modelData + 0x18));' <<<"$teardown_fn"

# func_800A0524's actor slots +0x4C/+0x04 are also four-byte guest pointers.
! grep -Eq '\*\(u8\*\*\)' <<<"$field_fn"
grep -Fq 'u8* pSrcData = (u8*)(uintptr_t)*(u32*)(pSrcField + 0x4C);' <<<"$field_fn"
grep -Fq 'u8* pDstData = (u8*)(uintptr_t)*(u32*)(pDstField + 0x4C);' <<<"$field_fn"
grep -Fq 'u8* pSrcSub = (u8*)(uintptr_t)*(u32*)(pSrcField + 0x4C);' <<<"$field_fn"
grep -Fq 'u8* pDstSprite = (u8*)(uintptr_t)*(u32*)(pDstField + 0x04);' <<<"$field_fn"

# Keep both caller-provided actor indices (including nonzero indices) and the
# retail 92-byte FieldActor stride visible in the implementation.
grep -Fq 'srcIdx * 92' <<<"$field_fn"
grep -Fq 'dstIdx * 92' <<<"$field_fn"

echo 'field pointer-width containment and indexed actor semantics: PASS'
