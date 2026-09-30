# Files that include upstream work

These files include work from [ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp) by its contributors. That project has not published a license, so all rights in that work remain with its authors. It is included here with credit, is not covered by this repository's MIT license, and is being re-derived independently; each file leaves this list when its upstream-authored parts are replaced.

147 files are listed. Every other file in this repository is original work under the MIT license in [LICENSE](LICENSE), except the third-party components named in [CREDITS.md](CREDITS.md).

## How this list was made

From the git history of the source branch (`claude/r6-launcher-fmv` at `23250136`), a file is listed when either:

- an upstream contributor appears in the file's history (`git log --follow --format=%an`), or
- `git blame -w -M -C` attributes at least one of its non-blank lines to an upstream contributor. This also catches upstream code that was moved or copied into a new file.

The "Upstream lines" column is that blame count at the snapshot, 16,828 lines in all. A 0 means the file's history has upstream commits but blame gives no current line to an upstream author. Those files are listed anyway, to be safe. The owner's commits (`blizz`, `Blizz127`) count as original work. Author names are the ones recorded in git.

## Decompiled game code (`src/`)

| File | Upstream authors | Upstream lines |
|---|---|--:|
| `src/battle/mainc122.c` | ladysilverberg | 4 |
| `src/field/camera/camera_movement.c` | ladysilverberg, Akshay Gopinath, Nora Strong | 365 |
| `src/field/dialogue/text_box.c` | ladysilverberg, Nora Strong | 155 |
| `src/field/dialogue/text_box_render.c` | ladysilverberg, Nora Strong | 165 |
| `src/field/effects/distortion.c` | ladysilverberg, Nora Strong | 25 |
| `src/field/effects/fade.c` | ladysilverberg, Nora Strong | 94 |
| `src/field/effects/fade_render.c` | ladysilverberg, Nora Strong | 58 |
| `src/field/effects/particles.c` | ladysilverberg, Nora Strong | 496 |
| `src/field/game_logic/gold.c` | ladysilverberg, Nora Strong | 40 |
| `src/field/game_logic/scenario_flags.c` | ladysilverberg, Nora Strong | 51 |
| `src/field/main/init.c` | ladysilverberg | 42 |
| `src/field/main/input_script_handlers.c` | ladysilverberg, Nora Strong | 30 |
| `src/field/main/main.c` | ladysilverberg | 35 |
| `src/field/main/misc.c` | ladysilverberg, Aku Kotkavuo, Valentin Robert | 652 |
| `src/field/main/misc10.c` | ladysilverberg, Nora Strong | 25 |
| `src/field/main/misc11.c` | ladysilverberg, Akshay Gopinath, Nora Strong | 677 |
| `src/field/main/misc2.c` | ladysilverberg | 187 |
| `src/field/main/misc3.c` | ladysilverberg | 50 |
| `src/field/main/misc4.c` | ladysilverberg | 270 |
| `src/field/main/misc5.c` | ladysilverberg, Nora Strong | 204 |
| `src/field/main/misc6.c` | ladysilverberg, Akshay Gopinath, Nora Strong | 546 |
| `src/field/main/misc7.c` | ladysilverberg, Nora Strong | 419 |
| `src/field/main/misc8.c` | ladysilverberg, Nora Strong | 90 |
| `src/field/main/misc9.c` | ladysilverberg, Nora Strong | 163 |
| `src/field/party/stats.c` | ladysilverberg, Akshay Gopinath, Nora Strong | 147 |
| `src/field/scripts/variable_handlers.c` | ladysilverberg, Nora Strong | 223 |
| `src/field/scripts/virtual_machine.c` | ladysilverberg, Nora Strong | 341 |
| `src/shop_menu/main/misc.c` | ladysilverberg, Valentin Robert | 232 |
| `src/shop_menu/main/misc2.c` | ladysilverberg, Valentin Robert | 1,664 |
| `src/shop_menu/main/misc3.c` | ladysilverberg | 94 |
| `src/shop_menu/main/misc4.c` | ladysilverberg | 92 |
| `src/shop_menu/main/misc5.c` | ladysilverberg, Valentin Robert | 137 |
| `src/shop_menu/main/misc6.c` | ladysilverberg | 246 |
| `src/shop_menu/main/misc7.c` | ladysilverberg | 79 |
| `src/shop_menu/main/misc8.c` | ladysilverberg | 360 |
| `src/shop_menu/main/misc9.c` | ladysilverberg | 216 |
| `src/slus_006.64/graphics/line_scroll.c` | ladysilverberg, Nora Strong | 64 |
| `src/slus_006.64/main/main.c` | ladysilverberg | 4 |
| `src/slus_006.64/system/archive.c` | ladysilverberg, Aku Kotkavuo, devperson7, Nora Strong | 503 |
| `src/slus_006.64/system/asset_loader.c` | ladysilverberg | 2 |
| `src/slus_006.64/system/controller.c` | ladysilverberg | 216 |
| `src/slus_006.64/system/font.c` | ladysilverberg | 384 |
| `src/slus_006.64/system/graphics.c` | ladysilverberg | 27 |
| `src/slus_006.64/system/heap_debug.c` | ladysilverberg, Nora Strong | 16 |
| `src/slus_006.64/system/libarchive.c` | ladysilverberg, devperson7 | 370 |
| `src/slus_006.64/system/menu.c` | ladysilverberg, Nora Strong, Valentin Robert | 117 |
| `src/slus_006.64/system/rendering.c` | ladysilverberg | 33 |
| `src/slus_006.64/system/system.c` | ladysilverberg | 82 |
| `src/slus_006.64/system/temp1.c` | ladysilverberg | 37 |
| `src/slus_006.64/system/temp1b.c` | ladysilverberg | 8 |
| `src/slus_006.64/system/temp1e.c` | ladysilverberg | 13 |
| `src/slus_006.64/system/temp2.c` | ladysilverberg | 23 |
| `src/slus_006.64/system/temp3.c` | ladysilverberg | 239 |
| `src/slus_006.64/system/work_list.c` | ladysilverberg, Nora Strong | 263 |

