set pagination off
set debuginfod enabled off

break func_800A7C58
commands
  silent
  printf "A7C map=%d D20=%d D2A=%d D2C=%d D2E=%d music=%d mode=%d\n", g_GameSceneMapNum, D_800C3A20, D_800C3A2A, D_800C3A2C, D_800C3A2E, D_800C3A38, D_800ADB74
  continue
end

break func_80085788
commands
  silent
  printf "BGM map=%d music=%d file=0x%x\n", g_GameSceneMapNum, D_800C3A38, D_800C3A38 + 0x115
  continue
end

break func_800855C8
commands
  silent
  printf "SFX map=%d id=0x%x vol=%d pan=%d chan=%d\n", g_GameSceneMapNum, soundId, volume, pan, channel
  continue
end

break func_8008F7B8
commands
  silent
  printf "MUSIC_OP map=%d fieldId=%d current=%d state=%d bankState=%d\n", g_GameSceneMapNum, FieldScriptVMGetArgument(1), D_8004F324, D_8004F308, D_8004F354
  continue
end

break func_80085B20
commands
  silent
  printf "MUSIC_LOAD map=%d id=%d current=%d state=%d bankState=%d\n", g_GameSceneMapNum, a0, D_8004F324, D_8004F308, D_8004F354
  continue
end

break func_80085C90
commands
  silent
  if D_8004F358 == 1 || D_800AFC54 == 1
    printf "MUSIC_POLL map=%d id=%d cur=%d load=%d queued=%d bankState=%d\n", g_GameSceneMapNum, a0, D_8004F338, D_8004F308, D_800AFC54, D_8004F354
  end
  continue
end

run
