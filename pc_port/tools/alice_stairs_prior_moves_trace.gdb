set pagination off
set confirm off
set debuginfod enabled off

set environment DISPLAY :98
set environment SDL_VIDEODRIVER x11
set environment SDL_AUDIODRIVER dummy
set environment XENO_FIELD_TEST 1
set environment XENO_KERNEL_SEL 0
set environment XENO_FIELD_MAP 12
set environment XENO_FIELD_ENTRANCE 0

set $fei_move_hits = 0
set $stalled_hits = 0

break func_80099980
condition 1 g_FieldActors != 0 && (char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 1
  silent
  set $fei_move_hits = $fei_move_hits + 1
  set $fei_actor = (unsigned char*)g_FieldScriptVMCurActor
  set $fei_ip = *(unsigned short*)($fei_actor + 0xcc)
  set $fei_pc = (unsigned char*)g_FieldScriptVMCurScriptData + $fei_ip
  printf "FEI_MOVE hit=%d slot=%d ip=%04x bytes=%02x,%02x,%02x,%02x,%02x,%02x pos=%d,%d origin=%d,%d move=%d,%d tri=%d wm=%d flags0=%08x flags4=%08x\n", $fei_move_hits, *(unsigned char*)($fei_actor + 0xce), $fei_ip, $fei_pc[0], $fei_pc[1], $fei_pc[2], $fei_pc[3], $fei_pc[4], $fei_pc[5], *(int*)($fei_actor + 0x20) >> 16, *(int*)($fei_actor + 0x28) >> 16, *(int*)($fei_actor + 0xd0), *(int*)($fei_actor + 0xd8), *(int*)($fei_actor + 0x30), *(int*)($fei_actor + 0x38), *(short*)($fei_actor + 0x08), *(short*)($fei_actor + 0x10), *(unsigned int*)($fei_actor + 0x00), *(unsigned int*)($fei_actor + 0x04)
  if $fei_ip == 0x0074
    set $stalled_hits = $stalled_hits + 1
    if $stalled_hits >= 4
      printf "FEI_MOVE_TRACE_DONE total=%d stalled=%d\n", $fei_move_hits, $stalled_hits
      quit
    end
  end
  if $fei_move_hits >= 256
    printf "FEI_MOVE_TRACE_LIMIT total=%d\n", $fei_move_hits
    quit
  end
  continue
end

run > /dev/null 2>&1