## Headers (`include/`)

| File | Upstream authors | Upstream lines |
|---|---|--:|
| `include/common.h` | ladysilverberg, Jonathan Peros, Nora Strong | 16 |
| `include/field/actor.h` | ladysilverberg, Akshay Gopinath, Aku Kotkavuo, Nora Strong | 372 |
| `include/field/camera.h` | ladysilverberg, Nora Strong | 19 |
| `include/field/effects.h` | ladysilverberg, Nora Strong | 47 |
| `include/field/graphics.h` | ladysilverberg, Nora Strong | 21 |
| `include/field/main.h` | ladysilverberg, Nora Strong | 96 |
| `include/field/particles.h` | ladysilverberg, Nora Strong | 98 |
| `include/field/script_vm.h` | ladysilverberg, Nora Strong | 35 |
| `include/field/text_box.h` | ladysilverberg, Nora Strong | 72 |
| `include/gte.inc` | ladysilverberg, Nora Strong | 67 |
| `include/include_asm.h` | ladysilverberg, Nora Strong | 14 |
| `include/macro.inc` | ladysilverberg, Nora Strong | 8 |
| `include/main/game.h` | ladysilverberg, Nora Strong, Valentin Robert | 81 |
| `include/main/main.h` | ladysilverberg, Nora Strong | 17 |
| `include/system/archive.h` | ladysilverberg, Nora Strong | 79 |
| `include/system/controller.h` | ladysilverberg, Nora Strong | 160 |
| `include/system/debug.h` | ladysilverberg, Nora Strong | 10 |
| `include/system/font.h` | ladysilverberg, Nora Strong | 60 |
| `include/system/graphics.h` | ladysilverberg, Nora Strong | 15 |
| `include/system/kernel.h` | ladysilverberg, Nora Strong | 24 |
| `include/system/math.h` | ladysilverberg, Nora Strong | 16 |
| `include/system/memory.h` | ladysilverberg, Nora Strong | 127 |
| `include/system/menu.h` | ladysilverberg, Nora Strong, Valentin Robert | 403 |
| `include/system/menu_resources.h` | ladysilverberg, Nora Strong | 45 |
| `include/system/sound.h` | ladysilverberg, GigaGoji, Jonathan Peros, Nora Strong | 235 |
| `include/types.h` | ladysilverberg, Jonathan Peros, Nora Strong | 33 |

## Configuration (`config/`)

| File | Upstream authors | Upstream lines |
|---|---|--:|
| `config/battle.yaml` | ladysilverberg | 19 |
| `config/battle_command_file1.yaml` | ladysilverberg | 8 |
| `config/battling.yaml` | ladysilverberg | 19 |
| `config/checksum.sha` | ladysilverberg, Nora Strong | 4 |
| `config/field.yaml` | ladysilverberg, Nora Strong | 59 |
| `config/member_change_menu.yaml` | ladysilverberg, Nora Strong | 50 |
| `config/menu.yaml` | ladysilverberg, Nora Strong | 0 |
| `config/movie.yaml` | ladysilverberg, Nora Strong | 46 |
| `config/movie_player.yaml` | ladysilverberg, Nora Strong | 0 |
| `config/overlay-template.yaml` | Nora Strong | 78 |
| `config/shop_menu.yaml` | ladysilverberg, Nora Strong | 49 |
| `config/slus_006.64.yaml` | ladysilverberg, devperson7, Jonathan Peros | 181 |
| `config/symbol_addrs.field.txt` | ladysilverberg, Aku Kotkavuo, Nora Strong, Valentin Robert | 297 |
| `config/symbol_addrs.member_change_menu.txt` | ladysilverberg, Nora Strong | 58 |
| `config/symbol_addrs.shop_menu.txt` | ladysilverberg, Nora Strong, Valentin Robert | 88 |
| `config/symbol_addrs.slus_006.64.txt` | ladysilverberg, devperson7, GigaGoji, James Brown, Jonathan Peros, Nora Strong, Valentin Robert | 939 |
| `config/world_map.yaml` | ladysilverberg | 19 |

