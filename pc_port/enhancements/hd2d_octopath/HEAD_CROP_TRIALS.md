The current frame-190 part-5 head crop fails the strict horizontal test by five
calibrated pixels. No threshold, silhouette failure or production profile is
changed by these trials. Original PNG/RGBA and retail packets remain unchanged.

Baseline triple 1206/1214/1222 has retail mask `[300,182,339,288]` and HD mask
`[299,182,339,288]`. The b84 runtime full-alpha bbox `[146,85,173,135]` is calibrated
against the HD visible bbox, producing `(sx,sy,ox,oy)=(1.5185185,2.14,77.2962963,.1)`.
This maps the part-5 quad to `[303.5556,182,321.7778,224.8]`, whose retail/HD mask
bounds are `[309,182,321,203]` and `[304,182,321,203]`.

The direct 640×480 projection for the native 320×224 viewport maps its original
quad `[149,85,161,105]` to `[298,182.1429,322,225]`. Its retail/HD visible bounds
are `[309,182,321,203]` and `[299,182,321,203]`: the actual left hair extends ten
capture pixels too far left. The calibration compresses this discrepancy, so
both actual coordinates and the unchanged strict calibrated results matter.

Read-only alpha inspection predicts the same baseline left edge (~299.23).
Including the existing transparent left padding of the unchanged first atlas
pose can bring the sampled head outline rightward within the identical quad.
Predetermined padded variants have approximate unoccluded left edges 308.56–
311.57 against retail 309. The y+20/y+40 variants also sample lower head/face
pixels above the easel; neither face visibility nor strict agreement is claimed
before real renders. All trial source rectangles are within first-pose tile
x<313/y<348, before any other authored pose. They include original transparent
padding outside the content bbox; no pixels are fabricated, cropped in the
exported raster, stretched in the raster, or repainted.

Root can compile `tests/hd2d_head_crop_probe.c` with the existing port source
flags, add that object to the test link and add
`-Wl,--wrap=PcPort_Hd2dSpriteSubmitPart` alongside the footprint wrappers. Enable
`XENO_HD2D_HEAD_CROP_VARIANTS=1`, and set
`XENO_HD2D_HEAD_CROP_VARIANT_LOG=<trial-run>/head-variants.log`.
Only the exact verified frame-190/actor-1/part-5 baseline mapping is tuned.
The wrapper changes a local mapping copy before calling the real transport;
quad/OT/colour/ABR/profile/source fields are preserved. Eight variants cycle
once per eight-present retail/HD/hidden triple. At least 96 captures provide
repeated variant cycles after loading transients.

For exact head-part consumption evidence, the existing footprint GPU wrapper
can declare `void PcPort_Hd2dHeadCropGpuObserved(const void*,int,unsigned);`
and call it after the real texture hook returns:
`PcPort_Hd2dHeadCropGpuObserved(packet,r,r ? *tex : 0);`.
The helper counts only a real nonzero-texture acceptance at the exact registered
head packet address, once, and writes the associated variant separately.

`tests/hd2d_head_crop_report.py RUN --out NEW_REPORT_DIR` groups each variant's
capture triples and runs the unchanged b84 analyzer with strict horizontal
and tolerance one. Source captures are symlinked without pixel changes. The
report lists whole-sprite GPU acceptances and exact head-part consumptions.
A passing trial is a candidate only: root must select/review a source rectangle,
compile a native production profile and rerun without UV trial wrappers before
claiming a production PASS. Source pixel hashes must remain unchanged.
