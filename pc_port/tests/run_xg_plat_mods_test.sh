#!/usr/bin/env bash
# xg_plat mods layer: manifests, load order, hooks, plugin.so, asset replacement.
set -euo pipefail
cd "$(dirname "$0")/../.."
base="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/xg_plat_mods_test"
rm -rf "${base:?}"
mods="$base/mods"
mkdir -p "$mods/a_first/assets" "$mods/b_last/assets" "$mods/c_mid" "$mods/d_bad"
printf 'name = First\nload_order = 1\n' > "$mods/a_first/mod.txt"
printf 'name = Last\nload_order = 9\n' > "$mods/b_last/mod.txt"
printf 'name = Mid\nload_order = 5\n' > "$mods/c_mid/mod.txt"
printf 'version = 1\n' > "$mods/d_bad/mod.txt"
hA=$(head -c 32 /dev/zero | tr '\0' 'A' | sha256sum | cut -d' ' -f1)
hB=$(head -c 32 /dev/zero | tr '\0' 'B' | sha256sum | cut -d' ' -f1)
printf 'first' > "$mods/a_first/assets/$hA.bin"      # overridden by Last
printf 'HELLO' > "$mods/b_last/assets/$hA.bin"
head -c 48 /dev/zero > "$mods/b_last/assets/$hB.bin"  # too big for a 40-byte buffer
gcc -std=gnu17 -O2 -Wall -Wextra -Werror -shared -fPIC \
    pc_port/tests/xg_plat_mods_test_plugin.c -o "$mods/b_last/plugin.so"
gcc -std=gnu17 -O2 -Wall -Wextra -Werror -Wno-unused-function \
    pc_port/tests/xg_plat_mods_test.c pc_port/src/plat/xg_plat_mods.c pc_port/src/mod_events.c \
    pc_port/src/plat/xg_plat_config.c -ldl -o "$base/test"
"$base/test" "$mods"
