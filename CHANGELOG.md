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
