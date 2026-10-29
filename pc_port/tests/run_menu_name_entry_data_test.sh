#!/usr/bin/env bash
# Name entry (menu mode 3) per-entry data refill (pc_port/src/data_name_entry_menu.c)
# against the user's disc/name_entry_menu.bin (archive dir 0x10 file 8), at
# O0 / O2 / UBSan; then deliberate mutants of the loader must be rejected.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
test "$(sha256sum disc/name_entry_menu.bin | cut -d' ' -f1)" = b0c6226511fdf0d3b874cf5e8658ed1fe19fd06833c0e0d61c4762aeb6a6f1b0
OUT=pc_port/build_native/menu_name_entry_data
mkdir -p "$OUT"
BASE=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
build() { # mode, loader source, exe
  local flags=(-"$1"); if [[ "$1" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
  gcc "${BASE[@]}" "${flags[@]}" -Wall -Werror -c "$2" -o "$3.loader.o"
  gcc "${BASE[@]}" "${flags[@]}" -Wall -Werror -c pc_port/tests/menu_name_entry_data_test.c -o "$3.test.o"
  gcc -no-pie "${flags[@]}" "$3.loader.o" "$3.test.o" -o "$3"
}
for mode in O0 O2 UBSan; do
  build "$mode" pc_port/src/data_name_entry_menu.c "$OUT/$mode"
  "$OUT/$mode" | tee "$OUT/$mode.log"
  grep -q '^NAME ENTRY DATA PASS' "$OUT/$mode.log"
done
mutate() { # name, unique text in the loader, replacement
  python3 - pc_port/src/data_name_entry_menu.c "$OUT/$1.c" "$2" "$3" <<'PY'
import sys, pathlib
s = pathlib.Path(sys.argv[1]).read_text(); a, b = sys.argv[3], sys.argv[4]
assert s.count(a) == 1, a
pathlib.Path(sys.argv[2]).write_text(s.replace(a, b))
PY
  build O2 "$OUT/$1.c" "$OUT/$1"
  if "$OUT/$1" > "$OUT/$1.log" 2>&1 || ! grep -q '^FAIL name entry data' "$OUT/$1.log"; then
    echo "MUTANT SURVIVED: $1"; exit 1; fi
  echo "MUTANT REJECTED: $1 ($(grep -m1 '^FAIL' "$OUT/$1.log" | cut -c1-80))"
}
mutate letters-addr '{NameEntryMenu_D_801CBEC0, 0x801CBEC0u,' '{NameEntryMenu_D_801CBEC0, 0x801CBEC4u,'
mutate drop-region '    {NameEntryMenu_D_801CBF98, 0x801CBF98u, sizeof(NameEntryMenu_D_801CBF98)},
' ''
mutate save-id-addr '{NameEntryMenu_D_801C5000, 0x801C5000u,' '{NameEntryMenu_D_801C5000, 0x801C5010u,'
mutate wrong-file '#define NAME_ENTRY_FILE 8 /* menu mode 3 + 5 */' '#define NAME_ENTRY_FILE 9 /* menu mode 3 + 5 */'
mutate load-once '    int size;
    u8* pImage;
' '    int size;
    u8* pImage;
    static int loaded;
    if (loaded++) return;
'
mutate no-free '    HeapFree(pImage);
}' '}'
echo 'NAME ENTRY DATA negative controls PASS count=6'
