# Third-party notices for the Windows port

PsyCross is the MIT-licensed PSX compatibility backend. This build uses
commit e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6 of
https://github.com/OpenDriver2/PsyCross and local port patches.
The full PsyCross license follows below. Its generated OpenGL loader uses
GLAD 0.1.36. See licenses/GLAD-0.1.36.txt for GLAD and Khronos terms.

The native opening movie decoder includes the public PSn00bSDK
variable-length-code table and standard MDEC quantization matrix. The
covered source as compiled, including its attribution, is distributed
under MPL-2.0 in licenses/PSn00bSDK alongside both notices and the license.
The font fallback uses Unifoundry’s public-domain Jiskan 16, from
https://unifoundry.com/japanese/jiskan16-1.hex.

The distribution includes replaceable SDL 2.26.4 and OpenAL Soft 1.25.1 DLLs.
The SDL source comes from the SDL directory in sdl2-sys 0.38.0.
licenses/ contains their original license notices and those of their
bundled components, the compiler and runtime. The source used to build the OpenAL DLL is in
sources/openal-soft-1.25.1-source.tar.bz2. The accompanying
OpenAL-SOURCE-NOTE.txt identifies the upstream archive and the one omitted
documentation graphic.
The two distributed DLLs have normalized development paths. Their machine
code is identical to the verified release build.

MIT License

Copyright (c) 2020 REDRIVER2 Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

The source companion in sources/renderer-presentation.tar.gz identifies the PsyCross renderer compiled for this game and the project changes that apply to the declared upstream revision.
