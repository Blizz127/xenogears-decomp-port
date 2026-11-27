# Native heap implementation

Install `src/slus_006.64/system/memory.c` with its adjacent `heap_native_abi.h` in the native PC source tree. The eleven included heap files come from `common/pc_port/src/cleanroom/` in Blizz127/xenogears-decomp-port at `0fba9c73eb2a2c6bc7408568b2deb13fb16b7f19`. Install those files at their corresponding `pc_port/src/cleanroom/` paths. Blizz127 and contributors published the implementations under MIT and recorded their function coverage and source hashes in `common/README.md`. Preserve the accompanying license.

The new C file binds the public implementations to native game storage. Its declarations use the types and offsets recorded in the Linux x64 and Windows x64 compiler objects. The delayed-release record occupies 24 bytes on both platforms. Its four-byte alignment is recorded directly in the Windows object. The Linux object records its field offsets and size.

The port supplies the owned-disc reader through `pc_port/src/retail_data.h`. Public v0.7.93-preview renderer source companions supply this header, its supporting platform headers and the host `psx/types.h` declarations. The reader fills 19 initially empty buffers from the player's US executable. Together these ranges cover 120 bytes. Only their destination names, sizes, file identity and file offsets are present in the new source.

The new `HeapGetNextBlockHeader` implements the public heap entry point using the observed 32-bit span calculation. Its Linux and Windows machine-code implementations each matched the released function for 2,076 synthetic heap cases. The Linux host executed the Windows code using the Microsoft x64 calling convention.

Linux GCC 15.2.0 and Windows-target Clang 21.1.8 accepted the complete translation unit and consumed the eleven reviewed providers. Both expanded constructor tables match the independently observed range roster in order. Whole-game linking and a player-visible gameplay check remain integration steps.
