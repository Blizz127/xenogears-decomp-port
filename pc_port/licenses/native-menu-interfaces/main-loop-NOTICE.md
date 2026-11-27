# Native game loop

The native game-loop source uses the project's public game-state declaration from `include/main/main.h` at `Blizz127/xenogears-decomp-port` commit `7c729166cbcf076cd5a30872a7022171fc7f6bbb`. The same public commit records the heap interface constants the game loop needs. Its native heap and game-service declarations were checked against the actual Linux x64 and Windows x64 definition objects.

The word-to-pointer adapter uses the project's publicly recorded RAM interface from commit `514ca2307c441f5aa078673c997ef9c6ac037c3d`. The replacement LZSS bounds checker and pointer adapter were authored using observed calls and differential tests. Supply `pc_port/src/psx_memory.h` from that public source, the adjacent native heap declaration header and the v0.7.93-preview renderer source headers when assembling the native game tree.

The six included game implementations cover state changes, overlay loading, dispatch, reset controls, splash-screen presentation and diagnostics. Their complete file digests match the original project writers' independently produced outputs and the source used by both f883 release builds. They supply eleven compiled functions, including three file-local helpers. The original project overlay decoder supplies three more functions through its bounded stream interface. The replacement reset routine follows the observed reset-service calls and callback arguments.

The overlay layout reader derives file sizes and compressed stream extents through the player's mounted-disc sector reader. All three active layout records returned by the released build were reproduced on an owned US Disc 1. The complete source supplies all 18 historical functions across 27 measured source and header inputs on each x64 target.

Preserve the accompanying MIT notice for Blizz127 and contributors, and the renderer's notices for its inputs. Source-level checks cover a single x64 translation unit. Integration into a complete game binary and a new player-visible playthrough are separate checks.
