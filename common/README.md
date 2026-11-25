# Independent host functions

These 79 C include files supply 145 named functions used by the PC port. The functions cover field scripts, graphics, cameras, dialogue, party state, controller input, archives and sound. Host translation units supply the game types and services that the fragments call. Building the full game also requires integration code outside this source release.

The project derived these implementations from observed game behavior and the port's calling conventions. It compared the replacements against original behavior and checked which source files supplied the compiled functions. Coverage varies by function. Some edge cases were exercised through focused tests.

The table lists all published entry points and file SHA-256 values. The
three newest fragments match the private Linux `d2250a55` and Windows
`611d96f6` sources.

| File | Entry points | SHA-256 |
| --- | --- | --- |
| [actor_dialogue_queries.inc](pc_port/src/cleanroom/actor_dialogue_queries.inc) | `FieldScriptVMGetActorIndex`, `func_8009CD18` | `ca0221a0c59b7671381d17157d55bec00d3dd7a3f8e57c70c09002465b0f924b` |
| [actor_party_queries.inc](pc_port/src/cleanroom/actor_party_queries.inc) | `FieldActorGetScriptStatus`, `FieldCharacterIdToPartyId` | `a87fa8ab61650c2c6e010f00ff75cff61c9d6f392eee7f224a3a4bdd4186c4c6` |
| [archive_index_init.inc](pc_port/src/cleanroom/archive_index_init.inc) | `ArchiveInit`, `ArchiveSetIndex` | `6bf3267962dd862697e89dc71174fb6f6bbeeaa4d59af7438c142b5fdabd0ddc` |
| [archive_sizes.inc](pc_port/src/cleanroom/archive_sizes.inc) | `ArchiveDecodeSize`, `ArchiveDecodeSizeAbsolute`, `ArchiveDecodeSizeAligned` | `6d64c5ccb2506a1e5728b7b9369bca217a16c4e07dbe15cbf331885ffb884aae` |
| [archive_stream_sections.inc](pc_port/src/cleanroom/archive_stream_sections.inc) | `ArchiveClearStreamFileSections` | `f2a201ab985a5096f04a2abcebda8234bce21d1ae9c7cb0f8b9c88d7a7a64ab6` |
| [controller_input.inc](pc_port/src/cleanroom/controller_input.inc) | `ControllerGetButtonState`, `ControllerGetType`, `ControllerRemapButtonState` | `548dacd4babb1d6b4136428f317fb7c0576c46ac582b41958d16e79e4fde111b` |
| [controller_poll.inc](pc_port/src/cleanroom/controller_poll.inc) | `ControllerPoll` | `d276c7836f7fb16c1301791646827436fd872d798bb1d687ddfb7d610707fc5f` |
| [field_actor_init_shadow_primitives.inc](pc_port/src/cleanroom/field_actor_init_shadow_primitives.inc) | `func_8007AA44` | `35a883817007ea8a625204cc94fcb55c4933b9571a8c0a0470827941cf0a8a00` |
| [field_adjust_camera_eye.inc](pc_port/src/cleanroom/field_adjust_camera_eye.inc) | `func_80073684` | `49c8cb733d64fa2086ab7269141be4741b201e93c2b54505d637bb014151717f` |
| [field_angle.inc](pc_port/src/cleanroom/field_angle.inc) | `FieldMathUpdateAngle` | `129d4b29a786ea841df4adbd8c5f29c7f62d7ccda4585ac8debe1e477eeea232` |
| [field_archive_request.inc](pc_port/src/cleanroom/field_archive_request.inc) | `func_8001B484` | `3ec2abc3cb10b383d98d2f067d2ce51c7cc0eee813603f139b2587503a6e6967` |
| [field_camera_movement.inc](pc_port/src/cleanroom/field_camera_movement.inc) | `FieldScriptSetCameraInterpolationStep`, `FieldScriptSetCameraPosMovementDest`, `FieldScriptSetCameraPosMovementFrom`, `FieldScriptSetCameraTargetMovementDest`, `FieldScriptSetCameraTargetMovementFrom`, `FieldScriptWaitForCameraMovement` | `7cb8e5dc12d2ccfcbdb3b9b0e380ea932fea680a5a4af3f253b604f44c95fbcb` |
| [field_clamp_ft4_uv.inc](pc_port/src/cleanroom/field_clamp_ft4_uv.inc) | `FieldClampPolyFT4UVs` | `0b002db90f963b81df14fb459dbf1056d8f01110558b849567e18ca448f3f3ba` |
| [field_compute_scene_matrices.inc](pc_port/src/cleanroom/field_compute_scene_matrices.inc) | `FieldComputeSceneMatrices` | `a7572a4a048e6c498be39df4065214bb03572d1263d442e77660b26b2d0f0fe7` |
| [field_control_flow.inc](pc_port/src/cleanroom/field_control_flow.inc) | `FieldScriptVMHandlerConditionalJmp`, `func_800A17F4`, `func_800A18B8` | `c4ae0a466d4c57ef030b98bc864056266a7bf6cf86ef4eef075040f9958e1790` |
| [field_fade_render.inc](pc_port/src/cleanroom/field_fade_render.inc) | `FieldFadeDraw`, `FieldFadeInitializePrimitives` | `e369725c1aba2390fc20743505e8b40443676d6e711682cb40412d2f98f3f406` |
| [field_fade_update.inc](pc_port/src/cleanroom/field_fade_update.inc) | `FieldFadeUpdate` | `5dc8c19e6868cc19e1d6f4477e8a5d997c5f65d397a13535317af0d948855c5c` |
| [field_flag_branch.inc](pc_port/src/cleanroom/field_flag_branch.inc) | `FieldScriptVMConditionalJump6` | `3c1f78826bccffb66dd2ef865762a7cee406f14ff6783449b95d47e4249460a9` |
| [field_initialize_default_particle_banks.inc](pc_port/src/cleanroom/field_initialize_default_particle_banks.inc) | `FieldInitializeDefaultParticleBanks` | `1b63565f1a12c0a4d18799d7422507aeb63a3af67e64ccecb70499d9e0b615b4` |
| [field_initialize_render_contexts.inc](pc_port/src/cleanroom/field_initialize_render_contexts.inc) | `FieldInitializeRenderContexts` | `cb2bb306e15d087b95fcc4df60fa96b0725c89c311303ba61b66cd80651de1cf` |
| [field_initialize_scene_sprites.inc](pc_port/src/cleanroom/field_initialize_scene_sprites.inc) | `func_800ABD18` | `7c82f822c1ace57b517cc8d55cc0255b7f89832889a06ad1a4cedad2c3f0d5ca` |
| [field_initialize_textured_quad.inc](pc_port/src/cleanroom/field_initialize_textured_quad.inc) | `func_8007A7F4` | `e1b7ddbd229179e3dfe60f5705043450c9f088b212c04e06cfa1cd738d134e55` |
| [field_input_script_tests.inc](pc_port/src/cleanroom/field_input_script_tests.inc) | `FieldScriptCheckControllerInputExact`, `FieldScriptVMCheckControllerInput`, `func_80096178`, `func_800961A0` | `629051f76132b0eab4671f6abde8740544ac34928faf9019085575437a07433e` |
| [field_load_tim_with_clut.inc](pc_port/src/cleanroom/field_load_tim_with_clut.inc) | `FieldLoadTIMWithClut` | `1b67b4b653296ea8b22ac203687e25d8ae91b5dc9574ea6c91cb40716144636d` |
| [field_matrix_look_at.inc](pc_port/src/cleanroom/field_matrix_look_at.inc) | `FieldMatrixLookAt` | `eaf867a7f458f020f1d009933cf2025f79a7fe37ab9c1a7c366850c0aadfaf0a` |
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
| [field_script_animate_door.inc](pc_port/src/cleanroom/field_script_animate_door.inc) | `func_80093E30` | `dbbb237a010caa8e02aa81317164a096a51409190cab5ff220d04e2ede5fd944` |
| [field_script_arguments.inc](pc_port/src/cleanroom/field_script_arguments.inc) | `FieldScriptArgument1`, `FieldScriptArgument2`, `FieldScriptArgument3`, `FieldScriptArgument4`, `FieldScriptArgument5`, `FieldScriptArgument6`, `FieldScriptArgument7`, `FieldScriptArgument8` | `51d159914b5aefe41dd1ce3ea87abbb62474dcb512f61ea7fa7651b8cdfca54b` |
| [field_script_bitwise_variables.inc](pc_port/src/cleanroom/field_script_bitwise_variables.inc) | `FieldScriptVMHandlerVariableAssign`, `FieldScriptVMHandlerVariableAND`, `FieldScriptVMHandlerVariableOR`, `FieldScriptVMHandlerVariableXOR` | `3031abbe4a65c671169e839db85f93891405c9bd3e6743c2de092846341bff6c` |
| [field_script_branch_on_clear_bit.inc](pc_port/src/cleanroom/field_script_branch_on_clear_bit.inc) | `func_8008D780` | `9cce832355f5300c1900a6b18bd0a16b2074b4089253b2a7acc45de5b14cda6f` |
| [field_script_camera_destination_metrics.inc](pc_port/src/cleanroom/field_script_camera_destination_metrics.inc) | `func_80090E70` | `a217c6ead4ba0ac3d3f4fc0bdfb0d10e5c7790063f3e998043d5916ed24ba133` |
| [field_script_camera_point_about_pivot.inc](pc_port/src/cleanroom/field_script_camera_point_about_pivot.inc) | `func_80091318` | `d7b10d9f0c6ce5858bb4bf316337741c127f852e6e2050dc6e61b64dc5a2ed59` |
| [field_script_check_actor_on_screen.inc](pc_port/src/cleanroom/field_script_check_actor_on_screen.inc) | `FieldScriptCheckActorOnScreen` | `7c035d93f83fd9303bf9b9aebe6721930d0021a53df5e9fc95595ad090d68af0` |
| [field_script_cur_camera_target.inc](pc_port/src/cleanroom/field_script_cur_camera_target.inc) | `FieldScriptWriteCurCameraTarget` | `4ec10836b62f1dd82adda169c3bb1d28cca6bebd4385fb839b21ef931a952b66` |
| [field_script_decrease_gold.inc](pc_port/src/cleanroom/field_script_decrease_gold.inc) | `FieldScriptDecreaseGold` | `88d56ce01ca316c086e822f39907b47be301feeca504380f636e0f5af5cf9ddd` |
| [field_script_dismiss_actor_window.inc](pc_port/src/cleanroom/field_script_dismiss_actor_window.inc) | `func_8009BE9C` | `265370315c2bd756c0ae3ba706cbe998878782dd144ddb64830161926a08fc95` |
| [field_script_move_absolute_xz.inc](pc_port/src/cleanroom/field_script_move_absolute_xz.inc) | `func_80099980` | `6fd3ca7b8e96cf8f357156dd0a79f1ab145778d460322642fd83994f03b1d5ba` |
| [field_script_move_retained.inc](pc_port/src/cleanroom/field_script_move_retained.inc) | `func_80098C3C` | `63436cb49be0a8ee3221ec03f0d1229bb99b1d08c5764b2d647ee3901b52f38f` |
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
| [field_swap_render_context.inc](pc_port/src/cleanroom/field_swap_render_context.inc) | `FieldSwapRenderContext` | `5197988435536018fb798be941b96644e30defc089b810fbce4cae72f1d887c7` |
| [field_text_box_has_visible_portrait.inc](pc_port/src/cleanroom/field_text_box_has_visible_portrait.inc) | `func_8009C538` | `8ef9817be1b6ee0311cae12d3e7b3bdc31c1ea46bcc7eb465ad368231b834827` |
| [field_zoom_fade_colors.inc](pc_port/src/cleanroom/field_zoom_fade_colors.inc) | `func_800A5600` | `2c217c0da4815524faf3f75484fb863680151e672631bb727f873e71d5b4d051` |
| [menu_gfx_environment.inc](pc_port/src/cleanroom/menu_gfx_environment.inc) | `MenuInitializeGfxEnvironment`, `MenuInitializeGfxEnvironments` | `d07d4157202d023387ac1425643297d2229c5156b666dc54ca9ba6a2de41a70e` |
| [party_skin_initializers.inc](pc_port/src/cleanroom/party_skin_initializers.inc) | `GamePartyCharactersInitializeSkins`, `GamePartyGearsInitializeSkins` | `2a7f2310844b3f5dd7eee9ff810f844d6950e5d3e40b940170cb679a65025769` |
| [party_stream_sync.inc](pc_port/src/cleanroom/party_stream_sync.inc) | `GamePartySyncStreamedData` | `d01704ed86f76cb900821b479a5c668b3a3578b37e7e96cc98a6e7a83546acc9` |
| [scenario_flags.inc](pc_port/src/cleanroom/scenario_flags.inc) | `FieldScriptCheckScenarioFlagsEqual`, `FieldScriptCheckScenarioFlagsGreaterThan`, `FieldScriptCheckScenarioFlagsLessThan` | `3c35b5e34107715a0e2f33819518e67a55ab68bbcc58c0c8ee5fe9b1b4617140` |
| [sound_apply_volume_settings.inc](pc_port/src/cleanroom/sound_apply_volume_settings.inc) | `SoundApplyVolumeSettings` | `3c05681d568315a9d3b6e6132311286f98db1b32853f2e52701abf2f27abda5c` |
| [sound_calculate_audio_manager_size.inc](pc_port/src/cleanroom/sound_calculate_audio_manager_size.inc) | `SoundCalculateAudioManagerSize` | `2c0ffc6ab1c37c4588c8131ad814f9c5ae9ad331d6c30648c00f1e16ef7e55f4` |
| [sound_manager_init_cd.inc](pc_port/src/cleanroom/sound_manager_init_cd.inc) | `SoundSetCdAttr`, `SoundInitializeAudioManager` | `6774b6f2dd93ea37d8a939e06eb30a2b159a4c641b5430bcdccadfafd32dc168` |
| [sound_reverb_controls.inc](pc_port/src/cleanroom/sound_reverb_controls.inc) | `SoundInitiateReverbWorkAreaTransfer`, `SoundSetReverbModeWithAllocation` | `166a1d10d4f9d439089d78f6779bbdb87085f7027ab7ffd489e8f1ccc505e1d6` |
| [sound_script_active_flag800.inc](pc_port/src/cleanroom/sound_script_active_flag800.inc) | `SoundScriptClearActiveFlag800`, `SoundScriptSetActiveFlag800` | `04b3ba4e368046682e66cae639bb35ce67a3e932741565ac1078e40899ef511f` |
| [sound_script_note_parameters.inc](pc_port/src/cleanroom/sound_script_note_parameters.inc) | `SoundScriptFermata`, `SoundScriptSetAttackTime`, `SoundScriptSetOctave`, `SoundScriptSetSustain` | `67289b2ae988ea3e482f72e436c6d3372c920e0869307dd25fe125778a381e8a` |
| [sound_script_pitch_controls.inc](pc_port/src/cleanroom/sound_script_pitch_controls.inc) | `SoundScriptAddUnk6E`, `SoundScriptLowerOctave`, `SoundScriptRaiseOctave`, `SoundScriptSetUnk74` | `ad51c3747692a1335499baf454b810a7e7c26b88ba11c3ab7a7f59a7d2b23e47` |
| [sound_spu_memory_get_free_block.inc](pc_port/src/cleanroom/sound_spu_memory_get_free_block.inc) | `SoundSpuMemoryGetFreeBlock` | `a2bcd3dae53a73413f39849347c4729b795a8c679829cee014e70266d7b08a7c` |
| [sound_spu_memory_initialize.inc](pc_port/src/cleanroom/sound_spu_memory_initialize.inc) | `SoundSpuMemoryInitialize` | `5ee5fb4f443c78398f0e2ebcf4d0c1dbfabae142b194b2b4f4920410fddeafda` |
| [sound_transfer_controls.inc](pc_port/src/cleanroom/sound_transfer_controls.inc) | `SoundQueueSpuWriteCommand`, `SoundTransferQueueSync`, `SoundWdsSetTransferParamters` | `a2a79d14183e1510ee3306fbc01bd3c9ac1c887398d2a502a0cd66b0b5406007` |
| [sound_voice_channel_lifecycle.inc](pc_port/src/cleanroom/sound_voice_channel_lifecycle.inc) | `SoundAssignVoiceToChannelAndPlay`, `SoundAssignVoiceToChannelAndStop`, `SoundReleaseVoiceFromChannel`, `SoundStopVoiceOnChannel` | `374ff896f7b760ce0283b4a3fd76bff7fa5d3db092bdbb9827b4930e1e2975fc` |
| [text_box_lifecycle.inc](pc_port/src/cleanroom/text_box_lifecycle.inc) | `func_8007FFE8`, `func_800805F4` | `14208792ae3b43ba143fd811b898908ace25e56406572afe60322eb4670622e8` |
| [text_box_windows.inc](pc_port/src/cleanroom/text_box_windows.inc) | `func_8007E114`, `func_800807B4` | `1a15fbee1f895cc8e70dd05a62a7a8b905ccb531e3b432d00a5862b049e49f5d` |

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
