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

set $alice_walk_hits = 0
set $alice_collision_hits = 0
set $alice_active = 0

break func_80099980
condition 1 g_FieldActors != 0 && (char*)g_FieldScriptVMCurActor == (char*)(unsigned long)g_FieldActors[1].pActorData && *(unsigned short*)((char*)g_FieldScriptVMCurActor + 0xcc) == 0x0074
commands 1
  silent
  set $alice_walk_hits = $alice_walk_hits + 1
  set $alice_active = 1
  set $alice_actor = (unsigned char*)g_FieldScriptVMCurActor
  printf "ALICE_WALK hit=%d pos=%d,%d move=%d,%d rot=%04x tri=%d wm=%d flags12c=%08x clip=%08x\n", $alice_walk_hits, *(int*)($alice_actor + 0x20) >> 16, *(int*)($alice_actor + 0x28) >> 16, *(int*)($alice_actor + 0x30), *(int*)($alice_actor + 0x38), *(unsigned short*)($alice_actor + 0x104), *(short*)($alice_actor + 0x08), *(short*)($alice_actor + 0x10), *(unsigned int*)($alice_actor + 0x12c), *(unsigned int*)($alice_actor + 0x114)
  if $alice_walk_hits >= 12
    quit
  end
  continue
end

break src/field/main/misc4.c:1722
condition 2 $alice_active == 1 && (char*)actorData == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 2
  silent
  set $alice_collision_hits = $alice_collision_hits + 1
  printf "ALICE_COLLISION hit=%d side=%d tri=%d last=%d steps=%d move=%d,%d edge=%d,%d,%d,%d,%d,%d,%d\n", $alice_collision_hits, sideMask, triIndex, lastTri, steps, move[0], move[2], outEdge[0], outEdge[1], outEdge[2], outEdge[4], outEdge[5], outEdge[6], outEdge[7]
  if $alice_collision_hits == 1
    bt 8
  end
  continue
end

break src/field/main/misc8.c:1043
condition 3 $alice_active == 1 && (char*)actorData == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 3
  silent
  printf "ALICE_CLIP candidate=%d,%d packed=%08x polygon=%08x,%08x,%08x,%08x\n", x, z, position, point0, point1, point2, point3
  continue
end

break src/field/main/misc8.c:1353
condition 4 $alice_active == 1 && (char*)actorData == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 4
  silent
  printf "ALICE_PRECOLLISION vec=%d,%d angle=%04x sprite=%d,%d actorMove=%d,%d\n", vec[0], vec[2], angle, *(int*)(spriteData + 0x0c), *(int*)(spriteData + 0x14), *(int*)(actorData + 0x40), *(int*)(actorData + 0x48)
  continue
end

break src/field/main/misc8.c:1405
condition 5 $alice_active == 1 && (char*)actorData == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 5
  silent
  printf "ALICE_WALKMESH result=%p vec=%d,%d tri=%d wm=%d\n", result, vec[0], vec[2], *(short*)(actorData + 0x08), *(short*)(actorData + 0x10)
  continue
end

break src/field/main/misc8.c:2544
condition 6 $alice_active == 1 && (char*)actorData == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 6
  silent
  printf "ALICE_APPLY pos=%d,%d move=%d,%d tri=%d wm=%d\n", *(int*)(actorData + 0x20) >> 16, *(int*)(actorData + 0x28) >> 16, *(int*)(actorData + 0x30), *(int*)(actorData + 0x38), *(short*)(actorData + 0x08), *(short*)(actorData + 0x10)
  continue
end

break src/field/main/misc8.c:2647
condition 7 $alice_active == 1 && (char*)actorData == (char*)(unsigned long)g_FieldActors[1].pActorData
commands 7
  silent
  printf "ALICE_RESTORE pos=%d,%d move=%d,%d tri=%d wm=%d\n", *(int*)(actorData + 0x20) >> 16, *(int*)(actorData + 0x28) >> 16, *(int*)(actorData + 0x30), *(int*)(actorData + 0x38), *(short*)(actorData + 0x08), *(short*)(actorData + 0x10)
  continue
end

run > /dev/null 2>&1
