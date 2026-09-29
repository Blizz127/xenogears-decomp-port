# Jiskan 16 KROM fallback

`pc_port/src/krom_jiskan16.inc` contains the 94 by 94 JIS X 0213 Plane 1
glyph grid from Jiskan 16 in the 16 by 16, MSB-first bitmap layout expected by
the port's KROM font callers. The source file is `jiskan16-1.hex`, available at
<https://unifoundry.com/japanese/jiskan16-1.hex>.

The upstream font page identifies these glyphs as **Public Domain** and gives
SHA-256 `cb177cddeb3cf73bd3e62dc93a7db1ad8c162256a50b93ecf7fc3bd641ea26d1`.
No Sony BIOS glyphs or game data are included. The font is a visual fallback;
glyph pixels can differ from Sony's KROM. To regenerate the C include:

Two punctuation cells (JIS `2169` and `224F`) are emitted blank because their
public-domain bitmap bytes coincided with 32-byte runs in the locally checked
BIOS ROM and were rejected by the strict retail-data guard. This is a small
additional pixel divergence that keeps the fallback source independent.

```sh
python3 pc_port/tools/gen_jiskan16.py pc_port/font_data/jiskan16-1.hex \
  pc_port/src/krom_jiskan16.inc
```

The converter validates the source checksum and emits zero-filled entries for
unassigned JIS cells.
