# Build the world map's matched C (src/world_map/main.c) the way the port
# compiles game TUs, for runners that exercise switched func_XXXXXXXX bodies
# instead of retired wm_XXXXXXXX ones.  Only the named functions (and what
# they reference) are kept, so the object links without the whole port:
#   wm_obj=$(world_map_matched_obj "$OUT" func_80080900 [func_...])
# The matched bodies read world-map data through
# pc_port/src/world_map_port_data.h (PSX_ADDR into g_PsxRam), so the test
# must define g_PsxRam.
world_map_matched_obj() {
    local out="$1" root keep=() sym
    shift
    root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
    mkdir -p "$out"
    for sym in "$@"; do keep+=(-u "$sym"); done
    "${CC_BIN:-${CC:-cc}}" -std=gnu17 -w -fpermissive -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -fno-builtin -O0 -g -ffunction-sections -fdata-sections \
        -I"$root/pc_port/include_shim" -I"$root/include" \
        -I"$root/pc_port/extern/PsyCross/include" -I"$root/pc_port/extern/PsyCross/include/psx" \
        -I"$root/pc_port/src" \
        -c "$root/src/world_map/main.c" -o "$out/world_map_matched_all.o" >&2
    ld -r --gc-sections "${keep[@]}" -o "$out/world_map_matched.o" "$out/world_map_matched_all.o" >&2
    echo "$out/world_map_matched.o"
}
