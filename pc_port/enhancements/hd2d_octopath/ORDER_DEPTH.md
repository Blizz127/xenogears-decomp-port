The composition backend supports two explicit depth modes. Existing synthetic
positive GL_R32F view-distance fixtures remain in `XG_HD2D_DEPTH_LINEAR` mode,
with their original physical focus/fog parameters. Runtime OT-order buffers use
`XG_HD2D_DEPTH_OT_ORDER` with GL_R16 raw unsigned slot codes, sampled as normalized
R16 and multiplied by `65535 / ot_max_value`. Zero slot is invalid/background;
larger nonzero slots are nearer. The shader converts valid ordering to ordinal
distance `1 - clamp(order,0,1)`, including valid nearest distance zero.

OT order is a painter-order proxy. It does not establish physical camera depth,
world units, actor feet or a physically accurate focal plane. Separate normalized
`order_focus`, `order_range`, `order_tilt`, `order_fog_start` and `order_fog_end`
parameters control the proxy. Defaults are focus .5, range .2, fog .3 through .9.
Particles currently carry physical positive view distance, so OT-order mode
explicitly disables their composition until a producer supplies a consistent
projected-depth interpretation. Shadow/point-light items still require verified
projected inputs; the buffer callback does not fabricate those items.

Legacy binary GL_R8 protection remains supported. Explicit coverage-bit mode
decodes the R8 byte: sprite 1, map 2, UI 4 and override 8. Only the exact byte 2
certifies editable map-only coverage. Zero, unknown bits and mixed coverage
protect pixels from every effect, including lighting, grade and vignette.
This also protects retained colour when its coverage is zero or unknown;
an incorrectly retained map-only tag still requires a producer-side fix.
Protected neighbours are
excluded from blur/glow samples so their crisp texture cannot bleed into the
environment. All effects fail closed without certified protection.

Both auxiliary dimensions zero retain the existing viewport-sized attachment
contract. Full-drawable attachments specify both positive dimensions and use
the viewport x/y as an exact texel offset. Scene colour is captured only from the
viewport. Auxiliary depth/coverage use texelFetch and nearest sampler state;
no interpolation of coverage bytes or ordinal slots occurs. Out-of-viewport
pixels remain untouched, and the backend restores all GL state it changes.

The GL regression preserves the R32F fixtures and verifies low bit coverage,
all-effect crisp protection, raw maximum-slot/zero-slot distinction, ordinal
fog direction, viewport-offset sampling and state restoration. Those fixtures
establish backend behavior; integrated producer/current-frame evidence remains
necessary before claiming runtime depth or coverage correctness.
