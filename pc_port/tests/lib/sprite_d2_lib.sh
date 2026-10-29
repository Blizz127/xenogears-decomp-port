# shellcheck shell=bash
# Shared build/run/mutant helpers for the d2 sprite retail-differential tests.
# Callers set: BUILD_DIR, TEST_SRC (test .c), UUT_SRC (game TU), WEAKEN (array of
# --weaken-symbol=...), FAIL_RE (stderr/stdout regex that marks a rejected
# mutant) and then call d2_build_run / d2_mutant. Run from the repo root.
CC="${CC:-gcc}"
D2_EXTRA=()
SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"

d2_check_disc() {
    local actual
    actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
    if [[ "$actual" != "$SLUS_SHA" ]]; then
        echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
        exit 1
    fi
}

D2_BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
         -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
         -include assert.h -fpermissive -ffunction-sections -fdata-sections)
D2_INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
        -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

# d2_build_run name source flags... ; leaves stdout in $BUILD_DIR/name.stdout and
# returns the test's exit status (0 pass, nonzero diverge/fail).
d2_build_run() {
    local name="$1" source="$2" linker="$CC" rc=0
    shift 2
    "$CC" "${D2_BASE[@]}" "${D2_INC[@]}" -w -fno-inline -fno-builtin -fno-ipa-ra \
        -fno-sanitize=object-size "${D2_EXTRA[@]}" "$@" -c "$source" -o "$BUILD_DIR/$name.uut.o"
    objcopy "${WEAKEN[@]}" "$BUILD_DIR/$name.uut.o"
    "$CC" "${D2_BASE[@]}" "${D2_INC[@]}" -Wall -Wextra -Werror "${D2_EXTRA[@]}" "$@" \
        -c "$TEST_SRC" -o "$BUILD_DIR/$name.test.o"
    "$CC" "${D2_BASE[@]}" "${D2_INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    if [[ "$name" == UBSan* ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -no-pie "$@" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "$BUILD_DIR/$name.uut.o" ${EXTRA_OBJS:-} -Wl,--gc-sections -lm -o "$BUILD_DIR/$name"
    timeout "${D2_RUN_TIMEOUT:-30}" "$BUILD_DIR/$name" "${TEST_ARGS[@]}" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr" || rc=$?
    return $rc
}

# d2_regimes: all green required in O0, O2 and UBSan with identical stdout.
d2_regimes() {
    d2_build_run O0 "$UUT_SRC" -O0 -g || { cat "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O0.stderr" >&2; return 1; }
    d2_build_run O2 "$UUT_SRC" -O2 || { cat "$BUILD_DIR/O2.stdout" "$BUILD_DIR/O2.stderr" >&2; return 1; }
    d2_build_run UBSan "$UUT_SRC" -O1 -g -fsanitize=undefined ${D2_UBSAN_SHIFT--fno-sanitize=shift-base} -fno-sanitize-recover=all \
        || { cat "$BUILD_DIR/UBSan.stdout" "$BUILD_DIR/UBSan.stderr" >&2; return 1; }
    test ! -s "$BUILD_DIR/O0.stderr"
    cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
    cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
    cat "$BUILD_DIR/O0.stdout"
}

# d2_mutant name fn_regex sed_expr : mutate the function range [fn_regex..^}] of
# $UUT_SRC and require the test to go red.
d2_mutant() {
    local name="$1" fn="$2" expr="$3" rc=0
    sed "/$fn/,/^}/ { $expr; }" "$UUT_SRC" > "$BUILD_DIR/$name.c"
    if cmp -s "$UUT_SRC" "$BUILD_DIR/$name.c"; then
        echo "MUTANT NO-OP (sed did not change source): $name" >&2
        exit 1
    fi
    d2_build_run "$name" "$BUILD_DIR/$name.c" -O0 -g || rc=$?
    if [[ "$rc" -eq 0 ]]; then
        echo "MUTANT NOT DETECTED: $name" >&2
        exit 1
    fi
    echo "  mutant rejected: $name  ($(head -c 160 "$BUILD_DIR/$name.stdout" | head -1))"
}
