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

set $last_a1_slot = -1
set $last_a1_ip = -1
set $last_a9_slot = -1
set $last_a9_ip = -1
set $chain_events = 0

break src/field/scripts/virtual_machine.c:249
condition 1 g_FieldActors != 0 && ((char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[1].pActorData || (char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[9].pActorData)
commands 1
  silent
  set $trace_actor = 9
  if (char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[1].pActorData
    set $trace_actor = 1
  end
  set $trace_data = (unsigned char*)g_FieldScriptVMCurActor
  set $trace_slot = *(unsigned char*)($trace_data + 0xce)
  set $trace_ip = *(unsigned short*)($trace_data + 0xcc)
  set $trace_pc = (unsigned char*)g_FieldScriptVMCurScriptData + $trace_ip
  set $trace_changed = 0
  if $trace_actor == 1 && ($trace_slot != $last_a1_slot || $trace_ip != $last_a1_ip)
    set $trace_changed = 1
    set $last_a1_slot = $trace_slot
    set $last_a1_ip = $trace_ip
  end
  if $trace_actor == 9 && ($trace_slot != $last_a9_slot || $trace_ip != $last_a9_ip)
    set $trace_changed = 1
    set $last_a9_slot = $trace_slot
    set $last_a9_ip = $trace_ip
  end
  if $trace_changed == 1
    set $chain_events = $chain_events + 1
    printf "SCRIPT_CHAIN event=%d actor=%d slot=%d ip=%04x op=%02x bytes=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x pos=%d,%d state=%d flags=%08x\n", $chain_events, $trace_actor, $trace_slot, $trace_ip, $trace_pc[0], $trace_pc[0], $trace_pc[1], $trace_pc[2], $trace_pc[3], $trace_pc[4], $trace_pc[5], $trace_pc[6], $trace_pc[7], *(int*)($trace_data + 0x20) >> 16, *(int*)($trace_data + 0x28) >> 16, *(unsigned char*)($trace_data + 0x96 + $trace_slot * 8), *(unsigned int*)($trace_data + 0x00)
  end
  if $trace_actor == 1 && $trace_ip == 0x009f
    printf "SCRIPT_CHAIN_REACHED_NEXT_DIALOGUE events=%d pos=%d,%d\n", $chain_events, *(int*)($trace_data + 0x20) >> 16, *(int*)($trace_data + 0x28) >> 16
    x/56bx (unsigned char*)g_FieldScriptVMCurScriptData + 0x0068
    x/48bx (unsigned char*)g_FieldScriptVMCurScriptData + 0x01e8
    quit
  end
  if $chain_events >= 512
    printf "SCRIPT_CHAIN_LIMIT events=%d\n", $chain_events
    quit
  end
  continue
end

run > /dev/null 2>&1
