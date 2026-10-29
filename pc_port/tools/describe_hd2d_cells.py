#!/usr/bin/env python3
"""Record authored 4x4 atlas cells without inventing game animation mappings."""
import argparse
from collections import deque
import json
from pathlib import Path
from compile_hd2d_atlas import LABEL

def content_bounds(image):
    """Inspect alpha only; preserve every authored pixel and avoid grid clipping."""
    width, height = image.size
    alpha = image.getchannel('A').tobytes()
    seen = bytearray(width * height)
    minimum = max(4, width * height // 512)
    components = []
    for index, value in enumerate(alpha):
        if value < 128 or seen[index]:
            continue
        seen[index] = 1
        queue = deque([index])
        area = 0
        x0 = x1 = index % width
        y0 = y1 = index // width
        while queue:
            current = queue.popleft()
            x, y = current % width, current // width
            area += 1
            x0, x1 = min(x0, x), max(x1, x)
            y0, y1 = min(y0, y), max(y1, y)
            for yy in range(max(0, y - 1), min(height, y + 2)):
                for xx in range(max(0, x - 1), min(width, x + 2)):
                    neighbor = yy * width + xx
                    if alpha[neighbor] >= 128 and not seen[neighbor]:
                        seen[neighbor] = 1
                        queue.append(neighbor)
        if area >= minimum:
            components.append((x0, y0, x1 + 1, y1 + 1))
    if len(components) != 16:
        raise ValueError('expected exactly sixteen separate sprite silhouettes; review artwork')
    components.sort(key=lambda b: (b[1] + b[3], b[0]))
    ordered = []
    for row in range(4):
        group = sorted(components[row * 4:row * 4 + 4], key=lambda b: b[0] + b[2])
        for x0, y0, x1, y1 in group:
            ordered.append((max(0, x0 - 1), max(0, y0 - 1),
                            min(width, x1 + 1), min(height, y1 + 1)))
    for i, a in enumerate(ordered):
        for b in ordered[i + 1:]:
            if max(a[0], b[0]) < min(a[2], b[2]) and max(a[1], b[1]) < min(a[3], b[3]):
                raise ValueError('sprite rectangles overlap; review artwork')
    return ordered

def describe(width, height, bounds=None):
    if type(width) is not int or type(height) is not int or not 4 <= width <= 4096 or not 4 <= height <= 4096:
        raise ValueError('atlas dimensions must be integers 4..4096')
    cells = []
    for row, facing in enumerate(('front', 'back', 'left', 'right')):
        for column, phase in enumerate(('idle', 'left_step', 'passing', 'right_step')):
            x, y = column * width // 4, row * height // 4
            x1, y1 = (column + 1) * width // 4, (row + 1) * height // 4
            if bounds is not None:
                x, y, x1, y1 = bounds[row * 4 + column]
            cells.append(dict(name=f'{facing}_{phase}', facing=facing, authored_phase=phase,
                              x=x, y=y, width=x1-x, height=y1-y))
    return dict(schema=1, kind='authored_cells_only', label=LABEL,
                width=width, height=height, cells=cells,
                rectangle_source='inspected alpha silhouettes' if bounds else 'uniform grid (unverified artwork bounds)',
                game_mappings=[], approved_opening_maps=[],
                mapping_status='pending verified retail frame indices; not runtime metadata')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('image', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--content-bounds', action='store_true', help='inspect sixteen alpha silhouettes without clipping to a guessed grid')
    args = p.parse_args()
    from PIL import Image
    with Image.open(args.image) as im:
        if im.mode != 'RGBA':
            raise ValueError('expected authored RGBA PNG')
        data = describe(*im.size, bounds=content_bounds(im) if args.content_bounds else None)
    args.output.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
