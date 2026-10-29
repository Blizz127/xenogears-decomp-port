import unittest
from hd2d_head_crop_report import groups
class GroupTest(unittest.TestCase):
    def test_exact_three_blocks(self):
        rows=['present=1214 variant=2 registered=1','present=1238 variant=3 registered=1']
        self.assertEqual(groups(rows,{1206,1214,1222,1230,1238,1246}),{2:{1206,1214,1222},3:{1230,1238,1246}})
    def test_missing_or_ambiguous(self):
        rows=['present=1214 variant=2 registered=1','present=1213 variant=3 registered=1']
        self.assertEqual(groups(rows,{1206,1214,1222}),{})
        self.assertEqual(groups(['present=1214 variant=2 registered=1'],{1206,1214}),{})
if __name__=='__main__':unittest.main()
