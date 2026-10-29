The production sprite profile is intentionally empty pending verified runtime
traces. The sixteen authored Fei pose rectangles are source art, and do not
establish any runtime animation, actor, frame, UV, map or part association.

Art label: fan-made, AI-generated replacement sprite art created for this project; not from the game; characters © Square Enix

`pc_port/tools/compile_hd2d_parts.py manifest.json pc_port/include_shim/hd2d_sprite_profile.h`
compiles a reviewed manifest into the immutable runtime profile. Production raw
RGBA belongs under `pc_port/assets/hd2d/`. Master enable and the independently
approved opening map list remain controlled by the integration policy.

The JSON object has exactly these fields: `schema` (integer 1), `label` (the exact
label above), `width`, `height`, `rgba_path`, and `parts`. Each part has exactly:

- `identity`: FNV-1a identity defined by `hd2d_sprite.h`, unsigned 32-bit.
- `map`, `actor`, `frame`, `part`: exact traced map, actor index, frame cursor and
  retail link-order part index. Negative/unknown identities cannot be mapped.
- `tpage`, `clut`: exact original unsigned 16-bit packet values.
- `u`, `v`: four original retail byte coordinates in FT4 vertex order.
- `rect`: authored atlas pixel `[x, y, width, height]` for this individual part.
- `trace`: nonempty reference to the reviewed trace evidence supporting this row.

A trace reference is an audit trail, not automatic evidence verification. The
compiler rejects ambiguity and missing fields and never infers mappings.

The sprite transport relinks the complete original FT4 at its original OT slot.
Only the packet-aware renderer hook supplies a replacement texture/UV rectangle;
screen corners, anchors, colours, tpage, CLUT, blend flags and packet order stay
retail. Registration uses the packet pointer plus a complete post-link snapshot,
so another packet with identical UVs does not gain the replacement. Unmapped,
changed, exhausted, missing-file and invalid-profile paths render retail.

The owner calls `PcPort_Hd2dSpritePresentDone` at both present paths; it retires
consumed entries only because field constructs the next OT before presenting the
previous frame. `PcPort_Hd2dSpriteGpuWalkDone`, supplied by the packet lifetime
patch, retires entries seen during the actual completed OT traversal. Unrelated
OT walks preserve unseen entries. Enable-state transitions explicitly call
`PcPort_Hd2dSpriteReset`, which clears all CPU registrations without GL calls. Texture upload restores current-unit
texture binding, unpack-buffer binding, alignment, row length and skip state;
uses nearest/clamp without mipmaps; and preserves the existing Hook3 shader's
alpha-under-0.5 discard and retail modulation/ABR behavior. The area texture hook
ABI remains unchanged; active DR texture bindings still bypass both hooks.
