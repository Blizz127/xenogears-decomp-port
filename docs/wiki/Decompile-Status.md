# Decompile status

US retail is SLUS-00664. Counts are from `python3 tools/scripts/decomp_status.py` against `src/` and a local `asm/<overlay>/{matchings,nonmatchings}` split. That split is generated and is not committed.

**Matched C** is a C body with no `INCLUDE_ASM`. **Coexistence** is retail assembly kept in the matching build. **Unported** would be a bare `INCLUDE_ASM` with no port body. The in-repo gate `tools/scripts/test_decomp_complete.py` pins a universe of 3295. The split used for this page counted 3294. Matched C was not re-checked with objdiff for this write-up. The project rule is that a plain C body is only committed when it is the finished function. Near-matches stay on `INCLUDE_ASM`.

| | Count |
|---|--:|
| Retail functions in the split | 3294 |
| Matched C bodies | 2557 (77.6%) |
| Coexistence | 737 (22.4%) |
| Unported, by that script | 0 |
| Done for the matching build | 3294 (100%) |
| C files under `src/` | 275 |
| Lines in `src/**/*.c` | 91,590 |

| Overlay | Functions | Matched C | Coexistence | Unported | Matched C share | Done |
|---|--:|--:|--:|--:|--:|--:|
| battle | 814 | 291 | 523 | 0 | 35.7% | 100% |
| field | 878 | 878 | 0 | 0 | 100% | 100% |
| member_change_menu | 67 | 67 | 0 | 0 | 100% | 100% |
| menu | 312 | 173 | 139 | 0 | 55.4% | 100% |
| shop_menu | 120 | 106 | 14 | 0 | 88.3% | 100% |
| slus_006.64 | 1103 | 1042 | 61 | 0 | 94.5% | 100% |
| **Total** | **3294** | **2557** | **737** | **0** | **77.6%** | **100%** |

`battle_command_file1`, `battling`, `movie`, and `world_map` have configs and a few controller or data units. They are not rows in the function table.

"Done for the matching build" includes coexistence. The retail bytes are still linked from assembly. Field and the member-change menu are the overlays that are entirely plain C.

A direct read of `INCLUDE_ASM` in `src/` is stricter than the dashboard, because the dashboard treats an `INCLUDE_ASM` inside `#ifndef XENO_PC_PORT` as coexistence even when the port branch has no function body:

| | Count |
|---|--:|
| `INCLUDE_ASM` with a port C body in the same file | 129 |
| `INCLUDE_ASM` omitted when the port compiles the file | 608 |
| of which battle | 506 |
| `INCLUDE_ASM` in `src/battling` (unconditional) | 169 |

The starter project's chart remains [decomp.dev/ladysilverberg/xenogears-decomp](https://decomp.dev/ladysilverberg/xenogears-decomp). It describes [ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp), not this fork.
