#!/usr/bin/env bash
# Regression pin: g_PartyStreamDataQueue must hold the 6 entries
# GamePartyStreamLoadSkinData (src/slus_006.64/system/temp3.c) can queue:
# 3 party skins + archive 0xA7 + 0xA8 + the terminator.  Its port storage is a
# generated stub sized from config/symbol_addrs.slus_006.64.txt; a 0x20 size
# (4 entries) let the terminator/extra entries overrun into
# g_SoundAudioManagerListHead and crash the sound interrupt (func_8003C020).
#
# Checks the symbol_addrs size (retail entries are 8 bytes) and, when a linked
# pc_port/build_native/xeno-port exists, the stub's real size (16-byte host
# entries).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
NEED=6
fail=0

size=$(sed -n 's/^g_PartyStreamDataQueue = 0x800625A4;.*size:\(0x[0-9A-Fa-f]*\).*/\1/p' \
    config/symbol_addrs.slus_006.64.txt)
if [ -z "$size" ]; then
    echo "FAIL: g_PartyStreamDataQueue size annotation not found"; exit 1
fi
if [ $((size)) -lt $((NEED * 8)) ]; then
    echo "FAIL: symbol_addrs size $size < $NEED retail entries ($((NEED * 8)) bytes)"; fail=1
else
    echo "PASS: symbol_addrs size $size holds $NEED retail entries"
fi

BIN="${XENO_PORT_BINARY:-pc_port/build_native/xeno-port}"
if [ -x "$BIN" ] && command -v nm >/dev/null; then
    hex=$(nm -S "$BIN" | awk '$4 == "g_PartyStreamDataQueue" { print $2 }')
    if [ -z "$hex" ]; then
        echo "FAIL: g_PartyStreamDataQueue not in $BIN"; fail=1
    elif [ $((16#$hex)) -lt $((NEED * 16)) ]; then
        echo "FAIL: port stub is 0x$hex bytes < $NEED host entries"; fail=1
    else
        echo "PASS: port stub 0x$hex bytes holds $NEED host entries"
    fi
else
    echo "SKIP: no linked $BIN (symbol_addrs check only)"
fi
exit $fail
