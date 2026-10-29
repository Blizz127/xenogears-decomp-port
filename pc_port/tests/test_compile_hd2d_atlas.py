import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('atlas', Path(__file__).parents[1] / 'tools/compile_hd2d_atlas.py')
atlas = importlib.util.module_from_spec(spec)
spec.loader.exec_module(atlas)

class MetadataTest(unittest.TestCase):
    def make(self):
        return dict(schema=1, label=atlas.LABEL, width=4, height=4, maps=[2], frames=[
            dict(animation=0, cursor=0, facing=0, x=0, y=0, width=4, height=4, foot_x=2, foot_y=4)])

    def test_valid_exact_mapping(self):
        d = self.make()
        d['frames'][0]['cursor'] = 65535
        self.assertEqual(atlas.validate(d)[3][0]['cursor'], 65535)

    def test_malformed(self):
        changes = [('width', True), ('width', 4097), ('maps', []), ('maps', [2, 2]), ('label', 'retail')]
        for key, value in changes:
            with self.subTest(key=key, value=value):
                d = self.make(); d[key] = value
                with self.assertRaises(ValueError): atlas.validate(d)

    def test_bounds_and_keys(self):
        for key, value in [('x', 1), ('height', 0), ('facing', 16), ('foot_x', 5), ('cursor', -1)]:
            with self.subTest(key=key):
                d = self.make(); d['frames'][0][key] = value
                with self.assertRaises(ValueError): atlas.validate(d)
        d = self.make(); d['frames'].append(d['frames'][0].copy())
        with self.assertRaises(ValueError): atlas.validate(d)

if __name__ == '__main__': unittest.main()
