# Source from a run_*_retail_test.sh.  Builds the harness pieces for one mode:
#   hx_build_misc <mode> <out.o>   : the (possibly spliced) menu TU, -fPIC,
#       every symbol weakened so the test's recording stubs and defsym'd
#       globals win (intra-TU calls go through the PLT under -fPIC).
#   hx_link <mode> <exe> <objs...> : link -no-pie with --gc-sections.
# Globals are bound to retail addresses with HX_DEFSYMS=(name=0xADDR ...).
HX_COMMON=(-std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
  -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/tests/lib -Isrc/menu/main
  -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
hx_mode_flags() {
  case "$1" in
    O0) echo "-O0" ;;
    O2) echo "-O2" ;;
    UBSan) echo "-O2 -fsanitize=undefined -fno-sanitize-recover=all" ;;
  esac
}
hx_build_misc() {
  local mode=$1 out=$2 src=${3:-${MENU_SRC:-src/menu/main/misc.c}}
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -fPIC -fpermissive -w -c "$src" -o "$out.strong.o"
  objcopy --weaken "$out.strong.o" "$out"
}
hx_link() {
  local mode=$1 exe=$2; shift 2
  local defs=()
  for d in "${HX_DEFSYMS[@]}"; do defs+=("-Wl,--defsym,$d"); done
  gcc -no-pie $(hx_mode_flags "$mode") -Wl,--gc-sections "$@" "${defs[@]}" -o "$exe"
}
