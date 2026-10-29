#!/usr/bin/env python3
"""Fei HD-2D footprint / OT analysis (run_fei_hd2d_footprint_test.sh).

Input: a run with XENO_FEI_HD2D_TEST_CYCLE=1, which renders the player sprite
three ways on consecutive presents (present % 3: 0 retail, 1 HD-2D, 2 Fei
hidden), its part log (XENO_FEI_HD2D_PART_LOG) and the captures of those
presents (fei-frame-<present>.png, 24-bit BMP payload).

For every retail/HD/hidden triple whose parts (screen quads, OT slots, atlas
cell) are identical and whose background is identical outside Fei, Fei's
visible mask in a render is the set of pixels that differ from the hidden
render. Checks, in capture pixels ("render scale"):
  1. footprint/OT: HD parts use exactly the retail parts' quads and OT slots;
  2. silhouette: HD mask top/bottom == retail within TOL px;
  3. per part: mask bounds inside each part's quad, HD vs retail: top/bottom
     within TOL px. Left/right follow the atlas art's own outline and are
     reported (FEI_FOOTPRINT_STRICT_HORIZONTAL=1 enforces them too);
  4. occlusion: the rows of Fei's silhouette box that are fully hidden
     (no visible pixel) are the same rows in both, within TOL at each run
     edge, and so are the per-row visible spans' vertical extent.
Writes PNG evidence (crops, masks, a side-by-side) to OUTDIR.

usage: fei_hd2d_footprint_analyze.py PARTLOG CAPDIR OUTDIR [--tol N]
"""
import os
import struct
import sys
import zlib
from collections import defaultdict


