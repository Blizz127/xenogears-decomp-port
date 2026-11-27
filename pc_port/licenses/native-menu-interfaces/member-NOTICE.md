# Native member-change menu

This source assembles the native party-selection menu. The main source and its two adjacent interface headers were written using the public project API and the ABI recorded by Linux and Windows compilers. The entry-point declarations use the parameter and return types from the linked service definitions. External game variables provide the existing party state, input and menu resources.

Place the three files in `src/member_change_menu/main/` within the host source tree. The source includes the project menu definitions and its clean renderer interface. It expects the selected menu implementations at `pc_port/src/cleanroom/`. The port's MIT license accompanies the assembly.

Blizz127 and contributors published 29 of the linked function providers plus the shared palette helper under MIT in `Blizz127/xenogears-decomp-port` at `0fba9c73eb2a2c6bc7408568b2deb13fb16b7f19`. The release lists the files and their digests in `common/README.md`. The other eight linked provider files require the separately reviewed project sources recorded in the private verification materials. Preserve the notices on each accepted source when assembling a distribution.

The reviewed controller decoder uses four button codes absent from the earlier public menu header. The codes were independently observed at the named input fields in the Linux and Windows objects compiled from the reviewed MIT controller provider. The declaration for Windows memory clearing uses the type of the Windows port's linked compatibility service.

Linux GCC and Windows Clang checked the layouts of 23 public menu and game structures, including all 412 of their fields. The tests checked field types, sizes and offsets on each target and checked the 20 external variable declarations. The four file-local read-only objects found in the earlier linked menu objects come from three reviewed source files. They require no initializers in the assembly.
