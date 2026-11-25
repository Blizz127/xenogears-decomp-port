# Independent host functions

The 172 implementation files listed below supply 445 named functions used by the PC port. An additional 4 shared source files provide menu helpers. The functions cover field scripts, graphics, cameras, dialogue, party state, controller input, archives, fonts, heap management and sound. Host translation units supply the game types and services that the fragments call. Building the full game also requires integration code outside this source release.

The project derived these implementations from observed game behavior and the port's calling conventions. It compared the replacements against original behavior and checked which source files supplied the compiled functions. Coverage varies by function. Some edge cases were exercised through focused tests.

The table lists the reviewed entry points and SHA-256 of each source file.

| File | Entry points | SHA-256 |
| --- | --- | --- |
| [actor_dialogue_queries.inc](pc_port/src/cleanroom/actor_dialogue_queries.inc) | `FieldScriptVMGetActorIndex`, `func_8009CD18` | `ca0221a0c59b7671381d17157d55bec00d3dd7a3f8e57c70c09002465b0f924b` |
| [actor_party_queries.inc](pc_port/src/cleanroom/actor_party_queries.inc) | `FieldActorGetScriptStatus`, `FieldCharacterIdToPartyId` | `a87fa8ab61650c2c6e010f00ff75cff61c9d6f392eee7f224a3a4bdd4186c4c6` |
| [archive_cd_data_sync.inc](pc_port/src/cleanroom/archive_cd_data_sync.inc) | `ArchiveCdDataSync` | `08de98c658840ccdc2d02580a23eb0b27aa337067d3044f54fa70721668ac25e` |
| [archive_drive_command_handler.inc](pc_port/src/cleanroom/archive_drive_command_handler.inc) | `ArchiveCdDriveCommandHandler` | `b96a43a49f5a9e6db8600169c12fec7b7599ac8edae2210d93a9f79038e249c0` |
| [archive_index_init.inc](pc_port/src/cleanroom/archive_index_init.inc) | `ArchiveInit`, `ArchiveSetIndex` | `6bf3267962dd862697e89dc71174fb6f6bbeeaa4d59af7438c142b5fdabd0ddc` |
| [archive_sizes.inc](pc_port/src/cleanroom/archive_sizes.inc) | `ArchiveDecodeSize`, `ArchiveDecodeSizeAbsolute`, `ArchiveDecodeSizeAligned` | `6d64c5ccb2506a1e5728b7b9369bca217a16c4e07dbe15cbf331885ffb884aae` |
| [archive_stream_sections.inc](pc_port/src/cleanroom/archive_stream_sections.inc) | `ArchiveClearStreamFileSections` | `f2a201ab985a5096f04a2abcebda8234bce21d1ae9c7cb0f8b9c88d7a7a64ab6` |
| [controller_input.inc](pc_port/src/cleanroom/controller_input.inc) | `ControllerGetButtonState`, `ControllerGetType`, `ControllerRemapButtonState` | `548dacd4babb1d6b4136428f317fb7c0576c46ac582b41958d16e79e4fde111b` |
| [controller_poll.inc](pc_port/src/cleanroom/controller_poll.inc) | `ControllerPoll` | `d276c7836f7fb16c1301791646827436fd872d798bb1d687ddfb7d610707fc5f` |
| [field_actor_init_shadow_primitives.inc](pc_port/src/cleanroom/field_actor_init_shadow_primitives.inc) | `func_8007AA44` | `35a883817007ea8a625204cc94fcb55c4933b9571a8c0a0470827941cf0a8a00` |
| [field_actor_script_dispatch.inc](pc_port/src/cleanroom/field_actor_script_dispatch.inc) | `func_8009EB78`, `func_8009ED68`, `func_8009F0A0` | `c76e1422732efe4024e14499e0df32bc7e60ef35e4c8a4e1491411641b7f868c` |
| [field_adjust_camera_eye.inc](pc_port/src/cleanroom/field_adjust_camera_eye.inc) | `func_80073684` | `49c8cb733d64fa2086ab7269141be4741b201e93c2b54505d637bb014151717f` |
| [field_angle.inc](pc_port/src/cleanroom/field_angle.inc) | `FieldMathUpdateAngle` | `129d4b29a786ea841df4adbd8c5f29c7f62d7ccda4585ac8debe1e477eeea232` |
| [field_archive_request.inc](pc_port/src/cleanroom/field_archive_request.inc) | `func_8001B484` | `3ec2abc3cb10b383d98d2f067d2ce51c7cc0eee813603f139b2587503a6e6967` |
| [field_camera_movement.inc](pc_port/src/cleanroom/field_camera_movement.inc) | `FieldScriptSetCameraInterpolationStep`, `FieldScriptSetCameraPosMovementDest`, `FieldScriptSetCameraPosMovementFrom`, `FieldScriptSetCameraTargetMovementDest`, `FieldScriptSetCameraTargetMovementFrom`, `FieldScriptWaitForCameraMovement` | `7cb8e5dc12d2ccfcbdb3b9b0e380ea932fea680a5a4af3f253b604f44c95fbcb` |
| [field_clamp_ft4_uv.inc](pc_port/src/cleanroom/field_clamp_ft4_uv.inc) | `FieldClampPolyFT4UVs` | `0b002db90f963b81df14fb459dbf1056d8f01110558b849567e18ca448f3f3ba` |
| [field_compute_scene_matrices.inc](pc_port/src/cleanroom/field_compute_scene_matrices.inc) | `FieldComputeSceneMatrices` | `a7572a4a048e6c498be39df4065214bb03572d1263d442e77660b26b2d0f0fe7` |
| [field_control_flow.inc](pc_port/src/cleanroom/field_control_flow.inc) | `FieldScriptVMHandlerConditionalJmp`, `func_800A17F4`, `func_800A18B8` | `c4ae0a466d4c57ef030b98bc864056266a7bf6cf86ef4eef075040f9958e1790` |
| [field_controller_render.inc](pc_port/src/cleanroom/field_controller_render.inc) | `FieldRenderSyncAndFlush`, `FieldSetControllerBuffers`, `FieldSetMouseSpeed`, `func_800796F4`, `func_8007995C`, `func_8007ADA4` | `c9fefd9ec6486477ebfd5acb4653924b5e255f4c6c8fb111f3aade5e538a5e85` |
| [field_fade_render.inc](pc_port/src/cleanroom/field_fade_render.inc) | `FieldFadeDraw`, `FieldFadeInitializePrimitives` | `e369725c1aba2390fc20743505e8b40443676d6e711682cb40412d2f98f3f406` |
| [field_fade_update.inc](pc_port/src/cleanroom/field_fade_update.inc) | `FieldFadeUpdate` | `5dc8c19e6868cc19e1d6f4477e8a5d997c5f65d397a13535317af0d948855c5c` |
| [field_flag_branch.inc](pc_port/src/cleanroom/field_flag_branch.inc) | `FieldScriptVMConditionalJump6` | `3c1f78826bccffb66dd2ef865762a7cee406f14ff6783449b95d47e4249460a9` |
| [field_initialize_default_particle_banks.inc](pc_port/src/cleanroom/field_initialize_default_particle_banks.inc) | `FieldInitializeDefaultParticleBanks` | `1b63565f1a12c0a4d18799d7422507aeb63a3af67e64ccecb70499d9e0b615b4` |
| [field_initialize_render_contexts.inc](pc_port/src/cleanroom/field_initialize_render_contexts.inc) | `FieldInitializeRenderContexts` | `cb2bb306e15d087b95fcc4df60fa96b0725c89c311303ba61b66cd80651de1cf` |
| [field_initialize_scene_sprites.inc](pc_port/src/cleanroom/field_initialize_scene_sprites.inc) | `func_800ABD18` | `7c82f822c1ace57b517cc8d55cc0255b7f89832889a06ad1a4cedad2c3f0d5ca` |
| [field_initialize_textured_quad.inc](pc_port/src/cleanroom/field_initialize_textured_quad.inc) | `func_8007A7F4` | `e1b7ddbd229179e3dfe60f5705043450c9f088b212c04e06cfa1cd738d134e55` |
| [field_input_script_tests.inc](pc_port/src/cleanroom/field_input_script_tests.inc) | `FieldScriptCheckControllerInputExact`, `FieldScriptVMCheckControllerInput`, `func_80096178`, `func_800961A0` | `629051f76132b0eab4671f6abde8740544ac34928faf9019085575437a07433e` |
| [field_load_tim_with_clut.inc](pc_port/src/cleanroom/field_load_tim_with_clut.inc) | `FieldLoadTIMWithClut` | `1b67b4b653296ea8b22ac203687e25d8ae91b5dc9574ea6c91cb40716144636d` |
| [field_matrix_copy.inc](pc_port/src/cleanroom/field_matrix_copy.inc) | `FieldMatrixCopy` | `f1446b18364b0017180ed1e9eef053f5bacb2f52752adbdb464eaecc2cc393dc` |
| [field_matrix_look_at.inc](pc_port/src/cleanroom/field_matrix_look_at.inc) | `FieldMatrixLookAt` | `eaf867a7f458f020f1d009933cf2025f79a7fe37ab9c1a7c366850c0aadfaf0a` |
| [field_mouse_position.inc](pc_port/src/cleanroom/field_mouse_position.inc) | `FieldSetMousePosition` | `1c2b427422509aa1088f557b1e34c2399030e27c76a5f96f589999c1ccb94563` |
| [field_particle_actor_stop.inc](pc_port/src/cleanroom/field_particle_actor_stop.inc) | `FieldParticleActorStop` | `e54d9bc6b3dcbf5d8e89d0ff51778688a094d2ac83e0e99b94e93f5c56061db1` |
| [field_particle_setup.inc](pc_port/src/cleanroom/field_particle_setup.inc) | `FieldScriptInitializeParticleBank`, `FieldScriptSetParticleBankColor`, `FieldScriptSetParticleBankParameters`, `FieldScriptSetParticleBankPhysics`, `FieldScriptSetParticleBankPosition`, `FieldScriptSetParticleBankScale` | `874735b349abc8903f0b652a0f2e737d2218737e200b0e13a4339a8336712c73` |
| [field_party_adjust_vitals.inc](pc_port/src/cleanroom/field_party_adjust_vitals.inc) | `FieldPartyMemberIncreaseHp`, `FieldPartyMemberDecreaseHp`, `FieldPartyMemberIncreaseMp`, `FieldPartyMemberDecreaseMp` | `fa6c7e7733947297b90decf92985f18d6a5689b024de87cbc57d51727b5542db` |
| [field_party_position_variables.inc](pc_port/src/cleanroom/field_party_position_variables.inc) | `func_800A0158` | `d62526b27152a1cc404643f5d2f54bec310edc59498bc98df19ff027ef457983` |
| [field_party_restore_vitals.inc](pc_port/src/cleanroom/field_party_restore_vitals.inc) | `FieldScriptVMHandlerRestoreHp`, `FieldScriptVMHandlerRestoreMp`, `FieldScriptVMHandlerRestoreCharacterHpAndMp` | `df47fe974ef21d4e2b22746a50d2c8fbaae53e9032c0036e99f60b32b646ea99` |
| [field_party_script_adjust_amount.inc](pc_port/src/cleanroom/field_party_script_adjust_amount.inc) | `FieldScriptVMHandlerDecreasePartyHp`, `FieldScriptVMHandlerIncreasePartyMp`, `FieldScriptVMHandlerDecreasePartyMp` | `f84a3b680f38832fa20bfded0542e809d66536330543093fd04c6b67dea5010b` |
| [field_party_script_vitals.inc](pc_port/src/cleanroom/field_party_script_vitals.inc) | `FieldScriptVMHandlerWritePartyMemberHp`, `FieldScriptVMHandlerWritePartyMemberMp`, `FieldScriptVMHandlerSetPartyMemberHp`, `FieldScriptVMHandlerSetPartyMemberMp` | `f3c5dc65fa7f355e013fb602299dc80a31292289f58921c28649872a1b3b87d0` |
| [field_render_quad.inc](pc_port/src/cleanroom/field_render_quad.inc) | `FieldRenderQuad` | `e7a9bf9cbcb1e4e90dcf2d8f60ee160f61c55b727d614fb938a8627832e8b797` |
| [field_reset_matrix.inc](pc_port/src/cleanroom/field_reset_matrix.inc) | `func_80070594` | `2fbabc50feed3efa6310a01a7a90855eba3f94eb999e64203036228f1bd01c4e` |
| [field_restore_vram.inc](pc_port/src/cleanroom/field_restore_vram.inc) | `func_80078C5C` | `3f9b542e14899d75419c7067725855dc4aa5f661a47ce49b32ae6cd75613d4a1` |
| [field_script_actor_commands.inc](pc_port/src/cleanroom/field_script_actor_commands.inc) | `FieldScriptPartyMemberDisembarkGear`, `FieldScriptPartyMemberRideGear`, `FieldScriptVMHandlerHideActorById`, `func_8009E248`, `func_8009E2C8`, `func_8009E35C`, `func_8009E4BC`, `func_8009F424`, `func_8009F5A8` | `9e56f83103bc3702d7b1817ba555350805177c692f4d7ea48913a289c9f2f462` |
| [field_script_actor_state.inc](pc_port/src/cleanroom/field_script_actor_state.inc) | `FieldScriptVMHandlerDisableDialogActivation`, `FieldScriptVMHandlerEnableDialogActivation`, `FieldScriptVMHandlerHideActor`, `func_8009DA70`, `func_8009DA98`, `func_8009E014`, `func_8009E208`, `func_8009E330`, `func_8009E83C`, `func_8009FC10` | `704a6168a3cff5ce313fe3a3d65607e775dcf429ab36866e936492c5d17c9780` |
| [field_script_animate_door.inc](pc_port/src/cleanroom/field_script_animate_door.inc) | `func_80093E30` | `dbbb237a010caa8e02aa81317164a096a51409190cab5ff220d04e2ede5fd944` |
| [field_script_arguments.inc](pc_port/src/cleanroom/field_script_arguments.inc) | `FieldScriptArgument1`, `FieldScriptArgument2`, `FieldScriptArgument3`, `FieldScriptArgument4`, `FieldScriptArgument5`, `FieldScriptArgument6`, `FieldScriptArgument7`, `FieldScriptArgument8` | `51d159914b5aefe41dd1ce3ea87abbb62474dcb512f61ea7fa7651b8cdfca54b` |
| [field_script_arithmetic_variables.inc](pc_port/src/cleanroom/field_script_arithmetic_variables.inc) | `FieldScriptVMHandlerVariableAdd`, `FieldScriptVMHandlerVariableDiv`, `FieldScriptVMHandlerVariableMul`, `FieldScriptVMHandlerVariableSub` | `3787945ea3b1774619ac4da690ebd2c63f4836cbbc3c41635d17e85f293fff02` |
| [field_script_bitwise_variables.inc](pc_port/src/cleanroom/field_script_bitwise_variables.inc) | `FieldScriptVMHandlerVariableAssign`, `FieldScriptVMHandlerVariableAND`, `FieldScriptVMHandlerVariableOR`, `FieldScriptVMHandlerVariableXOR` | `3031abbe4a65c671169e839db85f93891405c9bd3e6743c2de092846341bff6c` |
| [field_script_branch_on_clear_bit.inc](pc_port/src/cleanroom/field_script_branch_on_clear_bit.inc) | `func_8008D780` | `9cce832355f5300c1900a6b18bd0a16b2074b4089253b2a7acc45de5b14cda6f` |
| [field_script_camera_destination_metrics.inc](pc_port/src/cleanroom/field_script_camera_destination_metrics.inc) | `func_80090E70` | `a217c6ead4ba0ac3d3f4fc0bdfb0d10e5c7790063f3e998043d5916ed24ba133` |
| [field_script_camera_point_about_pivot.inc](pc_port/src/cleanroom/field_script_camera_point_about_pivot.inc) | `func_80091318` | `d7b10d9f0c6ce5858bb4bf316337741c127f852e6e2050dc6e61b64dc5a2ed59` |
| [field_script_check_actor_on_screen.inc](pc_port/src/cleanroom/field_script_check_actor_on_screen.inc) | `FieldScriptCheckActorOnScreen` | `7c035d93f83fd9303bf9b9aebe6721930d0021a53df5e9fc95595ad090d68af0` |
| [field_script_cur_camera_target.inc](pc_port/src/cleanroom/field_script_cur_camera_target.inc) | `FieldScriptWriteCurCameraTarget` | `4ec10836b62f1dd82adda169c3bb1d28cca6bebd4385fb839b21ef931a952b66` |
| [field_script_decrease_gold.inc](pc_port/src/cleanroom/field_script_decrease_gold.inc) | `FieldScriptDecreaseGold` | `88d56ce01ca316c086e822f39907b47be301feeca504380f636e0f5af5cf9ddd` |
| [field_script_dismiss_actor_window.inc](pc_port/src/cleanroom/field_script_dismiss_actor_window.inc) | `func_8009BE9C` | `265370315c2bd756c0ae3ba706cbe998878782dd144ddb64830161926a08fc95` |
| [field_script_entry_offset.inc](pc_port/src/cleanroom/field_script_entry_offset.inc) | `FieldScriptGetBytecodeOffset` | `541273c8eea829a3ab6d21c51729769c1eae00165b5ae36faba4715e93e0514e` |
| [field_script_fade_commands.inc](pc_port/src/cleanroom/field_script_fade_commands.inc) | `FieldScriptFadeIn`, `FieldScriptFadeOut`, `func_800973A4`, `func_80098C00` | `ba3b1411232a57cdd7e2e5b1f02f883688e6e71c0acf866c0bfe0c0334b93a7f` |
| [field_script_move_absolute_xz.inc](pc_port/src/cleanroom/field_script_move_absolute_xz.inc) | `func_80099980` | `6fd3ca7b8e96cf8f357156dd0a79f1ab145778d460322642fd83994f03b1d5ba` |
| [field_script_move_backdrop.inc](pc_port/src/cleanroom/field_script_move_backdrop.inc) | `func_800A476C` | `40a4a122d281fbd2f8027c2aee0c84887a547aa66adbe33a4661722edfc6a739` |
| [field_script_move_retained.inc](pc_port/src/cleanroom/field_script_move_retained.inc) | `func_80098C3C` | `63436cb49be0a8ee3221ec03f0d1229bb99b1d08c5764b2d647ee3901b52f38f` |
| [field_script_random_variables.inc](pc_port/src/cleanroom/field_script_random_variables.inc) | `FieldScriptVMHandlerMulVariableWithRand`, `FieldScriptVMHandlerRandVariable`, `func_8009CF70` | `ee6b7861e5558f30f6015366147f7c56d9b745172cae64b28ae77820dcd54fbe` |
| [field_script_record_gear_map.inc](pc_port/src/cleanroom/field_script_record_gear_map.inc) | `func_8009FD10` | `8ff7a2e74d897ec0ea629ce9b256ef72bba2b91c54314521f3bf62c062e42dc4` |
| [field_script_reset_slots.inc](pc_port/src/cleanroom/field_script_reset_slots.inc) | `func_800A19B0` | `de3c6e0d5c48d43691a1df602f3e9977ae7ef22c5d5101deb0dbfc2d05ec3df5` |
| [field_script_resolve_actor.inc](pc_port/src/cleanroom/field_script_resolve_actor.inc) | `func_8009CD7C` | `43a799323e3ee142760b7f9ee4e0af794365d6479a4e834043b14d7cafef48bb` |
| [field_script_set_actor_flag_groups.inc](pc_port/src/cleanroom/field_script_set_actor_flag_groups.inc) | `func_8009E1A0` | `84a51dd18e0fb3a953c28ed2f50e0150b48ac5225e9c9022ac8ed68cfea1194f` |
| [field_script_set_actor_rotation_component.inc](pc_port/src/cleanroom/field_script_set_actor_rotation_component.inc) | `func_80094918` | `2186df789a0b7fe24e6a3ae9cd1e2cd35112ceac827d702084c44f1d5a79a8a9` |
| [field_script_set_dialog_parameters.inc](pc_port/src/cleanroom/field_script_set_dialog_parameters.inc) | `func_8009CEE0` | `4241db06b3190d9aae1591de134b3bc575facc16851b9bc5146a9a1515a20cbb` |
| [field_script_set_parent_actor.inc](pc_port/src/cleanroom/field_script_set_parent_actor.inc) | `FieldScriptSetParentActor` | `ce784799ea1902e6a15b127e1ee475aa37b335b5cd3229183e79799d5b48e771` |
| [field_script_setup_actor_resource.inc](pc_port/src/cleanroom/field_script_setup_actor_resource.inc) | `func_800A0D3C` | `b07abe723df3deb7a62db14776b256d4bd7b9f6a80c5554e2b90ce312b057366` |
| [field_script_show_actor.inc](pc_port/src/cleanroom/field_script_show_actor.inc) | `FieldScriptVMHandlerShowActor` | `eccaf3ab4621359cb8482dfdc8a421b1acab527f8c8951c416cd8bc7b9368a43` |
| [field_script_slot.inc](pc_port/src/cleanroom/field_script_slot.inc) | `func_800A1A8C` | `f386dc05565be3dd435b157421924420b5d3d275edd13173f80780eb79f3cafa` |
| [field_script_unary_variables.inc](pc_port/src/cleanroom/field_script_unary_variables.inc) | `FieldScriptVMHandlerVariableSetFalse`, `FieldScriptVMHandlerVariableSetTrue`, `FieldScriptVMHandlerIncVariable`, `FieldScriptVMHandlerDecVariable` | `3448ebc99a4946eb4eac63e4d30ef5fcf0581d7c0a155fd2b8c2d36fc383c09b` |
| [field_script_update_idle_facing.inc](pc_port/src/cleanroom/field_script_update_idle_facing.inc) | `func_8009F4CC` | `d9ec4b73643f78f1eaef3f50f46c5ff4d03ddb5350fc3b2b0cab3dd81bf8598b` |
| [field_script_variable_shifts.inc](pc_port/src/cleanroom/field_script_variable_shifts.inc) | `FieldScriptVMHandlerLShiftVariable`, `FieldScriptVMHandlerRShiftVariable`, `FieldScriptVMHandlerVariableSetBit`, `FieldScriptVMHandlerVariableUnsetBit` | `b6acd2d9eee83f55fd56b48a80735318ab3e73c11afcd6f7405d7d9137834dfc` |
| [field_swap_render_context.inc](pc_port/src/cleanroom/field_swap_render_context.inc) | `FieldSwapRenderContext` | `5197988435536018fb798be941b96644e30defc089b810fbce4cae72f1d887c7` |
| [field_text_box_has_visible_portrait.inc](pc_port/src/cleanroom/field_text_box_has_visible_portrait.inc) | `func_8009C538` | `8ef9817be1b6ee0311cae12d3e7b3bdc31c1ea46bcc7eb465ad368231b834827` |
| [field_wds_services.inc](pc_port/src/cleanroom/field_wds_services.inc) | `func_80083994`, `func_80085F30`, `func_80085FB8`, `func_80086024` | `7621777e5d8732ae62889b00b195ca159cf48012b1c0a09c51282fbcc52ef07c` |
| [field_zoom_fade_colors.inc](pc_port/src/cleanroom/field_zoom_fade_colors.inc) | `func_800A5600` | `2c217c0da4815524faf3f75484fb863680151e672631bb727f873e71d5b4d051` |
| [font_controls.inc](pc_port/src/cleanroom/font_controls.inc) | `FontGetCurLetterCount`, `FontGetCurLetterX`, `FontGetCurLetterY`, `FontGetDefaultLetterHeight`, `FontGetDefaultLetterWidth`, `FontGetFlags`, `FontGetLetterFlags`, `FontRemoveFlag`, `FontRemoveLetterFlag`, `FontResetLetterRendering`, `FontRestoreCurrentPositions`, `FontSetColor`, `FontSetCurLetterX`, `FontSetCurLetterY`, `FontSetFlag`, `FontSetLetterFlag`, `FontSetLetterWidth`, `FontSetPositionsByOffset`, `FontSetRowHeight`, `FontStoreCurrentPositions`, `func_8003708C` | `afa5075704fdda64ef04cd9ee60be7db67b38a428031dc15f9dfb84047dd8603` |
| [font_draw_letters.inc](pc_port/src/cleanroom/font_draw_letters.inc) | `FontDrawLetters` | `287c65b7f59c0f9b993c00c917b66d401349e7e6ae30ccf12eb869cdbcb65df7` |
| [font_glyph.inc](pc_port/src/cleanroom/font_glyph.inc) | `FontAddLetterPrimitive` | `f44c8e67dbf43db4f2dc5ea5b32a0ea9fab48b47529faec955756fe033f347e3` |
| [font_load_clut.inc](pc_port/src/cleanroom/font_load_clut.inc) | `FontLoadClut` | `80e4d9cc893d3d1b7bea01d1f79c39f9649486f3b4da217e64068299942b83ac` |
| [font_load_font.inc](pc_port/src/cleanroom/font_load_font.inc) | `FontLoadFont` | `55101b956f61ab481bb7ebc0e9a406ef851776f5eac144eb1dead08ff0d2c2d4` |
| [font_resource_binding.inc](pc_port/src/cleanroom/font_resource_binding.inc) | `func_8003747C` | `5f64424d7a84633f24bfc3e5aff5bb14d444384a718a7fdd5a6eea38b2a33ee6` |
| [font_services.inc](pc_port/src/cleanroom/font_services.inc) | `FontFree`, `FontPrintf` | `a72a0eaf1a4e60f06085e3d2928e4da2747b0a818a961901c5667f011a4d70a5` |
| [gfx_line_scroll_initialize.inc](pc_port/src/cleanroom/gfx_line_scroll_initialize.inc) | `GfxLineScrollInitialize` | `4ffa7018ef254a31728b4d64aa71d5e710197c401e70bb90d60b865093dd08c7` |
| [gfx_line_scroll_update.inc](pc_port/src/cleanroom/gfx_line_scroll_update.inc) | `GfxLineScrollUpdate` | `a06983354007edb8ca8c250ee3e20eca33f7ee3ef3adfd100f0298e55ecd066f` |
| [heap_allocate.inc](pc_port/src/cleanroom/heap_allocate.inc) | `HeapAlloc` | `e9877daf7d061dac6a17c406a3cd587d75091442615f978069e01948fa4590b4` |
| [heap_debug_services.inc](pc_port/src/cleanroom/heap_debug_services.inc) | `HeapGetSymbolNameFromAddress`, `HeapLoadSymbols`, `HeapPrintf` | `8ce8f91b68744cd929c9e47f15d36ed6739d4c769f245797e1e4f0ea0275bc35` |
| [heap_dump_block.inc](pc_port/src/cleanroom/heap_dump_block.inc) | `HeapDebugDumpBlock` | `dbed976d98da4ce5f4c40101fd1bd825028fc858c46c636daff0edaead895f99` |
| [heap_dump_file.inc](pc_port/src/cleanroom/heap_dump_file.inc) | `HeapDumpToFile` | `4ac6566bd34fcf2b741073dd6562141ac47ea268b15e67ba8506b68521af619a` |
| [heap_dump_report.inc](pc_port/src/cleanroom/heap_dump_report.inc) | `HeapDebugDump` | `3b802551033f9c02070dde1f936a3600b8f1fd13fb6474e10d0aec0471cc7797` |
| [heap_insert_block.inc](pc_port/src/cleanroom/heap_insert_block.inc) | `HeapInsertAlloc` | `3d09f3260304900fd7e25a3b74ce9f6953f130ca612677e1ea575dd1dbd84ee1` |
| [heap_list_sweeps.inc](pc_port/src/cleanroom/heap_list_sweeps.inc) | `HeapConsolidate`, `HeapForceFreeAllBlocks`, `HeapFreeAllBlocks`, `HeapFreeAllDelayedBlocks`, `HeapFreeBlocksWithFlag`, `HeapTickDelayedFree` | `e9ddb90f555c29ee0cfcda3dc96041cbd18fc60410c84cb877a864dbab80db59` |
| [heap_metadata.inc](pc_port/src/cleanroom/heap_metadata.inc) | `HeapChangeCurrentUser`, `HeapGetAllocInformation`, `HeapGetBlockSourceAddress`, `HeapGetBlockUser`, `HeapGetCurrentUser`, `HeapIsBlockPinned`, `HeapPinBlock`, `HeapSetCurrentContentType`, `HeapSetCurrentUser`, `HeapToggleErrorHandler`, `HeapUnpinBlockCopy`, `func_800318F0` | `dbe57a81eaabd83a3da011ce7c880ed211959f390cbd8980af12a721ab873cf7` |
| [heap_queries.inc](pc_port/src/cleanroom/heap_queries.inc) | `HeapGetLargestFreeBlockSize`, `HeapGetTotalFreeSize`, `HeapWalkUntilEnd` | `bc660cdbdf2331047fe62d2e2bf0b472742e549fc1c170c056b22cbf98c76311` |
| [heap_release_block.inc](pc_port/src/cleanroom/heap_release_block.inc) | `HeapDelayedFree`, `HeapFree` | `9a1b77334fdee38e0c9735468f6ad3d928d3ebb09e5d539635625630b63d40bc` |
| [heap_service_controls.inc](pc_port/src/cleanroom/heap_service_controls.inc) | `HeapAllocSound`, `HeapCalloc`, `HeapForceFree`, `HeapInit`, `HeapRelocate`, `HeapReset` | `65aa7e7aeff88af991cb5d4ff1f445da1e24f3d7d0207b083f567467a0abe754` |
| [heap_unpin_checked.inc](pc_port/src/cleanroom/heap_unpin_checked.inc) | `HeapUnpinBlock` | `f7113bc23ca0c0ee3c4da67c78948732b9e8ffc0bc0a90fae91293f5d47c61a5` |
| [kernel_menu_cursor.inc](pc_port/src/cleanroom/kernel_menu_cursor.inc) | `KernelMenuInitializeCursor` | `540191ab03ae8d7b12dd42acc748f4e42d2544955cd25e81f8b7fd5a97fd1729` |
| [kernel_menu_init.inc](pc_port/src/cleanroom/kernel_menu_init.inc) | `KernelMenuInitialize` | `907a834af8b3bbd7f4edb1cc2abee9f5a3149dced160cb5fe07877ffcdfce465` |
| [kernel_menu_main.inc](pc_port/src/cleanroom/kernel_menu_main.inc) | `KernelMenuMain` | `4d28a6f330709e8daeaa5c3995e7d945cbdb23adbc1b5a8870dac8f3fb66dcc9` |
| [kernel_menu_update.inc](pc_port/src/cleanroom/kernel_menu_update.inc) | `KernelMenuUpdate` | `33383cc1b341055ff7f8e38224075243463e326f6ae2ace9d73cdf57eff873c2` |
| [member_change_character.inc](pc_port/src/cleanroom/member_change_character.inc) | `MemberChangeMenuUpdateCharacter` | `1e5bf23a088f280840e1f9cb63be0f68e01380819c0dc1b029c8bc7c9f78fc9e` |
| [member_change_character_labels.inc](pc_port/src/cleanroom/member_change_character_labels.inc) | `func_801C9A08` | `c9cd60c50989516b1dfeda43e92a1be34c03901444b0176463804a182ce9be29` |
| [member_change_character_selection.inc](pc_port/src/cleanroom/member_change_character_selection.inc) | `MemberChangeMenuSetCursorToCharacter`, `MemberChangeMenuUpdateCharacters` | `85c6d1da81d7c87e52cd66bf555674531f002e13922f8d775c85b8b8b44d1510` |
| [member_change_character_strings.inc](pc_port/src/cleanroom/member_change_character_strings.inc) | `MemberChangeMenuSetCharacterHpAndMpStrings`, `MemberChangeMenuSetCharacterLevelStrings` | `d996f9eed21217fddfe1857f4d0e7cd1db7aec3ca8b927826ef6b018669cb155` |
| [member_change_main.inc](pc_port/src/cleanroom/member_change_main.inc) | `MemberChangeMenuMain` | `c083d4c06d5f9f71742bf173bab50a090e7ece533726049f6fc4b1300a7ea08d` |
| [member_change_render.inc](pc_port/src/cleanroom/member_change_render.inc) | `MemberChangeMenuRender` | `8c925b5ccb1bf55e9a0dd7c9145dab46054f6d468672a2cc7b6bb4b845726479` |
| [member_change_render_characters.inc](pc_port/src/cleanroom/member_change_render_characters.inc) | `MemberChangeMenuRenderCharacter`, `MemberChangeMenuRenderCharacters` | `ff9c99f8097b08dbf4c03b299f605ea105f1106aba9b13b6196c8da582a7309b` |
| [member_change_services.inc](pc_port/src/cleanroom/member_change_services.inc) | `MemberChangeMenuFreeWindow`, `MemberChangeMenuPlaySoundEffect`, `MemberChangeMenuRenderBackgroundDim`, `func_801C5CF4`, `func_801C5D24`, `func_801C7DA8`, `func_801C7E38`, `func_801C846C` | `269200c3bfcbd038ff015c0ed4b3057acab860e2cc190e2e1f0688e45c464265` |
| [member_change_state.inc](pc_port/src/cleanroom/member_change_state.inc) | `MemberChangeMenuIsCharacterFlagSet`, `MemberChangeMenuResetRenderContext`, `MemberChangeMenuSetCharacters`, `MemberChangeMenuSetManager`, `MemberChangeMenuSetSelectionMenu`, `func_801C5160`, `func_801C5228`, `func_801C5724` | `aa839effd612b511c609b5ae3bb03439160a0fcef5ccd89c4121175ef5c55cc9` |
| [member_change_window.inc](pc_port/src/cleanroom/member_change_window.inc) | `MemberChangeMenuSetWindow` | `ec7e434220c4a6f43bcfb7ac7fd9266cada4e5fec4061e7fb28e7006364bdc6c` |
| [member_initialize.inc](pc_port/src/cleanroom/member_initialize.inc) | `MemberChangeMenuInitialize` | `b6b2f0486026b2eabb17903d0b999d2fcffe718048cb57dd8fead151f808b6ed` |
| [member_menu_cursors.inc](pc_port/src/cleanroom/member_menu_cursors.inc) | `MemberChangeMenuDrawCursors`, `MemberChangeMenuInitializeCursors` | `570a70d1e8ba3bb2b64e83b5f65aa0f962aa25b33f0f7897a82862e231835c99` |
| [member_menu_frame_input_windows.inc](pc_port/src/cleanroom/member_menu_frame_input_windows.inc) | `MemberChangeMenuPollInput`, `MemberChangeMenuUpdateAndRender`, `MemberChangeMenuUpdateWindows` | `1367f0e882241ffd48a615b0807ada1f3dfae76bed9d102195292ddafe52739e` |
| [member_menu_lifecycle.inc](pc_port/src/cleanroom/member_menu_lifecycle.inc) | `MemberChangeMenuFree`, `MemberChangeMenuFreeCursors`, `MemberChangeMenuRenderWindows` | `150624d78ac79e57be299d23b83b0ceed2c54b8323e7fdd0b95e4d6462afffd2` |
| [member_menu_number.inc](pc_port/src/cleanroom/member_menu_number.inc) | `MemberChangeMenuParseNumberToString` | `88b8a408176574bc0c028c7069ffa6c73f5d1be19e475e64e07ddbb8843faa01` |
| [member_menu_roster_windows.inc](pc_port/src/cleanroom/member_menu_roster_windows.inc) | `MemberChangeMenuInitializeWindow`, `func_801C7EC8`, `func_801C9908` | `fb1cba1c04809cd9caafb9384da5bbc53bb47a45dbeed7677fd2d9e1d48abf90` |
| [member_menu_selection.inc](pc_port/src/cleanroom/member_menu_selection.inc) | `MemberChangeMenuMainLoop` | `bbc2cbd5778495ebae74b79f44c13544b56fd26a729899978ee85e1f2a02d8b8` |
| [member_menu_storage.inc](pc_port/src/cleanroom/member_menu_storage.inc) | `func_801C5034`, `func_801C51C4` | `1b8793903c35d6d85bfbb796c5d404f54a2f111d0fcc56b87cb4cbb6e991f693` |
| [member_menu_window_edges.inc](pc_port/src/cleanroom/member_menu_window_edges.inc) | `MemberChangeMenuSetWindowBorderBottom`, `MemberChangeMenuSetWindowBorderLeft`, `MemberChangeMenuSetWindowBorderRight`, `MemberChangeMenuSetWindowBorderTop` | `c99bf9b1b94ebcaf048d221321a6deb102df19f441edd07be98171a20805d6c4` |
| [member_menu_window_graphics.inc](pc_port/src/cleanroom/member_menu_window_graphics.inc) | `MemberChangeMenuInitializeWindowGraphics` | `94b193fe27c3b5dbeb949f1eb3de9e0524bc4571fa58e13c9fc7781a9a13c1a8` |
| [member_menu_window_shape.inc](pc_port/src/cleanroom/member_menu_window_shape.inc) | `MemberChangeMenuSetVertices`, `MemberChangeMenuSetWindowBorderPrimitive` | `daf8c53d0d043bc8858570c42932b849bc79ce4e448f8ccef61886d84fb0b8e6` |
| [member_window_atlas.inc](pc_port/src/cleanroom/member_window_atlas.inc) | `MemberChangeMenuInitializeWindowBorders` | `5b9ef2e842abbfdbf3cbeff335583b5720ab82ed2b5974e7170ffde532fc4e60` |
| [member_window_corner_init.inc](pc_port/src/cleanroom/member_window_corner_init.inc) | `MemberChangeMenuInitializeWindowBorderCorners` | `b1420340524e4d30273c1e813044bb0a3dcfb5a265e2e9923ab587ea52fe5add` |
| [member_window_edges.inc](pc_port/src/cleanroom/member_window_edges.inc) | `MemberChangeMenuRenderBottomWindowBorder`, `MemberChangeMenuRenderLeftWindowBorder`, `MemberChangeMenuRenderRightWindowBorder`, `MemberChangeMenuRenderTopWindowBorder` | `0033de8844947184d821059db8b6d94436493dd362f4974914b2fd299b888993` |
| [member_window_parts.inc](pc_port/src/cleanroom/member_window_parts.inc) | `MemberChangeMenuRenderScrollBar`, `MemberChangeMenuRenderWindowBackground`, `MemberChangeMenuRenderWindowBorderCorners` | `110f64086bb8b35f60a553412e55cb4a18b5eeb4f4e5f4aa9637539e2574df8a` |
| [member_window_scroll_init.inc](pc_port/src/cleanroom/member_window_scroll_init.inc) | `MemberChangeMenuInitializeScrollBar` | `9537b2c7fde822dd6a014929a0b39cffc839ae83e80e7b2604d6212881af8ec8` |
| [menu_gfx_environment.inc](pc_port/src/cleanroom/menu_gfx_environment.inc) | `MenuInitializeGfxEnvironment`, `MenuInitializeGfxEnvironments` | `d07d4157202d023387ac1425643297d2229c5156b666dc54ca9ba6a2de41a70e` |
| [menu_input_and_dispatch.inc](pc_port/src/cleanroom/menu_input_and_dispatch.inc) | `MenuMain`, `MenuProcessControllerInput`, `func_8001BEEC` | `3edd6efe61508577c3699b47f61d31f28f09c9fb0a73684514e0987dcd9f6784` |
| [party_skin_initializers.inc](pc_port/src/cleanroom/party_skin_initializers.inc) | `GamePartyCharactersInitializeSkins`, `GamePartyGearsInitializeSkins` | `2a7f2310844b3f5dd7eee9ff810f844d6950e5d3e40b940170cb679a65025769` |
| [party_stream_sync.inc](pc_port/src/cleanroom/party_stream_sync.inc) | `GamePartySyncStreamedData` | `d01704ed86f76cb900821b479a5c668b3a3578b37e7e96cc98a6e7a83546acc9` |
| [scenario_flags.inc](pc_port/src/cleanroom/scenario_flags.inc) | `FieldScriptCheckScenarioFlagsEqual`, `FieldScriptCheckScenarioFlagsGreaterThan`, `FieldScriptCheckScenarioFlagsLessThan` | `3c35b5e34107715a0e2f33819518e67a55ab68bbcc58c0c8ee5fe9b1b4617140` |
| [shop_initialize.inc](pc_port/src/cleanroom/shop_initialize.inc) | `ShopMenuInitialize` | `2853f1a1c27f5416cc85a3340149fda771badbe31ac243acad43ecf5006268b5` |
| [shop_menu_number.inc](pc_port/src/cleanroom/shop_menu_number.inc) | `ShopMenuParseNumberToString` | `e65dee78c6a229b46eb1a6aac29a6788e215054dcf53f5213e905c06d7285f6d` |
| [shop_menu_state_and_quad.inc](pc_port/src/cleanroom/shop_menu_state_and_quad.inc) | `ShopMenuGetCharacterBitMask`, `ShopMenuIsCharacterFlagSet`, `ShopMenuResetRenderContext`, `ShopMenuSelectionMenuManager`, `ShopMenuSetManager`, `ShopMenuShopManager`, `ShopMenuUnk1E20Manager`, `ShopMenuUnk348Manager`, `ShopMenuUnk354Manager`, `func_801C5040` | `c70401b280f38e7e114962cf0cc3708ea7d815e6afa3b2b1084e3ac48cf05258` |
| [shop_menu_storage.inc](pc_port/src/cleanroom/shop_menu_storage.inc) | `ShopMenuUnk32CManager`, `ShopMenuUnk330Manager` | `57a31484fa2ac31ff8dca37f13dd720747e32d715dd8bf09e2765489a1f7fc0c` |
| [sound_apply_volume_settings.inc](pc_port/src/cleanroom/sound_apply_volume_settings.inc) | `SoundApplyVolumeSettings` | `3c05681d568315a9d3b6e6132311286f98db1b32853f2e52701abf2f27abda5c` |
| [sound_calculate_audio_manager_size.inc](pc_port/src/cleanroom/sound_calculate_audio_manager_size.inc) | `SoundCalculateAudioManagerSize` | `2c0ffc6ab1c37c4588c8131ad814f9c5ae9ad331d6c30648c00f1e16ef7e55f4` |
| [sound_heap_allocate.inc](pc_port/src/cleanroom/sound_heap_allocate.inc) | `SoundHeapAllocate` | `cba63e3f5edb6aa92428998942eb9e7fafc236551073f0c4e8cd5f7588b6fddf` |
| [sound_heap_memory.inc](pc_port/src/cleanroom/sound_heap_memory.inc) | `SoundHeapClearBlockMemory`, `SoundHeapInitialize`, `SoundHeapSetBlockMemory`, `SoundSpuMemoryFindBlock` | `53a1f6cc97c538836182fad1b04684db9e6d4141180306ffbceca0562d8632c9` |
| [sound_manager_flags_validation.inc](pc_port/src/cleanroom/sound_manager_flags_validation.inc) | `SoundSetFlagsOnActiveVoices`, `SoundValidateFile`, `func_80039CC4`, `unk_SoundSetFlagsOnActiveVoices` | `e68f788a39ea698abbb69d01de60c87c70c770fe259dc7b724f4aa5a2e78f982` |
| [sound_manager_init_cd.inc](pc_port/src/cleanroom/sound_manager_init_cd.inc) | `SoundSetCdAttr`, `SoundInitializeAudioManager` | `6774b6f2dd93ea37d8a939e06eb30a2b159a4c641b5430bcdccadfafd32dc168` |
| [sound_manager_stop_fade.inc](pc_port/src/cleanroom/sound_manager_stop_fade.inc) | `func_80039C4C`, `func_80039C8C` | `f24b0ee4ad6ddc7e14d37dda28d75fdf5227a347639a6aec8a99fef748d2bd83` |
| [sound_mixer_controls.inc](pc_port/src/cleanroom/sound_mixer_controls.inc) | `func_80037F44`, `func_80037F88`, `func_80038624`, `func_8003869C`, `func_80038824`, `func_800388D4`, `func_8003890C` | `6ce8c48f47a72f5e581c915040ba49441a719ecc4888034b5149e27c3b305dbf` |
| [sound_record_lists.inc](pc_port/src/cleanroom/sound_record_lists.inc) | `SoundAddSedsEntry`, `SoundFreeWdsEntry`, `SoundRemoveAudioManagerFromList` | `4c8b075ce44428cc7e7ef6c59c12be8c5c627f5a89415ad320417a5fe0f4a015` |
| [sound_record_services.inc](pc_port/src/cleanroom/sound_record_services.inc) | `SoundAddAudioManagerToList`, `SoundCopyAudioManagerData`, `SoundFileComputeChecksum`, `SoundFindWdsEntry`, `SoundSpuMemoryAllocateWDS` | `72fab8e35fd20d5ddeab17678398ea7aac7a0c63ca47c42e68fbbf3fcef49996` |
| [sound_reset_sequence.inc](pc_port/src/cleanroom/sound_reset_sequence.inc) | `SoundReset` | `227b2da46fc5c8aca507aa17d0476f55e3e81aff6e37e749069f7dcc554f4676` |
| [sound_reverb_controls.inc](pc_port/src/cleanroom/sound_reverb_controls.inc) | `SoundInitiateReverbWorkAreaTransfer`, `SoundSetReverbModeWithAllocation` | `166a1d10d4f9d439089d78f6779bbdb87085f7027ab7ffd489e8f1ccc505e1d6` |
| [sound_script_active_flag800.inc](pc_port/src/cleanroom/sound_script_active_flag800.inc) | `SoundScriptClearActiveFlag800`, `SoundScriptSetActiveFlag800` | `04b3ba4e368046682e66cae639bb35ce67a3e932741565ac1078e40899ef511f` |
| [sound_script_cursor_ops.inc](pc_port/src/cleanroom/sound_script_cursor_ops.inc) | `SoundScriptDefaultHandler`, `SoundScriptDefaultSkip3`, `SoundScriptNop`, `SoundScriptNop2`, `SoundScriptNop3`, `SoundScriptNop4`, `SoundScriptNop5`, `SoundScriptSkip3` | `1f9954baa4ba3d7fa26c288b68497c74687d021aa631afcffac59891fc8e462e` |
| [sound_script_envelope_ops.inc](pc_port/src/cleanroom/sound_script_envelope_ops.inc) | `SoundScriptSetAdsrDRAndSR`, `SoundScriptSetAttackMode`, `SoundScriptSetDecayTime`, `SoundScriptSetRelease`, `SoundScriptSetReleaseMode`, `SoundScriptSetSustainLevel`, `SoundScriptSetSustainMode` | `085e45d16475deb6efe5f941922287d75c35ba8726edcbef4d7978f4a3649dcc` |
| [sound_script_note_parameters.inc](pc_port/src/cleanroom/sound_script_note_parameters.inc) | `SoundScriptFermata`, `SoundScriptSetAttackTime`, `SoundScriptSetOctave`, `SoundScriptSetSustain` | `67289b2ae988ea3e482f72e436c6d3372c920e0869307dd25fe125778a381e8a` |
| [sound_script_pitch_controls.inc](pc_port/src/cleanroom/sound_script_pitch_controls.inc) | `SoundScriptAddUnk6E`, `SoundScriptLowerOctave`, `SoundScriptRaiseOctave`, `SoundScriptSetUnk74` | `ad51c3747692a1335499baf454b810a7e7c26b88ba11c3ab7a7f59a7d2b23e47` |
| [sound_script_save_and_interpolator.inc](pc_port/src/cleanroom/sound_script_save_and_interpolator.inc) | `SoundScriptSaveOctaveAndIP`, `SoundTickInterpolator` | `8fc87d5ce2c5e0e11d91873d981f6471a464ea0d5c31b5ccf3f2750f9304e49c` |
| [sound_script_state_ops.inc](pc_port/src/cleanroom/sound_script_state_ops.inc) | `SoundScriptAddManagerUnk1a`, `SoundScriptCallE5BC`, `SoundScriptClearUnkCEAndF6`, `SoundScriptClearUnkCEAndUnk116`, `SoundScriptSetManagerUnk1a`, `SoundScriptSetUnkCEAndF6`, `SoundScriptSetUnkCEAndUnk116`, `SoundScriptSetUnkCEAndUnk136` | `c5f73447fb5c2a6785df80e87702c2eb3f42cc56a4b5ead21dc2ac2eded1db9c` |
| [sound_script_voice_flags.inc](pc_port/src/cleanroom/sound_script_voice_flags.inc) | `SoundScriptClearUnk04Bit1`, `SoundScriptPercussionOff`, `SoundScriptSetUnk62`, `SoundScriptSetUnk6E`, `SoundScriptSetVoiceFlags2000AndMode`, `SoundScriptSetVoiceFlags2000ClearMode`, `SoundScriptSetVoiceFlags4000ClearMode`, `SoundScriptToggleUnk04Bit2` | `fd66956ae4bb645a8499724bc3950a98872eec2dbab6b6ed0520f6e452c039c8` |
| [sound_sequencer_control_services.inc](pc_port/src/cleanroom/sound_sequencer_control_services.inc) | `func_80039748`, `func_80039D2C`, `func_80039E18`, `func_80039EC4`, `func_8003AFA0` | `02662387941e7cc369cb36b105a8bac5accf7f48fc85a50edb27a40dba69bc97` |
| [sound_spu_irq_allocator.inc](pc_port/src/cleanroom/sound_spu_irq_allocator.inc) | `SoundSetSpuIrqCallback`, `SoundSpuIRQHandler`, `SoundSpuMemoryFreeBlock` | `0f8bf97a90b5b2650c33bda7f50b57d2184bc2e0a14b73ddb6b5ba0f7b046edf` |
| [sound_spu_memory_get_free_block.inc](pc_port/src/cleanroom/sound_spu_memory_get_free_block.inc) | `SoundSpuMemoryGetFreeBlock` | `a2bcd3dae53a73413f39849347c4729b795a8c679829cee014e70266d7b08a7c` |
| [sound_spu_memory_initialize.inc](pc_port/src/cleanroom/sound_spu_memory_initialize.inc) | `SoundSpuMemoryInitialize` | `5ee5fb4f443c78398f0e2ebcf4d0c1dbfabae142b194b2b4f4920410fddeafda` |
| [sound_spu_voice_envelope.inc](pc_port/src/cleanroom/sound_spu_voice_envelope.inc) | `SoundSetVoiceAdsrAttackModeAndRate`, `SoundSetVoiceAdsrDecayShift`, `SoundSetVoiceAdsrSustainLevel`, `SoundSetVoiceAdsrSustainRateAndDirection` | `f2627b091737010a3be76ebfb2d2b88adf7752cc49ec9ad91ef45dec19041664` |
| [sound_spu_voice_registers.inc](pc_port/src/cleanroom/sound_spu_voice_registers.inc) | `SoundSetReverbVoices`, `SoundSetVoiceKeyOn`, `SoundSetVoicePitch`, `SoundSetVoiceStartAddress`, `SoundSetVoiceVolume` | `1dadc4095e06f372f79146fd13193a3496b4542b26dcea9b118c7d16ddd51afb` |
| [sound_transfer_controls.inc](pc_port/src/cleanroom/sound_transfer_controls.inc) | `SoundQueueSpuWriteCommand`, `SoundTransferQueueSync`, `SoundWdsSetTransferParamters` | `a2a79d14183e1510ee3306fbc01bd3c9ac1c887398d2a502a0cd66b0b5406007` |
| [sound_transfer_queue_services.inc](pc_port/src/cleanroom/sound_transfer_queue_services.inc) | `SoundExecuteReverbWorkAreaTransfer`, `SoundOnTransferCallback`, `SoundProcessTransferCommand`, `SoundQueueTransferCommand` | `a433a0f3bb961a98115ccd52904ce5225d303801deca27ba6ad594f175d834ff` |
| [sound_transfer_reserved_ops.inc](pc_port/src/cleanroom/sound_transfer_reserved_ops.inc) | `SoundQueueSpuReadCommand`, `SoundUnkDebugNoReturn_8003F4BC`, `func_8003977C`, `func_80039D24`, `func_8003A2D4`, `func_8003A2DC`, `func_8003BC58`, `func_8003BC7C`, `func_8003BDF4` | `37a37288655f831dce9491d3983ffe55ff48c1dec3cfa07846b49fcb1f4d4d8d` |
| [sound_voice_channel_lifecycle.inc](pc_port/src/cleanroom/sound_voice_channel_lifecycle.inc) | `SoundAssignVoiceToChannelAndPlay`, `SoundAssignVoiceToChannelAndStop`, `SoundReleaseVoiceFromChannel`, `SoundStopVoiceOnChannel` | `374ff896f7b760ce0283b4a3fd76bff7fa5d3db092bdbb9827b4930e1e2975fc` |
| [sound_voice_loop_address.inc](pc_port/src/cleanroom/sound_voice_loop_address.inc) | `SoundSetVoiceLoopAddress` | `f842d0b8124cf520667b9ff02e859ad2ad74fc087f354411ace42d827ea3655c` |
| [sound_voice_management.inc](pc_port/src/cleanroom/sound_voice_management.inc) | `SoundAbortAllVoices`, `SoundAbortVoiceOnChannel`, `SoundAssignVoiceToChannel`, `SoundReleaseAllVoices` | `2a09fffbc6e1a62c30a0376911a26677a83565d929c51c4741810ccb00508a67` |
| [sound_volume_fades_phase.inc](pc_port/src/cleanroom/sound_volume_fades_phase.inc) | `SoundSetCdVolumeWithFade`, `SoundSetMasterVolumeWithFade`, `SoundSetVolumeWithPhase` | `116e12f8f793fca385fa56f85a99aa38b3901f01eba9f69e5a7438d90eb93521` |
| [text_box_lifecycle.inc](pc_port/src/cleanroom/text_box_lifecycle.inc) | `func_8007FFE8`, `func_800805F4` | `14208792ae3b43ba143fd811b898908ace25e56406572afe60322eb4670622e8` |
| [text_box_windows.inc](pc_port/src/cleanroom/text_box_windows.inc) | `func_8007E114`, `func_800807B4` | `1a15fbee1f895cc8e70dd05a62a7a8b905ccb531e3b432d00a5862b049e49f5d` |

