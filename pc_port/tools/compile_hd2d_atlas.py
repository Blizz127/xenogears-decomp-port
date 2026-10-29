#!/usr/bin/env python3
"""Validate authored metadata and emit dependency-free C metadata + raw RGBA.
PNG conversion requires Pillow only at authoring time; runtime has no PNG/JSON dependency.
"""
import argparse
import json
from pathlib import Path

LABEL = 'fan-made, AI-generated replacement sprite art created for this project; not from the game; characters © Square Enix'
FIELDS = ('animation', 'cursor', 'facing', 'x', 'y', 'width', 'height', 'foot_x', 'foot_y')

def integer(value, lo, hi, name):
    if type(value) is not int or not lo <= value <= hi:
        raise ValueError(f'{name}: expected integer {lo}..{hi}')
    return value

def validate(data):
    if data.get('schema') != 1 or data.get('label') != LABEL:
        raise ValueError('schema 1 and exact provenance label required')
    w = integer(data['width'], 1, 4096, 'width')
    h = integer(data['height'], 1, 4096, 'height')
    maps = data['maps']
    frames = data['frames']
    if not isinstance(maps, list) or not 1 <= len(maps) <= 32:
        raise ValueError('maps must contain 1..32 explicit opening map IDs')
    for m in maps:
        integer(m, 0, 65535, 'map')
    if len(set(maps)) != len(maps):
        raise ValueError('duplicate map')
    if not isinstance(frames, list) or not 1 <= len(frames) <= 256:
        raise ValueError('frames must contain 1..256 records')
    keys = set()
    for f in frames:
        if set(f) != set(FIELDS):
            raise ValueError('frame fields must match documented schema exactly')
        for k in FIELDS:
            integer(f[k], 0, 65535, k)
        if f['facing'] >= 16 or not f['width'] or not f['height']:
            raise ValueError('invalid facing or empty rectangle')
        if f['x'] + f['width'] > w or f['y'] + f['height'] > h:
            raise ValueError('frame outside atlas')
        if f['foot_x'] > f['width'] or f['foot_y'] > f['height']:
            raise ValueError('foot anchor outside rectangle')
        key = (f['animation'], f['cursor'], f['facing'])
        if key in keys:
            raise ValueError('duplicate animation/cursor/facing mapping')
        keys.add(key)
    return w, h, maps, frames

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('metadata', type=Path)
    p.add_argument('image', type=Path, help='original authored PNG or tightly packed .rgba')
    p.add_argument('output', type=Path, help='output directory')
    args = p.parse_args()
    data = json.loads(args.metadata.read_text(encoding='utf-8'))
    w, h, maps, frames = validate(data)
    if args.image.suffix.lower() == '.png':
        from PIL import Image
        with Image.open(args.image) as im:
            if im.size != (w, h) or im.mode != 'RGBA':
                raise ValueError('PNG must match metadata dimensions and have RGBA channels')
            raw = im.tobytes()
    elif args.image.suffix.lower() == '.rgba':
        raw = args.image.read_bytes()
    else:
        raise ValueError('only PNG or raw RGBA accepted')
    if len(raw) != w * h * 4:
        raise ValueError('RGBA length does not match metadata')
    for f in frames:
        if not any(raw[((f['y'] + y) * w + f['x'] + x) * 4 + 3]
                   for y in range(f['height']) for x in range(f['width'])):
            raise ValueError('empty transparent mapped frame')
    rows = ',\n'.join('    {' + ', '.join(str(f[k]) for k in FIELDS) + '}' for f in frames)
    source = ('/* ' + LABEL + ' */\n#include "hd2d_sprite_atlas.h"\n'
              'static const uint16_t opening_maps[] = {' + ', '.join(map(str, maps)) + '};\n'
              'static const XgHd2dSpriteFrame frames[] = {\n' + rows + '\n};\n'
              f'const XgHd2dSpriteAtlas xg_hd2d_fei_atlas = {{{w}, {h}, {len(frames)}, {len(maps)}, frames, opening_maps}};\n')
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'atlas.rgba').write_bytes(raw)
    (args.output / 'atlas_metadata.c').write_text(source, encoding='utf-8')
    (args.output / 'atlas.json').write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
