# Credits

## The Xenogears decompilation

This port is built on top of the Xenogears matching decompilation started by
**ladysilverberg**: [ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp).
That project set up the splat layout, the SLUS-00664 target and the build,
and its contributors decompiled the first functions the port runs.

Contributors to the upstream decompilation, from its git history:

- ladysilverberg
- Kat
- Jonathan Peros (jperos, jdperos)
- Nora Strong
- devperson7
- Valentin Robert
- GigaGoji
- Aku Kotkavuo
- James Brown
- Akshay Gopinath

The upstream project has not published a license, so none of its code is
included or relicensed here. Its work remains its authors'.

The upstream project also thanks the
[Silent Hill decompilation](https://github.com/Vatuu/silent-hill-decomp) as
the project-structure template it was adapted from.

## Port, tools and documentation

Written by [Blizz127](https://github.com/Blizz127), with AI coding
assistants (Claude and Codex) under the owner's direction.

## Font

The port's Japanese glyph fallback (`pc_port/src/krom_jiskan16.inc`) is
generated from **Jiskan 16** (16×16 JIS X 0213:2000 Plane 1, version 1.06),
which is in the **public domain**. Font credits, from the BDF file:

- based on jiskan16-1990.bdf and jisksp16-1990.bdf;
- new characters merged by Toshiyuki Imamura and HANATAKA Shinya;
- with thanks to Taichi Kawabata, Koichi Yasuoka, TOYOSHIMA Masayuki,
  Kazuo Koike and SATO Yasunao.

The hex conversion we use comes from [Unifoundry](https://unifoundry.com/japanese/)
(`jiskan16-1.hex`, not redistributed here; see
[pc_port/font_data/README.md](pc_port/font_data/README.md)).

## Libraries used by the port

| Component | Use | License |
|---|---|---|
| [PsyCross](https://github.com/OpenDriver2/PsyCross) | PSX hardware and API layer (fetched at build time, patched by `pc_port/patches/`) | MIT |
| [SDL2](https://github.com/libsdl-org/SDL) | Window, input, audio device | zlib |
| [OpenAL Soft](https://github.com/kcat/openal-soft) | Audio | LGPL-2.0-or-later |
| [libsamplerate](https://github.com/libsndfile/libsamplerate) | Audio resampling | BSD-2-Clause |
| [OpenSSL](https://www.openssl.org/) (libcrypto) | SHA hashing of the user's files | Apache-2.0 |

Full license text for PsyCross is in [pc_port/THIRD_PARTY.md](pc_port/THIRD_PARTY.md).

## Tools

| Tool | Use | License |
|---|---|---|
| [splat](https://github.com/ethteck/splat) | Splitting the binaries | MIT |
| [spimdisasm](https://github.com/Decompollaborate/spimdisasm) | Disassembly | MIT |
| [maspsx](https://github.com/mkst/maspsx) | PSX assembler wrapper | MIT |
| [objdiff](https://github.com/encounter/objdiff) | Object diffing | Apache-2.0 |
| [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) | Matching search | MIT |
| [m2c](https://github.com/matt-kempster/m2c) | Initial C drafts | GPL-3.0 |
| GCC 2.6/2.7 MIPS cross compilers | Matching build (not included here) | GPL |
| [decomp.me](https://decomp.me/), [decomp.dev](https://decomp.dev/) | Collaboration and progress tracking | — |

None of these tools is redistributed in this repository.