Shared helpers supply menu code to more than one translation unit. The catalog counts the entry points that use them.

| Supporting source | SHA-256 |
| --- | --- |
| [menu_initialize_party.h](pc_port/src/cleanroom/menu_initialize_party.h) | `c3be02d188fc93a2971f0f6241003c80f1ddc2a01121d7f35a9286acfa8b7859` |
| [menu_numeric_digits.h](pc_port/src/cleanroom/menu_numeric_digits.h) | `d795e07d375bd3a62d2db4224bb28c707d6c9162832cebbbfde45fa0213af5bb` |
| [menu_storage_slots.h](pc_port/src/cleanroom/menu_storage_slots.h) | `bc7ff60ae3135f4d5cd527ec68a34cb78df73bbf2f1ee5def1a4f41b176589bd` |
| [menu_system_palette.inc](pc_port/src/cleanroom/menu_system_palette.inc) | `fd5d33a098a8b9ea5dcb3ff778e84bebdd6505f5f3bc802a806bf37605b08268` |

## Run the field-script regression

From the root of this source distribution, build and run the actor selector
and viewport tests with a C17 compiler. The tests provide their own actor
state and service functions.

```sh
cc -std=c17 -Wall -Wextra -Werror -O2 -Icommon/pc_port/src/cleanroom common/tests/field_script_test.c -o /tmp/xeno-field-test
/tmp/xeno-field-test
```

