This draft maps one observed retail sprite frame to the unchanged original
`front_idle` authored pose. It covers one observed frame; native strict footprint validation passed for this static camera. The easel occludes most of
Fei in the full retail screenshot, so the visible front-facing head supports
this pose choice but cannot establish a visible full-body/foot comparison.

fan-made, AI-generated replacement sprite art created for this project; not from the game; characters © Square Enix

The runtime association is exactly field map 14, player actor 1, identity
FNV-1a `db7d0105`, raw frame cursor 190, parts 0 through 5, tpage 26 and CLUT
14353 with each part's traced retail UVs. No animation interpretation is claimed.
Every other map, actor, identity, frame and part remains unmapped and falls back.

The sample is TRACE present 840, lines 3103–3113 in the private verified trace.
All six trace records report `packet_exact=1`. Their complete screen union is
native-coordinate `[146,85,174,135]`, width 28 and height 50. The full capture is
640×480, so native coordinates display at twice those values. Part 4 reaches the
lowest retail y=135; it maps to authored source y=236 through exclusive y=338.
The author's `front_idle` cell is `[x=81,y=37,width=181,height=301]`, with exclusive
bounds `[81,37,262,338]`, confirmed against the original alpha content bounds.

Each part's screen bounds map into this complete authored content rectangle by
its fraction of the complete retail union. Integer source edges use exact
rational nearest rounding, ties upward; shared screen edges produce identical
source edges. The first five rectangles stay within the authored content cell. Part5 was
calibrated separately to [0,77,200,120], including the original first-pose
transparent left padding and face; all rectangles remain inside the atlas. Retail screen positions, anchors, colour modulation, ABR and OT slots
are untouched. Missing authored pixels outside the retail part coverage can
still alter the visible silhouette; unchanged packet geometry alone does not
prove silhouette equivalence. Other observations of the same raw frame have
27-pixel unions due to projected rounding; this static association uses the
inspected present-840 union and does not resize those other retail quads.

`field14-frame190.provisional.json` is the strict runtime manifest.
`field14-frame190.provisional.audit.json` records each original quad, assigned
source rectangle, trace lines and hashes. `hd2d_sprite_profile.field14_frame190.draft.h`
records the original draft. The live `hd2d_sprite_profile.h` is generated from
the calibrated manifest with `compile_hd2d_parts.py`. Eight test-only crop
variants were measured on actual GPU draws. Variant5 passed strict tol1 with
zero measured part and whole-silhouette edge differences in four triples and
55 identical occluded rows. The rebuilt production profile passed without the crop-variant wrapper:
six comparable triples, zero measured per-part and whole-silhouette edge
difference, all55 occluded rows matching, 288 actual GPU acceptances and1112
changed Fei pixels. Full field OFF identity is a separate pending gate.

Production `pc_port/assets/hd2d/fei-octopath-v2.rgba` is the lossless, tightly packed
1254×1254 RGBA decode of the existing original PNG: 6,290,064 bytes, SHA-256
`988583de635568b36c47882d3873e8e84a37e8ec025eb02159738c2f5e21a56e`.
No resampling, repainting, alpha changes, cropping of the exported asset, new
poses or modifications to the sixteen authored cells were performed.
