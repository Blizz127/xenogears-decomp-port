#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

int PcPort_FieldTestMusicBootstrapAllowed(void);

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FIELD TEST MUSIC GUARD FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    unsetenv("XENO_FIELD_TEST");
    require(PcPort_FieldTestMusicBootstrapAllowed() == 0,
            "normal retail entry does not use the harness bootstrap");

    setenv("XENO_FIELD_TEST", "1", 1);
    require(PcPort_FieldTestMusicBootstrapAllowed() == 1,
            "the first direct field entry bootstraps common WDS/music");

    /* This models the next FieldMain invocation after a natural room/world
     * transition.  The retail script-owned music state must survive; the
     * direct-entry stand-in is process-once. */
    require(PcPort_FieldTestMusicBootstrapAllowed() == 0,
            "field re-entry does not overwrite retail music state");

    unsetenv("XENO_FIELD_TEST");
    require(PcPort_FieldTestMusicBootstrapAllowed() == 0,
            "disabling the harness cannot re-arm the bootstrap");

    puts("FIELD TEST MUSIC GUARD PASS");
    return EXIT_SUCCESS;
}