## Test the field and archive functions

The synthetic archive test checks cache hits, cache misses and game-service
calls. The fade test checks the color bytes of the active polygons in both
render contexts. The VRAM and party-position tests exercise mock graphics
services and variable getters. Run the tests from the root of the distribution.

```sh
for test in field_archive_request field_zoom_fade_colors field_restore_vram field_party_position_variables
do
    cc -std=c11 -Wall -Wextra -Werror -O2 -Icommon/pc_port/src/cleanroom "common/tests/${test}_test.c" -o "/tmp/xeno-${test}-test" && "/tmp/xeno-${test}-test" || exit 1
done
```

## Test the sound, script and party functions

These standalone tests supply their own data, game types and services.
Use Bash and GCC or Clang on Linux. The scripts exercise unoptimized and
optimized builds and undefined-behavior checks. Instrumented fixtures check
which memory accesses and service calls occur, and in which order. The CD
fixture runs a simulated Windows audio service and checks that the port
holds its critical section from the first attribute read through submission.

```sh
for suite in sound_script_active_flags sound_script_note_parameters sound_script_sequence_controls \
    sound_voice_channel_lifecycle sound_transfer_controls sound_reverb_controls \
    field_input_script_tests field_script_arguments field_script_unary_variables \
    field_party_adjust_vitals field_party_script_vitals field_party_masked_adjust \
    field_party_restore_vitals field_script_bitwise_variables
do
    bash "common/pc_port/tests/$suite/run.sh" || exit 1
done
```

The heap, font, sequencer and field-variable fixtures can be run from the repository root with GCC or Clang. Each uses synthetic inputs and checks both optimized and unoptimized builds as well as undefined-behavior instrumentation.

```sh
for suite in field_script_arithmetic_variables field_script_random_variables field_script_variable_shifts \
    font heap_native_services sound_mixer_controls \
    sound_reset_sequence sound_script_cursor_ops sound_script_envelope_ops \
    sound_script_state_ops sound_script_voice_flags sound_sequencer_control_services \
    sound_spu_irq_allocator sound_spu_voice_envelope sound_spu_voice_registers \
    sound_transfer_reserved_ops sound_voice_management
do
    bash "common/pc_port/tests/$suite/run.sh" || exit 1
done
```
