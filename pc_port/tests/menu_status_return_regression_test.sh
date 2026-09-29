#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
src="$repo_root/src/menu/main/misc.c"

body=$(awk '
    /^s32 func_801E2BE4\(void\)/ { in_fn=1 }
    in_fn { print }
    in_fn && /^}/ { exit }
' "$src")

test -n "$body"
grep -q 'func_801E2AE0();' <<<"$body"
grep -q 'return 1;' <<<"$body"

# Status cleanup frees the three work buffers allocated by func_801E2AE0.
# Keeping both halves paired prevents the generated-stub path from freeing
# uninitialized pointers after Status is selected.
cleanup=$(awk '
    /^void func_801E3088\(s32 arg0\)/ { in_fn=1 }
    in_fn { print }
    in_fn && /^}/ { exit }
' "$src")
grep -q 'case 6:' <<<"$cleanup"
grep -q 'func_801E2B80();' <<<"$cleanup"

echo 'menu Status safe-return regression: PASS'