## Build files and repository settings

| File | Upstream authors | Upstream lines |
|---|---|--:|
| `.gitattributes` | Nora Strong | 1 |
| `.gitignore` | ladysilverberg, Nora Strong | 15 |
| `.gitmodules` | ladysilverberg | 3 |
| `Dockerfile` | Nora Strong | 7 |
| `Makefile` | ladysilverberg, devperson7, James Brown, Jonathan Peros, Nora Strong | 82 |
| `disc/.gitkeep` | ladysilverberg | 0 |
| `gears.toml` | ladysilverberg, Nora Strong | 67 |
| `requirements.txt` | ladysilverberg, Nora Strong | 5 |

## Tools (`tools/`)

| File | Upstream authors | Upstream lines |
|---|---|--:|
| `tools/gears/.gitignore` | ladysilverberg | 1 |
| `tools/gears/Cargo.lock` | ladysilverberg | 292 |
| `tools/gears/Cargo.toml` | ladysilverberg | 13 |
| `tools/gears/src/file_system.rs` | ladysilverberg, Nora Strong | 42 |
| `tools/gears/src/main.rs` | ladysilverberg, Nora Strong | 329 |
| `tools/gears/src/presets.rs` | ladysilverberg, Nora Strong | 124 |
| `tools/get_yaml_target.py` | ladysilverberg, Nora Strong | 23 |
| `tools/objdiff/config.yaml` | ladysilverberg, Nora Strong | 27 |
| `tools/objdiff/objdiff_generate.py` | ladysilverberg, devperson7, Nora Strong | 85 |
| `tools/remu/tests/diff_800BEE2C.c` | ladysilverberg | 1 |
| `tools/scripts/cdrom/__init__.py` | ladysilverberg, Nora Strong | 0 |
| `tools/scripts/cdrom/cdxa.py` | Kat, Nora Strong | 43 |
| `tools/scripts/cdrom/fs.py` | Kat, Nora Strong | 114 |
| `tools/scripts/extract_overlays.py` | ladysilverberg, Kat, Nora Strong | 38 |
| `tools/scripts/ghidra_import_splat_symbols.py` | ladysilverberg, Kat, Nora Strong | 45 |
| `tools/scripts/overlays.yaml` | Nora Strong | 100 |
| `tools/scripts/psx/__init__.py` | ladysilverberg, Nora Strong | 0 |
| `tools/scripts/psx/lzss.py` | Kat, Nora Strong | 27 |
| `tools/scripts/psx/overlay.py` | Kat, Nora Strong | 70 |
| `tools/splat_ext/scommon.py` | ladysilverberg, Nora Strong | 15 |

## Port (`pc_port/`)

| File | Upstream authors | Upstream lines |
|---|---|--:|
| `pc_port/src/archive_port.c` | ladysilverberg | 24 |
| `pc_port/src/boot_sound_commit.c` | Jonathan Peros | 1 |
| `pc_port/src/fei_hd2d.c` | ladysilverberg | 2 |
| `pc_port/src/field_object_overlay.c` | ladysilverberg | 2 |
| `pc_port/src/field_pos_diag.c` | ladysilverberg | 3 |
| `pc_port/src/field_warp_diag.c` | ladysilverberg | 3 |
| `pc_port/src/game_overrides.c` | ladysilverberg | 2 |
| `pc_port/src/retail_leaf_adapters.c` | ladysilverberg | 3 |
| `pc_port/src/walkmesh_dump.c` | ladysilverberg | 3 |
| `pc_port/src/work_list_port.c` | ladysilverberg | 24 |
| `pc_port/src/world_map_init.c` | ladysilverberg | 3 |
| `pc_port/tests/change_field_handler_prod_test.c` | ladysilverberg | 1 |
| `pc_port/tests/field_gear_board_retail_test.c` | ladysilverberg | 3 |
| `pc_port/tests/field_gear_dispatch_test.c` | ladysilverberg | 3 |
| `pc_port/tests/field_pos_diag_lifecycle_test.c` | ladysilverberg | 3 |
| `pc_port/tests/field_script_vm_primitive_offset_test.c` | ladysilverberg | 1 |
| `pc_port/tests/field_sprite_bind_a06e8_retail_test.c` | ladysilverberg | 3 |
| `pc_port/tests/libarchive_file_integration_test.c` | ladysilverberg | 3 |
| `pc_port/tests/member_change_name_retail_test.c` | ladysilverberg | 2 |
| `pc_port/tests/menu_d8ea4_retail_test.c` | ladysilverberg | 3 |
| `pc_port/tests/portrait_render_regression_test.c` | ladysilverberg | 4 |
| `pc_port/tests/textbox_timing_prod_test.c` | ladysilverberg | 4 |
