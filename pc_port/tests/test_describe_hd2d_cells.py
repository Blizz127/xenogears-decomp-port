import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from describe_hd2d_cells import content_bounds, describe

class AuthoredBoundsTest(unittest.TestCase):
    def test_silhouettes_crossing_uniform_grid_are_not_clipped(self):
        from PIL import Image
        image = Image.new('RGBA', (128, 128))
        # First row crosses the equal-grid boundary at y=32, while silhouettes
        # remain separated. Inspection must preserve those feet.
        for row, y in enumerate((5, 39, 70, 100)):
            for col in range(4):
                for yy in range(y, y + (29 if row == 0 else 20)):
                    for xx in range(col * 32 + 5, col * 32 + 20):
                        image.putpixel((xx, yy), (255, 128, 64, 255))
        bounds = content_bounds(image)
        self.assertEqual(len(bounds), 16)
        self.assertGreater(bounds[0][3], 32)
        metadata = describe(128, 128, bounds)
        self.assertEqual(metadata['game_mappings'], [])
        self.assertEqual(metadata['approved_opening_maps'], [])
        self.assertEqual(image.getpixel((5, 33)), (255, 128, 64, 255))

    def test_missing_silhouettes_fail_instead_of_inventing_frames(self):
        from PIL import Image
        with self.assertRaisesRegex(ValueError, 'sixteen'):
            content_bounds(Image.new('RGBA', (128, 128)))

if __name__ == '__main__':
    unittest.main()
