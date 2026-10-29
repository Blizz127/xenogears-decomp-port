#!/usr/bin/env bash
# Retail differential for XENO_PC_PORT arms of system/temp2.c not covered by run_main_exe_temp2_retail_test.sh.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
source pc_port/tests/lib/slus_oracle_lib.sh
source pc_port/tests/lib/slus_suite_lib.sh
slus_check_disc
MUTS=(
 "func_8002BA58|*(u16*)(slot + 0) = 3;|*(u16*)(slot + 0) = 2;"
 "func_800303C8|HeapSetCurrentContentType(0x2C);|HeapSetCurrentContentType(0x2D);"
 "func_800305D8|T2_P(*(u32*)(pJoint + 4))|T2_P(*(u32*)(pJoint + 4) + 1)"
 "func_80031798|0x04000000u;|0x03000000u;"
 "func_800317E0|func_800317E0, 0x03000000|func_800317E0, 0x02000000"
 "func_80031804|func_80031804, 0x03000000|func_80031804, 0x02000000"
)
# func_80031798 routes through the REAL production PcPort_AddPrimDomainAware:
# pre-build it per regime into the dep glob that slus_link picks up (mutant
# O2 links reuse the same dep objects).
OUT_LINK=pc_port/build_native/main_exe_sl_temp2
mkdir -p "$OUT_LINK"
for mode in ${MAIN_EXE_SL_TEMP2_MODES:-O0 O2 UBSan}; do
  slus_flags "$mode"
  gcc "${SLUS_COMMON[@]}" "${SLUS_GAME[@]}" "${SLUS_FL[@]}" -fpermissive -w \
    -c pc_port/src/guest_prim_link.c -o "$OUT_LINK/$mode.dep.guestprim.o"
done
slus_suite sl_temp2 "MAIN EXE SL TEMP2" pc_port/tests/main_exe_sl_temp2_retail_test.c \
  "${MAIN_EXE_SL_TEMP2_SOURCE:-src/slus_006.64/system/temp2.c}" "func_800301C8 func_80030228 func_800302D4" 0 \
  "${MUTS[@]}"
