# Source branch after v0.7.82-source

## Fonts, heap services, sequencer controls and field variables

This update adds 137 independently authored host functions and 17 standalone synthetic test suites. The shared source tree now contains 282 entry points.

- Font services position text, choose glyph sizes and palettes, write sprite packets and link them for drawing. Diagnostics forward native variadic arguments on Windows and Linux.
- Heap services maintain compact allocation headers, split and consolidate space, manage delayed frees and resolve diagnostic symbols. The standalone test maps a small arena below 4 GiB to exercise the original 32-bit next-block representation.
- Sound services change sequencer settings, manage voices, apply envelope and mixer controls, queue transfers and coordinate IRQ allocation. Their fixtures provide the audio services needed to exercise ordering and state changes.
- Field scripts perform arithmetic, bit operations, shifts and random variable assignment. The fixtures cover signed values, cursor wrap and operand services that change the active actor.

The private Linux and Windows builds linked these entry points from the files listed in the shared source catalog. The tests in this tree run without retail software or a BIOS.

## Party recovery and bitwise script commands

This source update adds ten functions in three files, bringing the public
total to 145. The privately linked builds have 365 upstream matches on
Linux and 364 on Windows.

- Field scripts can restore HP or EP across all eleven characters,
  including those outside the active party. A separate command restores
  both values for a single character. Three adjustment commands apply a
  signed amount to occupied party slots selected by the script’s mask.
- Field scripts can assign a value to a variable or combine it with
  another value using AND, OR or XOR. All four commands write a 16-bit
  result and advance the active script cursor by six. Assignment retains
  its first destination. The three bitwise operations read the destination
  again after evaluating the right operand.

Three standalone tests cover these commands using synthetic actors, party
records and script services.

## Party HP and EP, field variables and audio services

This update adds 16 shared C functions in five source files. The reviewed
branch contained 135 named functions at that checkpoint. The private linked audit found
375 remaining upstream matches on Linux and 374 on Windows. The tagged
release remains `v0.7.82-source`.

- Four field functions adjust current HP or EP (called MP in the C
  interfaces). Increases store a 16-bit sum and then check it against
  the roster member’s maximum. A decrease that produces zero or a
  negative signed result stores one. Four field-script commands move
  HP and EP between party members and script variables. They advance
  the script cursor by four, including when the party slot is empty.
- Four other commands set a variable to true or false, increment it or
  decrement it. The tests check the script reads, variable writes and
  cursor advancement.
- The sound code initializes the audio manager, applies CD reverb and
  mix attributes, and manages the reverb work area and its transfer.
  On Windows the port enters the audio critical section before touching
  shared CD attributes and leaves it after submitting the update. Four new
  standalone test suites cover party vitals, unary variables and audio.
  The simulated Windows CD test checks the critical-section boundaries.

## Field transfers, party positions, input and argument readers

The preceding branch update added 17 functions across five fragments and
five standalone test suites. The tagged v0.7.82-source archive contains
the previous 102 functions.

- Field rendering restores a 256 × 32 strip in VRAM. With the operation
  enabled, the host disables dithering in the two draw contexts, reads
  back the strip, sets the color-mask bits on nonzero pixels and uploads
  the result. The disabled path leaves both contexts untouched.
- The party-position routine reads three variables for a valid party
  slot, in order, into the supplied output locations. An invalid slot
  leaves all three outputs untouched.
- Field input scripts compare either an exact controller state or
  overlapping bits. A match advances the script cursor by five bytes.
  A miss reads a branch destination from the script. Two handlers select
  which controller state to test.
- Eight field-script argument readers use separate flag bits to
  distinguish signed 16-bit literals from variable references.
- Sound transfer controls submit a write request to the existing queue,
  set the destination and remaining byte count, and report when the
  queue has at least six entries pending.

The complete game still requires sources under provenance review. The
public release remains source-only.

# November 25, 2026 · v0.7.82-source

This checkpoint adds 16 independently written native C functions across
six fragments. They implement field archive requests, the color update
during a field fade, sound sequencer commands, and voice-channel ownership
and playback. Six standalone field and sound regression tests accompany the new code.

On private Linux and Windows builds, Fei and Bart loaded File 06 in
Stalactite Cave, opened and closed the party menu, and walked again.
During each of 32 fade calls on each platform, the original Disc 1
routine produced the same 400 output bytes from that platform's input.
The original and port playthroughs used opposite render buffers and
had different polygon positions. Other tested routes include Bart’s
rescue of Margie and sound playback in the Bledavik inn.

