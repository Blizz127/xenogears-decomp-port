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

set $alice_hits = 0

break func_80099980
condition 1 g_FieldActors != 0 && (char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[1].pActorData && *(unsigned short*)((char*)g_FieldScriptVMCurActor + 0xcc) == 0x0074
commands 1
  silent
  set $alice_hits = $alice_hits + 1
  set $alice_actor = (unsigned char*)g_FieldScriptVMCurActor
  printf "ALICE_WALK hit=%d ip=%04x pos=%d,%d move=%d,%d rot=%04x tri=%d wm=%d flags0=%08x flags4=%08x slot=%08x\n", $alice_hits, *(unsigned short*)($alice_actor + 0xcc), *(int*)($alice_actor + 0x20) >> 16, *(int*)($alice_actor + 0x28) >> 16, *(int*)($alice_actor + 0x30), *(int*)($alice_actor + 0x38), *(unsigned short*)($alice_actor + 0x104), *(short*)($alice_actor + 0x08), *(short*)($alice_actor + 0x10), *(unsigned int*)($alice_actor + 0x00), *(unsigned int*)($alice_actor + 0x04), *(unsigned int*)($alice_actor + 0x90)
  if $alice_hits >= 64
    printf "ALICE_WALK_STALLED hits=%d\n", $alice_hits
    quit
  end
  continue
end

break src/field/main/misc7.c:972
condition 2 g_FieldActors != 0 && (char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[1].pActorData && *(unsigned short*)((char*)g_FieldScriptVMCurActor + 0xcc) == 0x007a
commands 2
  silent
  set $alice_actor = (unsigned char*)g_FieldScriptVMCurActor
  printf "ALICE_WALK_COMPLETE hits=%d ip=%04x pos=%d,%d\n", $alice_hits, *(unsigned short*)($alice_actor + 0xcc), *(int*)($alice_actor + 0x20) >> 16, *(int*)($alice_actor + 0x28) >> 16
  quit
end

run > /dev/null 2>&1