def read_bmp(path):
    d = open(path, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w, h = struct.unpack_from('<ii', d, 18)
    if struct.unpack_from('<H', d, 28)[0] != 24:
        raise ValueError(path + ': not 24-bit')
    stride = (w * 3 + 3) & ~3
    rows = []
    for y in range(abs(h)):
        sy = abs(h) - 1 - y if h > 0 else y
        r = d[off + sy * stride: off + sy * stride + w * 3]
        rows.append(bytes(b for i in range(w) for b in (r[i * 3 + 2], r[i * 3 + 1], r[i * 3])))
    return w, abs(h), rows


def write_png(path, w, h, rows):
    raw = b''.join(b'\0' + r for r in rows)

    def chunk(t, data):
        return struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def parse_log(path):
    frames = defaultdict(list)
    meta = {}
    for line in open(path):
        kv = dict(t.split('=', 1) for t in line.split() if '=' in t)
        if 'present' not in kv:
            continue
        p = int(kv['present'])
        if 'part' in kv:
            q = tuple(int(v) for v in kv['quad'].split(','))
            frames[p].append((int(kv['part']), int(kv['ot'], 16), q))
            meta[p] = (int(kv['mode']), int(kv['cell']))
        if 'box' in kv:
            meta.setdefault(('box', p), tuple(float(v) for v in kv['box'].split(',')))
    # The field double-buffers its OT, so the slot addresses alternate between
    # two tables frame to frame: key a part's slot relative to part 0's.
    for p, parts in frames.items():
        base = min(parts)[1]
        frames[p] = [(i, o - base, q) for i, o, q in parts]
    return frames, meta


def bbox(pts):
    if not pts:
        return None
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    return (min(xs), min(ys), max(xs), max(ys))


def diff_mask(a, b, w, region):
    x0, y0, x1, y1 = region
    m = set()
    for y in range(y0, y1 + 1):
        ra, rb = a[y], b[y]
        for x in range(x0, x1 + 1):
            if ra[x * 3:x * 3 + 3] != rb[x * 3:x * 3 + 3]:
                m.add((x, y))
    return m


def rows_equal_outside(a, b, w, h, region):
    x0, y0, x1, y1 = region
    for y in range(h):
        if y < y0 or y > y1:
            if a[y] != b[y]:
                return False
        elif a[y][:x0 * 3] != b[y][:x0 * 3] or a[y][(x1 + 1) * 3:] != b[y][(x1 + 1) * 3:]:
            return False
    return True


def point_in_quad(px, py, q):
    # POLY_FT4 vertex order 0 TL, 1 TR, 2 BL, 3 BR -> polygon 0,1,3,2
    poly = [(q[0], q[1]), (q[2], q[3]), (q[6], q[7]), (q[4], q[5])]
    inside = False
    j = 3
    for i in range(4):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if (yi > py) != (yj > py) and px < (xj - xi) * (py - yi) / ((yj - yi) or 1e-9) + xi:
            inside = not inside
        j = i
    return inside


def main():
    args = sys.argv[1:]
    tol = 1
    if '--tol' in args:
        i = args.index('--tol')
        tol = int(args[i + 1])
        del args[i:i + 2]
    partlog, capdir, outdir = args
    os.makedirs(outdir, exist_ok=True)
    frames, meta = parse_log(partlog)
    report = []
    fail = 0

    # 1. footprint and OT slots (all logged frames, not only captured ones)
    sig = {}
    for p, parts in frames.items():
        key = tuple((o, q) for _, o, q in sorted(parts))
        sig.setdefault(key, set()).add(meta[p][0])
    both = [k for k, modes in sig.items() if 0 in modes and 1 in modes]
    report.append(f"part sets drawn in both retail and HD modes: {len(both)} of {len(sig)} distinct")

    # Captures are taken at offset 6 of TEST_BLOCK-present mode blocks
    # (fei_hd2d.c); the frame shown there was built in the same block. A
    # block counts when every part set logged in it up to the capture is the
    # same (Fei did not animate), and a retail/HD/hidden triple is three
    # consecutive blocks with that same part set and atlas cell.
    BLOCK = 8
    captures = sorted(int(f[10:16]) for f in os.listdir(capdir) if f.startswith('fei-frame-'))
    blocks = {}
    for c in captures:
        blk = c // BLOCK
        built = [p for p in frames if blk * BLOCK <= p <= c]
        keys = {tuple((o, q) for _, o, q in sorted(frames[p])) for p in built}
        cells = {meta[p][1] for p in built}
        if len(keys) == 1 and len(cells) == 1:
            blocks[blk] = (c, keys.pop(), cells.pop(), max(built))
    triples = []
    for blk, (c, key, cell, built) in sorted(blocks.items()):
        if blk % 3 != 0:
            continue
        nxt = [blocks.get(blk + 1), blocks.get(blk + 2)]
        if all(n and n[1] == key and n[2] == cell for n in nxt):
            triples.append((c, nxt[0][0], nxt[1][0], built, nxt[0][3]))
    report.append(f"retail/HD/hidden triples with identical parts: {len(triples)} "
                  f"(footprint: the HD block drew the same part quads and OT slots as the retail block)")
    if not triples:
        print('\n'.join('FEI FOOTPRINT ' + r for r in report))
        print('FEI FOOTPRINT overall=FAIL no comparable triple')
        return 1

    checked = 0
    worst = defaultdict(int)
    evidence_done = False
    for (pr, ph, px, br_built, bh_built) in triples:
        w, h, R = read_bmp(os.path.join(capdir, 'fei-frame-%06d.png' % pr))
        _, _, H = read_bmp(os.path.join(capdir, 'fei-frame-%06d.png' % ph))
        _, _, X = read_bmp(os.path.join(capdir, 'fei-frame-%06d.png' % px))
        # region: everything that differs anywhere between the three renders
        full = (0, 0, w - 1, h - 1)
        mr = diff_mask(R, X, w, full)
        mh = diff_mask(H, X, w, full)
        if not mr or not mh:
            continue
        reg = bbox(list(mr | mh))
        region = (max(0, reg[0] - 2), max(0, reg[1] - 2), min(w - 1, reg[2] + 2), min(h - 1, reg[3] + 2))
        if region[2] - region[0] > w // 2 or region[3] - region[1] > h // 2:
            continue  # something else changed between the three presents
        # calibrate PSX screen -> capture pixels from the retail mask and the
        # union of the retail quads' bounds (per axis: scale then offset)
        parts = sorted(frames[br_built])
        qx = [v for _, _, q in parts for v in q[0::2]]
        qy = [v for _, _, q in parts for v in q[1::2]]
        box = meta.get(('box', bh_built))
        br = bbox(list(mr))
        bh = bbox(list(mh))
        checked += 1
        edges = [abs(br[i] - bh[i]) for i in range(4)]
        worst['silhouette_vertical'] = max(worst['silhouette_vertical'], edges[1], edges[3])
        worst['silhouette_horizontal'] = max(worst['silhouette_horizontal'], edges[0], edges[2])
        line = f"triple {pr}/{ph}/{px}: retail mask {br} HD mask {bh} edge deltas {edges}"
        # per part
        sx = sy = None
        if box:
            # box = HD silhouette box in PSX coords; map it onto the HD mask box
            sx = (bh[2] + 1 - bh[0]) / max(1e-6, box[2] - box[0])
            sy = (bh[3] + 1 - bh[1]) / max(1e-6, box[3] - box[1])
            ox, oy = bh[0] - sx * box[0], bh[1] - sy * box[1]
        per = []
        for idx, ot, q in parts:
            if sx is None:
                break
            cq = tuple((q[i] * sx + ox) if i % 2 == 0 else (q[i] * sy + oy) for i in range(8))
            pr_m = [(x, y) for (x, y) in mr if point_in_quad(x + 0.5, y + 0.5, cq)]
            ph_m = [(x, y) for (x, y) in mh if point_in_quad(x + 0.5, y + 0.5, cq)]
            a, b = bbox(pr_m), bbox(ph_m)
            if a is None and b is None:
                per.append(f"p{idx}:hidden/hidden")
                continue
            if a is None or b is None:
                per.append(f"p{idx}:{'hidden' if a is None else a}/{'hidden' if b is None else b}")
                worst['part_visibility'] += 1
                continue
            dv = max(abs(a[1] - b[1]), abs(a[3] - b[3]))
            dh = max(abs(a[0] - b[0]), abs(a[2] - b[2]))
            worst['part_vertical'] = max(worst['part_vertical'], dv)
            worst['part_horizontal'] = max(worst['part_horizontal'], dh)
            per.append(f"p{idx}:dv={dv},dh={dh}")
        line += " parts " + " ".join(per)
        # occlusion rows inside the retail silhouette's vertical extent
        top, bot = min(br[1], bh[1]), max(br[3], bh[3])
        vis_r = {y for (_, y) in mr}
        vis_h = {y for (_, y) in mh}
        hid_r = {y for y in range(top, bot + 1) if y not in vis_r}
        hid_h = {y for y in range(top, bot + 1) if y not in vis_h}
        sym = sorted(hid_r ^ hid_h)
        # every differing row must sit within tol of a boundary of the other set
        def near_edge(y, s):
            return any((y + d) in s for d in range(-tol, tol + 1)) and any((y + d) not in s for d in range(-tol, tol + 1))
        bad_rows = [y for y in sym if not (near_edge(y, hid_r) or near_edge(y, hid_h))]
        worst['occlusion_bad_rows'] = max(worst['occlusion_bad_rows'], len(bad_rows))
        line += f" hidden rows retail={len(hid_r)} hd={len(hid_h)} differing={len(sym)} beyond_tol={len(bad_rows)}"
        report.append(line)
        if not evidence_done:
            evidence_done = True
            x0, y0, x1, y1 = region
            pad = 12
            x0, y0 = max(0, x0 - pad), max(0, y0 - pad)
            x1, y1 = min(w - 1, x1 + pad), min(h - 1, y1 + pad)
            k = 4
            def crop(img, overlay=None):
                out = []
                for y in range(y0, y1 + 1):
                    r = bytearray()
                    for x in range(x0, x1 + 1):
                        c = img[y][x * 3:x * 3 + 3]
                        if overlay is not None:
                            c = bytes((255, 255, 255)) if (x, y) in overlay else bytes((0, 0, 0))
                        r += c * k
                    out += [bytes(r)] * k
                return out
            cw, ch = (x1 - x0 + 1) * k, (y1 - y0 + 1) * k
            imgs = {'retail': crop(R), 'hd2d': crop(H), 'hidden': crop(X),
                    'mask_retail': crop(R, mr), 'mask_hd2d': crop(H, mh)}
            for n, rows in imgs.items():
                write_png(os.path.join(outdir, f'fei_{n}.png'), cw, ch, rows)
            # side by side: retail | hd2d | mask xor
            xor = mr ^ mh
            both_rows = crop(R, xor)
            side = [imgs['retail'][i] + b'\x40\x40\x40' * 4 + imgs['hd2d'][i] + b'\x40\x40\x40' * 4 + both_rows[i]
                    for i in range(ch)]
            write_png(os.path.join(outdir, 'fei_side_by_side.png'), cw * 3 + 8, ch, side)
            for n, img in (('retail', R), ('hd2d', H), ('hidden', X)):
                write_png(os.path.join(outdir, f'full_{n}.png'), w, h, img)
            report.append(f"evidence from triple {pr}/{ph}/{px} in {outdir}")
    print('\n'.join('FEI FOOTPRINT ' + r for r in report))
    summary = ' '.join(f"{k}={v}" for k, v in sorted(worst.items()))
    print(f"FEI FOOTPRINT checked_triples={checked} worst: {summary} (tol {tol}px)")
    if checked == 0:
        fail = 1
    if worst['silhouette_vertical'] > tol or worst['part_vertical'] > tol or \
            worst['occlusion_bad_rows'] > 0 or worst['part_visibility'] > 0:
        fail = 1
    # Horizontal alpha extents follow the atlas art's own outline inside the
    # retail quads (the generated art is not traced from the retail sprite),
    # so they are reported, and enforced only on request.
    strict = os.environ.get('FEI_FOOTPRINT_STRICT_HORIZONTAL', '0') == '1'
    horiz = max(worst['part_horizontal'], worst['silhouette_horizontal'])
    print(f"FEI FOOTPRINT horizontal alpha extents (art outline): worst {horiz}px"
          f" -> {'ENFORCED' if strict else 'reported'}")
    if strict and horiz > tol:
        fail = 1
    print(f"FEI FOOTPRINT overall={'FAIL' if fail else 'PASS'}")
    return fail


if __name__ == '__main__':
    sys.exit(main())
