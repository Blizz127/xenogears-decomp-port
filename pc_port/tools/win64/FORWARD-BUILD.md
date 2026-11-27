# Building the Windows x64 game

The Windows build uses a Linux host with Python 3.11 or newer, Git, Clang and LLVM 21 with development headers, and Zig 0.13.0. Use the same clean source revision as the Linux build. Keep the unstripped Linux ELF and the generated files beside its `linux-link.json`. The Linux object ownership records determine the Windows weak declarations, battle namespace and bridge. Both platforms compile their own game objects.

Run the source inventory from the game checkout. It lists any missing committed source files. The preview selects 67 ordinary battle objects that provide 91 native leaf entries. Other ordinary battle code can execute through the guest fallback using the player's disc.

```sh
tools=pc_port/tools/win64
revision=$(git rev-parse HEAD)
python3 -B "$tools/forward_compile.py" inventory --source-root . --commit "$revision"
```

Set `RELEASE93_ZIP` to the published v0.7.93-preview Windows ZIP, `SDL2_CRATE` to the sdl2-sys 0.38.0 source crate, and `PSYCROSS_UPSTREAM` to a Git checkout of OpenDriver2/PsyCross containing commit `e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6`. Set `WORK` to a new local build directory. Keep your disc, split, BIOS and generated game sources local. The source restorer verifies the compressed renderer companion and all 74 restored file digests. The dependency restorer verifies the 54 Windows multimedia headers and the SDL2 and OpenAL DLLs and import libraries.

```sh
mkdir "$WORK"
unzip -p "$RELEASE93_ZIP" sources/renderer-presentation.tar.gz > "$WORK/renderer.tar.gz"
python3 -B pc_port/tools/prepare_renderer.py --platform windows \
    --source-bundle "$WORK/renderer.tar.gz" --upstream "$PSYCROSS_UPSTREAM" --out "$WORK/restored"
PREFIX="$WORK/multimedia"
python3 -B "$tools/restore_public_win_deps.py" "$RELEASE93_ZIP" "$SDL2_CRATE" "$PREFIX"
python3 -B "$tools/build_renderer.py" --source-root . --commit "$revision" \
    --restored "$WORK/restored" --prefix "$PREFIX" --out "$WORK/renderer-build" --jobs 4
SDK_ARCHIVE="$WORK/renderer-build/libpsycross.a"
SDK_PROOF="$WORK/renderer-build/sdk-receipt.json"
RENDERER="$WORK/renderer-build/inputs/pc_port/extern/PsyCross"
```

The producer verifies all 58 restored vendor files. It stages `libgpu.h`, `libspu.h` and `libcd.h` from `pc_port/renderer_headers/psx/` in the selected game commit, alongside the other verified inputs. Both Windows compilers use this effective renderer directory. Zig 0.13.0 compiles each of the 22 Windows objects. LLVM 21 removes CodeView records containing host paths while keeping DWARF. The producer verifies the archive member order, SHA-256 of every member and each compiler dependency. For a focused build, pass `--unit ABS.C.o` and choose another output directory. That mode emits an object and `sdk-subset.json`.

For a complete game build, set `LINUX_PROOF` to the new Linux link receipt and `ZIG_LIB` to the installed Zig 0.13.0 `lib` directory. Choose unused output directories for `OBJECTS` and `LINKED`.

```sh
python3 -B "$tools/forward_compile.py" compile --source-root . --commit "$revision" \
    --renderer-root "$RENDERER" --prefix "$PREFIX" --zig-lib "$ZIG_LIB" \
    --linux "$LINUX_PROOF" --out "$OBJECTS" --jobs 4
python3 -B "$tools/forward_link.py" --source-root . \
    --objects "$OBJECTS/windows-compile.json" --generated "$OBJECTS" \
    --linux "$LINUX_PROOF" --sdk "$SDK_ARCHIVE" --sdk-receipt "$SDK_PROOF" \
    --prefix "$PREFIX" --out "$LINKED"
```

The compile receipt records each object, source, compiler command and included header digest. The linker checks object order, DLL and import-library identities, renderer objects, the fixed PE image layout, bridge binding sizes and public export ordinals. `windows-link.json` identifies the stripped x64 game at `xeno-port-stripped.exe`. The debug PE and both link maps stay beside the receipt.

This profile builds 446 shared game objects and one Windows host resolver. It omits the optional Gear Shop and arena implementations. The five Gear Shop field entry points remain callable and report the shop as unavailable. The cinematic path advances past movies and omits the unused video transform. Windows preserves 6,151 export ordinals from v0.7.93-preview and leaves 2,889 slots vacant, including the 2,887 retired names.

For a quick compiler check, add `--unit field_party_gear.c.o` to the compile command. Repeat `--unit` to select more objects. A subset output is usable for object-level checks and is rejected by the game linker. Run the rule checks separately with `python3 -B "$tools/forward_link.py" --self-check`.
