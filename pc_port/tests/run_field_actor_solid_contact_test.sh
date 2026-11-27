#!/usr/bin/env bash
set -euo pipefail
tests=$(cd "$(dirname "$0")" && pwd)
source_file=${1:-"$tests/../../src/field/main/misc8.c"}
out=$(mktemp -d "$tests/.xeno-field-solid.XXXXXXXX")
trap 'rm -rf -- "$out"' EXIT
python3 - "$source_file" "$out/contact.inc" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
signature = "void func_80084158(s32 actorIndex, void* pFieldActor, void* pActorData) {"
if source.count(signature) != 1:
    raise ValueError("expected exactly one field contact resolver definition")
start = source.index(signature)
end = source.index("\n}\n#endif /* XENO_PC_PORT */", start) + 2
Path(sys.argv[2]).write_text(source[start:end] + "\n")
PY
# The field record format stores 32-bit addresses. Static test objects in a
# non-PIE executable keep its real pointer reads intact on x86-64 Linux.
"${CC:-cc}" -std=c11 -O2 -fno-strict-aliasing -fno-pie -no-pie \
    -I "$out" "$tests/field_actor_solid_contact_test.c" -o "$out/check"
"$out/check"
