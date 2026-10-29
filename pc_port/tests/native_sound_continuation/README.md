# Experimental battle sound continuation

This test-only instruction transcription covers the continuation after the
DisableEvent call in retail `0x8003B644`, including voice assignment, release,
and instrument setup. It is not wired into production and must not be used
with the real sound event: the event call is stubbed, and safe locking,
callback order, pointer domains and failure cleanup are not established.

`./pc_port/tests/native_sound_continuation/run.sh` compares six synthetic
states with the local original CPU instructions, including a simulated LP64
channel table and the sequence of three sound-helper calls and EnableEvent.
A controlled clock invokes the real game-thread vblank service at selected
helper/event boundaries; one case mutates synthetic sound flags there. This
does not execute the full production bridge or real sound-event lock. It requires the existing local retail image, generated
`pc_port/build_native_only` headers and PsyCross sources. It does not add the
retail image to the repository. The comparison shares PsyCross GTE state and is
not an independent hardware, audio, timing, save or playable-slice test.
The test runs under a hard timeout and deletes its temporary build directory.

`run-reservation.sh` separately compiles the real PsyCross event registry with
the experimental reservation patch and checks a synthetic event and SDL
threads. A reservation is an extra raw mutex hold, accepted only for an
enabled sound tick with no existing caller hold. A per-slot generation check
detects close/reopen while held. It is not yet called from
battle: holding it during preflight can suppress ticks earlier than retail,
and callback re-entry, event mutations and failure cleanup remain unresolved.

The synthetic fixture also exercises abort from a setjmp/longjmp failure.
The experimental reserved bracket can finish normally or restore the prior
pending state on a clean abort; if the original slot was closed and reused,
abort does not modify the replacement. Abort releases the reservation and its own bracket hold, leaving callback-owned
holds intact. It reports whether the original event state was restored or a
callback changed it. Synthetic coverage includes callback slot reuse, nested
holds, and enable/disable calls that restore the apparent state while changing
event history. This does not roll back sound-data writes or integrate into battle.

The fixture also checks that the battle bus resolves the sound element count
to its native scalar, and that invalid zero, odd and out-of-range counts are
refused before the enabled sound wrapper enters the element service.
The element-span preflight checks pair bounds, guest RAM wrap and overlap with
the stack; the full sound-event preflight is still incomplete.

`run-combined-bracket.sh` compares six fixed synthetic continuation cases with the
retail CPU while both sides use the real patched PsyCross SDL event reservation
and bracket. A separate thread attempts a sound tick at each helper boundary;
ticks are suppressed while the bracket is held and one reaches the registered
callback after finish. The continuation's sound data, vblank clock and callback
mutation are fixtures. This does not validate production dispatch, real callback
timing, sound-data rollback, or justify removing the DisableEvent guard.

A separate failure case closes and reopens the event in a simulated vblank
callback during the continuation. Finish refuses and abort releases its own
reservation while preserving the replacement. The callback also changes a sound element, and that change remains after
refusal. Writes before refusal are not rolled back; this fixture does not make the production path safe to enable.

The combined fixture uses a test-only prepared index-zero instrument body and
prepared full-width LP64 slot assignment and release bodies; retail CPU uses
projected slot access. Fixed synthetic
cases compare registers, memory, slots and ordering with the real SDL event
bracket, including matching and mismatched releases, empty and occupied slots,
and signed-priority decisions. Out-of-range release and changed-event/slot
refusals and changed instrument-bank refusal are checked separately.
Preparation validates fixture spans before reservation, then captures selected slots under the raw SDL reservation and
checks them again before assignment. A failed capture releases the reservation.
The continuation body also uses pre-resolved fixture memory spans while the
bracket is held. An uncaptured access refuses without logging under the lock,
and the fixture then releases the reservation. This does not establish live
pointer lifetime, callback behavior or sound-data rollback. A synthetic guard
tracks the prepared manager, selected elements, stack frame, SED header,
offsets, event and sound-data words, and bank span. It refuses if a callback
changes those bytes even when event history remains unchanged. It cannot detect
changes outside the captured spans or a change restored before comparison;
earlier parent writes are not rolled back.
One fixed callback case changes and restores a captured word before the next
check and confirms that both the snapshot and event-history checks miss it.

The prepared helpers check thread-local bracket ownership, event generation and
mutation history after the fixture's synthetic callback. These checks cannot
detect arbitrary changes to sound data. A callback can leave earlier
continuation writes in place after a refusal; production-wide preflight,
rollback and timing remain unresolved.

A separate synthetic sound-heap membership capture validates a bounded header
chain and manager-list membership while a raw reservation is held. It rejects
invalid sizes, links, count mismatches and list cycles. This is a snapshot: it
does not prove callback stability, real allocation lifetime or rollback.
One controlled callback invokes the actual manager-removal leaf under the
reservation. The disabled-event membership recheck catches a remaining unlink,
but a second controlled case removes and restores the head before the recheck;
that restored change is not detected. Neither callback is claimed to occur in
the live game.

The combined bracket fixture routes its synthetic callback through the actual
port vblank service and `func_8003634C` controller callback slot. Controller
input and several controller helpers are stubbed. A counter confirms that the
controller path invokes the fixture callback while the sound event is reserved.
This establishes a possible re-entry route in the fixture; it does not establish
that a particular live game state registers a callback or that its memory
changes match the fixture.

The fixture also calls the actual host instrument, channel-assignment and
channel-release functions inside the reserved SDL bracket on synthetic data,
with a controlled controller callback between assignment and release. Separate
fixed retail-CPU comparisons cover those leaves. The port declares the voice
processing mask as a 32-bit word; a guard-page regression checks that native
voice operations do not access beyond it. No live sound lifetime or rollback
is proven.

A separate test-only guest-slot view and adapter resolve a KSEG0 guest voice to
host RAM before reservation, then compare fixed same, empty and priority cases
against a CPU oracle using that synthetic guest view. A changed slot refuses
before voice writes and restores the guest argument. This projection is not
used by production and does not establish representation consistency across
other paths, real manager lifetime, callbacks or rollback.
The fixture supplies the slot's guest segment as known provenance. A KSEG1
slot versus KSEG0 argument takes a different retail branch despite identical
host storage, and the test-only preflight refuses that mixed view. Production
currently has no verified provenance for making this admission decision.
