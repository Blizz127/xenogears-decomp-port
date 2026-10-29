#!/usr/bin/env bash
set -euo pipefail
if [[ ${XENO_RESERVATION_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_RESERVATION_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../../.."
ulimit -c 0
ulimit -v 12582912
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-sound-reservation.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
mkdir -p "$OUT/src/psx"
cp pc_port/extern/PsyCross/src/psx/LIBAPI.C "$OUT/src/psx/LIBAPI.C"
if ! rg -q _xeno_sound_reservation "$OUT/src/psx/LIBAPI.C"; then
    patch -s -d "$OUT" -p1 < pc_port/patches/psycross_sound_reservation.patch
fi
if ! rg -q _xeno_sound_reservation_identity "$OUT/src/psx/LIBAPI.C"; then
    patch -s -d "$OUT" -p1 < pc_port/patches/psycross_sound_reservation_identity.patch
fi
if ! rg -q _xeno_sound_reservation_cleanup "$OUT/src/psx/LIBAPI.C"; then
    patch -s -d "$OUT" -p1 < pc_port/patches/psycross_sound_reservation_cleanup.patch
fi
if ! rg -q _xeno_sound_reservation_epoch "$OUT/src/psx/LIBAPI.C"; then
    patch -s -d "$OUT" -p1 < pc_port/patches/psycross_sound_reservation_epoch.patch
fi
if ! rg -q _xeno_sound_reservation_owned "$OUT/src/psx/LIBAPI.C"; then
    patch -s -d "$OUT" -p1 < pc_port/patches/psycross_sound_reservation_owned.patch
fi
if ! rg -q _xeno_sound_reservation_pristine "$OUT/src/psx/LIBAPI.C"; then
    patch -s -d "$OUT" -p1 < pc_port/patches/psycross_sound_reservation_pristine.patch
fi
read -r -a SDL_FLAGS <<< "$(pkg-config --cflags sdl2)"
read -r -a SDL_LIBS <<< "$(pkg-config --libs sdl2)"
INC=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/extern/PsyCross/src/psx)
g++ -std=c++17 -O1 -fpermissive -w -ffunction-sections -fdata-sections "${SDL_FLAGS[@]}" "${INC[@]}" -c "$OUT/src/psx/LIBAPI.C" -o "$OUT/vendor.o"
gcc -std=gnu17 -O1 "${SDL_FLAGS[@]}" "${INC[@]}" -c pc_port/tests/native_sound_continuation/reservation-test.c -o "$OUT/test.o"
g++ -Wl,--gc-sections "$OUT/vendor.o" "$OUT/test.o" "${SDL_LIBS[@]}" -o "$OUT/test"
"$OUT/test"