The independent replacement counts are 84 of 134 common functions and
18 of 93 sound functions. The wider audit of 1,128 rows still finds
407 linked upstream matches on Linux and 406 on Windows. These totals
include two functions per platform from upstream commented references.
Public game executables remain pending provenance clearance and the
native execution gate. The production game still includes a PS1 CPU
interpreter. A private decoder-free build won the solo Ramsus battle.
The following scripted Gear sequence still crashes on the tested
corrected Linux build.

Owner device checks for controller input, borderless fullscreen and
the automatic on-screen keyboard are each **NOT RUN** for this release.

# November 25, 2026 · v0.7.81-source

This release adds independently written host C to the reviewed r7.21 source
snapshot. Recent additions resolve field-script actor selectors, check whether
an actor is on screen, and initialize field sprite and draw-mode packets. The
packet initializer ran on room entry during Bart's rescue of Margie. It matched
the original on all 320 packet bytes and all 35 graphics-service calls in each
measured initialization.

The private Linux r7.79 client loaded an ordinary memory-card save in Margie's
chamber. Bart won the first Ramsus fight, received 500 G and rejoined Fei.
The client crashed at the start of the second scripted battle. Play beyond
that point remains unverified on that build. The original reference checks
cover the field initializer and rescue sequence. The battle observation comes
from the private client. Production executables retain a PS1 CPU interpreter.
A separate decoder-free build completed the rescue slice.

The Linux r7.81 and Windows r7.80 candidates at development revision
`552d9417d37875b11462d8b55375d95da05fb44d` contain 82 independent
replacements in the original 134-function cohort and four in the 93-function
sound cohort. Across 1,128 frozen ledger rows, the linked-code audit still
identifies **423 matches on Linux and 422 on Windows**, including two compiled
functions on each platform that match upstream commented reference bodies.
These are measured lower bounds. Public game executables await independent
replacements or redistribution clearance and the native execution gate.

Handheld controller, borderless fullscreen and automatic on-screen keyboard
checks are all **NOT RUN** for this source release.

# r7.21 source checkpoint

This snapshot includes 36 files for Linux and 50 for Windows.
Each platform directory has its own file manifest and credits. The
ownership review omitted original Sony SDK files, translations of the
matching SDK, retail assets and upstream game implementations with
unresolved redistribution rights. The snapshot cannot rebuild the complete
executables. A linked-code audit identified at least 134 whole functions
from 27 unlicensed upstream files in both executables tested below.
Public game binaries are pending replacement or clearance of that code.

## Changes since the earlier public port snapshot

- The Windows x64 port builds an AMD64 executable with SDL2 and OpenAL.
  Its file and memory interfaces adapt the original 32-bit game conventions
  to Windows x64. Title-screen Continue now reads the complete disc number
  and can open an original US File 02 memory card.
- Battle dispatch checks the call target before reading experimental host
  settings. In the tested Windows Stalactite Cave Gear encounter, the
  first action menu opened 9.4 seconds after formation. Input reduced an
  enemy from 138 to 122 HP. The party won, collected its rewards and
  returned to field control.
- Field and checkpoint changes preserve party placement, pending music
  and camera state for the tested routes. The graphics changes include
  dialogue sprite sampling and framebuffer blending. The earlier HD-2D
  experiments were removed. The tested builds use the original presentation.
- The disc preparation utility uses Python's standard library, checks the
  supplied US Disc 1 against its reference digest and extracts the required
  files. Linux Python and Windows x64 Python under Wine produced identical
  files. The utility ships without game data.

## Gameplay evidence and limits

The Windows game under Wine loaded File 02 with Fei, Bart and 1,630G and
played the Lahan guard and Alice's house routes. All 40 wedding dialogue
cues advanced. The first 39 matched the original cue ID, speaker and box
slot. Both lines on the last page matched the original glyphs. Fei returned
downstairs at the original coordinates and could walk. The installed
Linux game reloaded the Windows house save and its four sidecars in a
fresh process. Their hashes were preserved.

Both production builds retain the PS1 CPU interpreter. The observed
Windows wedding and Gear victory runs recorded zero interpreted
instructions over those routes. Through the November 21 matching
checkpoint, the PSX build reproduced 12 reference binaries using C and
remaining assembly. A separate object comparison found matches for
3,972 of 4,583 functions across 358 units. These numbers describe the PSX
reconstruction. Whole-game native execution and full-campaign parity
remain open.

Bal's optional Gear Shop reports that Parts are unavailable and returns
to the field. Some world-map states, sound-bank transitions and movie
timing remain incomplete. Disc 2 gameplay, physical Windows hardware
and the handheld controller, borderless and on-screen keyboard checks
remain unverified.
