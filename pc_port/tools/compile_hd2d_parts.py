#!/usr/bin/env python3
"""Compile explicit verified runtime part traces into the production C profile.
No animation, UV, identity, part or screen geometry is inferred from authored art.
"""
import argparse
import json
from pathlib import Path
from compile_hd2d_atlas import LABEL, integer

FIELDS = {'identity', 'map', 'actor', 'frame', 'part', 'tpage', 'clut', 'u', 'v', 'rect', 'trace'}

def compile_profile(data):
    if set(data) != {'schema', 'label', 'width', 'height', 'rgba_path', 'parts'}:
        raise ValueError('exact profile fields required')
    if type(data['schema']) is not int or data['schema'] != 1 or data['label'] != LABEL:
        raise ValueError('schema 1 and exact provenance label required')
    w = integer(data['width'], 1, 4096, 'width')
    h = integer(data['height'], 1, 4096, 'height')
    path = data['rgba_path']
    if not isinstance(path, str) or not path.startswith('pc_port/assets/hd2d/') or '..' in path or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_./-' for c in path):
        raise ValueError('production RGBA path required')
    parts = data['parts']
    if not isinstance(parts, list) or len(parts) > 16384:
        raise ValueError('parts must be explicit list, at most 16384')
    keys, rows = set(), []
    for p in parts:
        if not isinstance(p, dict) or set(p) != FIELDS:
            raise ValueError('exact traced part fields required')
        if not isinstance(p['trace'], str) or not p['trace'].strip():
            raise ValueError('verified trace reference required')
        values = [integer(p[k], 0, hi, k) for k, hi in [('identity', 0xffffffff), ('map', 65535), ('actor', 65535), ('frame', 32767), ('part', 63), ('tpage', 65535), ('clut', 65535)]]
        key = tuple(values[:5])
        if key in keys:
            raise ValueError('ambiguous duplicate runtime key')
        keys.add(key)
        vectors = []
        for field in ('u', 'v', 'rect'):
            v = p[field]
            if not isinstance(v, list) or len(v) != 4:
                raise ValueError('four explicit values required')
            vectors.append([integer(x, 0, 255 if field != 'rect' else 4096, field) for x in v])
        x, y, rw, rh = vectors[2]
        if not rw or not rh or x + rw > w or y + rh > h:
            raise ValueError('part rectangle outside atlas')
        rows.append('    {' + ', '.join(map(str, values)) + ', {' + ', '.join(map(str, vectors[0])) + '}, {' + ', '.join(map(str, vectors[1])) + '}, ' + ', '.join(map(str, vectors[2])) + '}')
    result = '/* ' + LABEL + ' */\n#include "hd2d_sprite_parts.h"\n'
    if rows:
        result += 'static const PcPortHd2dPartMapping hd2d_parts[] = {\n' + ',\n'.join(rows) + '\n};\n'
    result += f'static const PcPortHd2dPartProfile hd2d_sprite_profile = {{{w}, {h}, ' + ('hd2d_parts' if rows else 'NULL') + f', {len(rows)}, "{path}"}};\n'
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('header', type=Path)
    args = parser.parse_args()
    result = compile_profile(json.loads(args.manifest.read_text(encoding='utf-8')))
    args.header.write_text(result, encoding='utf-8')

if __name__ == '__main__':
    main()
