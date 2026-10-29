import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from compile_hd2d_parts import compile_profile, LABEL

class PartsTest(unittest.TestCase):
    def setUp(self):
        self.data = dict(schema=1, label=LABEL, width=64, height=64,
            rgba_path='pc_port/assets/hd2d/fei.rgba', parts=[dict(identity=123,
            map=7, actor=0, frame=2, part=0, tpage=9, clut=8,
            u=[1,2,1,2], v=[3,3,4,4], rect=[0,0,16,32], trace='capture.json:17')])
    def test_explicit_only(self):
        self.assertIn('hd2d_parts', compile_profile(self.data))
        self.data['parts'] = []
        self.assertIn('NULL, 0,', compile_profile(self.data))
    def test_reject_ambiguity_or_unverified(self):
        self.data['parts'].append(copy.deepcopy(self.data['parts'][0]))
        with self.assertRaises(ValueError): compile_profile(self.data)
        self.data['parts'].pop()
        self.data['parts'][0]['trace'] = ''
        with self.assertRaises(ValueError): compile_profile(self.data)
    def test_bad_inputs(self):
        for field,value in [('identity',True), ('frame',32768), ('rect',[60,0,16,32]), ('u',[1,2,3]), ('part',64)]:
            d=copy.deepcopy(self.data); d['parts'][0][field]=value
            with self.assertRaises(ValueError): compile_profile(d)
        self.data['rgba_path']='pc_port/assets/hd2d/../x.rgba'
        with self.assertRaises(ValueError): compile_profile(self.data)
if __name__ == '__main__': unittest.main()
