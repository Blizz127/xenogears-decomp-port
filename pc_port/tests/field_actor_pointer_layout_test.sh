#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_root"

# These handlers operate on ActorData through FieldActor's +0x4C PSX pointer
# slot. Keep the audit focused: the old code used a host void** and a 0x7C
# actor stride, either of which selects the wrong actor as soon as index > 0.
for function in func_8009749C func_800989F0 func_8009A1E4 func_8009AA00 func_8009B210; do
    body=$(awk -v name="$function" '
        $0 ~ "^[[:space:]]*void " name "\\(" { in_fn=1 }
        in_fn { print }
        in_fn && /^}/ { exit }
    ' src/field/main/misc7.c)
    test -n "$body"
    if grep -Eq '\* *0x7C|void[[:space:]]*\*\*' <<<"$body"; then
        echo "stale actor layout access remains in $function" >&2
        exit 1
    fi
    grep -q 'pActorData' <<<"$body"
done

body=$(awk '
    $0 ~ "^[[:space:]]*s32 func_80077E10\\(" { in_fn=1 }
    in_fn { print }
    in_fn && /^}/ { exit }
' src/field/main/main.c)
test -n "$body"
if grep -Eq '\* *0x7C|void[[:space:]]*\*\*' <<<"$body"; then
    echo "stale actor layout access remains in func_80077E10" >&2
    exit 1
fi
grep -q 'pActorData' <<<"$body"

# Compile-time layout assertions protect the 32-bit PSX pointer-slot contract
# on the 64-bit host. In particular, actor index 1 must advance by 0x5C,
# while pActorData must remain at byte offset 0x4C.
layout_test="${TMPDIR:-/tmp}/field_actor_pointer_layout_test.$$"
cc -std=gnu17 -fpermissive -DXENO_PC_PORT -D_LANGUAGE_C -w \
    -Ipc_port/include_shim -Iinclude \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -include common.h -x c -o "$layout_test" - <<'EOF'
#include <stddef.h>
#include "field/actor.h"

_Static_assert(sizeof(FieldActor) == 0x5C, "FieldActor stride changed");
_Static_assert(offsetof(FieldActor, pActorData) == 0x4C,
               "pActorData slot moved");
_Static_assert(offsetof(FieldActor, pModelData) == 0x00,
               "pModelData slot moved");

int main(void) {
    FieldActor actors[2] = {0};
    return ((char *)&actors[1] - (char *)&actors[0]) == 0x5C ? 0 : 1;
}
EOF

"$layout_test"
rm -f "$layout_test"
echo "field actor pointer/layout regression: PASS"
