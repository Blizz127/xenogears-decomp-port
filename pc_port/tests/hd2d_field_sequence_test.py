#!/usr/bin/env python3
"""Regression checks: no frame exclusion can turn a mutation into PASS."""
import unittest
from hd2d_field_sequence_compare import compare_sequence

class CompleteSequenceTests(unittest.TestCase):
    def test_exact_nonperiodic(self):
        self.assertTrue(compare_sequence([1,2,3], [1,2,3])['pass'])

    def test_complete_periodic_phase(self):
        result=compare_sequence([1,2]*4,[2,1]*4)
        self.assertTrue(result['pass'])
        self.assertEqual(result['period'],2)
        self.assertEqual(result['cycles'],4)

    def test_every_single_sample_mutant_fails(self):
        a=[1,2]*12
        for i in range(len(a)):
            b=[2,1]*12
            b[i]=9
            with self.subTest(index=i):
                self.assertFalse(compare_sequence(a,b)['pass'])

    def test_same_histogram_wrong_order_fails(self):
        self.assertFalse(compare_sequence([1,2]*4,[1,1,2,2]*2)['pass'])

    def test_count_mismatch_fails(self):
        self.assertFalse(compare_sequence([1,2]*4,[2,1]*3)['pass'])

    def test_nonperiodic_rotation_fails(self):
        self.assertFalse(compare_sequence([1,2,3],[2,3,1])['pass'])

    def test_empty_fails(self):
        self.assertFalse(compare_sequence([],[])['pass'])

if __name__=='__main__':
    unittest.main()
