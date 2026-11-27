# Sound event reservation tests

Run `./pc_port/tests/native_sound_continuation/run-reservation.sh` on Linux x86_64 with the project's prepared PsyCross renderer, SDL2 development package, GCC and G++. The fixture uses an SDL worker thread and synthetic events. It checks event ownership, recursion, identity, callback re-entry, abort cleanup and lock release.

The same runner checks the test channel projection for native pointers, KSEG0 and KSEG1 aliases, subword access, and rejection of invalid writes. A synthetic heap and linked manager list exercise membership, cycles, count and size inconsistencies under a real SDL event reservation. The non-PIE fixture maps its test arena into the 32-bit address range used by the projection.

The earlier six-case continuation comparison used test-local instruction translations and was retired. This runner reports reservation and memory-domain results. Live sound sequencing, callback integration, and playback require separate tests.
