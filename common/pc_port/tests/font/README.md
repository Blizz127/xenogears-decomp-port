# Standalone font checks

Run `sh run.sh` from this directory with a C11 compiler that supports GNU `may_alias` and UndefinedBehaviorSanitizer. Set `CC=gcc` or `CC=clang` to select the compiler. The runner checks unoptimized, optimized and sanitizer builds.

The fixture exercises all 26 font providers with generated colors, glyph widths, palettes, packets and host service callbacks. It checks table selection, packet contents, text formatting and resource ownership. The draw tests vary the queue length and mutate the selected font during callbacks. The structure in `fixture.h` models the fields used by these tests and is local to this standalone target.
