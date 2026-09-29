# Xenogears

This wiki is the guide to the decompilation and the PC port in [Blizz127/xenogears-decomp-ai](https://github.com/Blizz127/xenogears-decomp-ai).

The game is Xenogears for the PlayStation, US release SLUS-00664. The code on `main` is two trees:

- `src/` is the decompilation. The matching build can still include retail assembly where a function is not plain C yet.
- `pc_port/` is the native port. It compiles that C for x86 and talks to the machine through PsyCross.

Maintained by [Blizz127](https://github.com/Blizz127). The original starter project is [ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp):

> This project is an in-progress matching decompilation of Xenogears for Playstation 1. The project currently targets the US release (SLUS 006.64), with the intention to target other releases as well down the road.

The splat layout and the US-disc target come from that starter. The matching work and the port after the fork are this repository.

## Pages

| Page | What it covers |
|---|---|
| [Decompile status](Decompile-Status) | Function counts, matched C, coexistence, what "done" means |
| [Port status](Port-Status) | What the native build links, what it skips, what still has no C body |
| [Current status](Current-Status) | Field routes that have been exercised, and older milestone notes |
| [Build and run](Build-and-Run) | How to build the port |
| [Decomp and port coexistence](Port-Coexistence-Architecture) | `SKIP_ASM`, overrides, and why two definitions of one symbol fail the link |
| [Matching and porting rules](Matching-and-Porting-Rules) | When a change belongs in `src/` and when it belongs in `pc_port/` |
| [Field control and movement](Field-Control-and-Movement) | Walkmesh, collision, player control |
| [Field rendering and camera](Field-Rendering-and-Camera) | Camera, framing, visibility |
| [Field script VM and opcodes](Field-Script-VM-and-Opcodes) | VM behavior |
| [Known addresses and globals](Known-Addresses-and-Globals) | Addresses used by the field and boot code |
| [Debugging and tracing](Debugging-and-Tracing) | gdb and logging |
| [Useful commands](Useful-Commands) | Copy-paste commands |

The same completion and port tables are on the [repository README](https://github.com/Blizz127/xenogears-decomp-ai#completion).
