# Fake matches

These are semantic C reconstructions used by the PC port. They are not
byte-exact decompilation matches and are excluded from matched-C progress.
The retail build continues to use the pinned assembly for each function. Each
entry needs later review for a natural form that matches retail bytes.

| Function | Current form | Reason and revisit target |
|---|---|---|
| `func_80086700` | `/* FAKEMATCH */` under `XENO_PC_PORT` | Reconstructs the 80-record X/Z delta update with 27-bit wrapping. The available permuter candidates distort pointer/control flow; keep the clear semantic loop and later tune from the retail instruction order. |
| `func_8008C28C` | `/* FAKEMATCH */` under `XENO_PC_PORT` | Reconstructs channel-selected SpriteData creation, animation, scale, and flag clearing. A high-scoring permuter candidate incorrectly conditions initialization; later derive a retail-matching natural form. |

Do not include these functions in the headline matched-C percentage. A future
retail byte match may retire an entry only after `rom-check` and objdiff verify
the result.
