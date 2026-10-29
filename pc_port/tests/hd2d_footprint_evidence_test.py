#!/usr/bin/env python3
"""A CPU accept or old analyzer PASS alone cannot establish replacement art."""
import unittest
from hd2d_footprint_validation import replacement_evidence_pass as gate
class EvidenceTests(unittest.TestCase):
    def test_cpu_only_rejected(self): self.assertFalse(gate(0,0,100,True))
    def test_gpu_but_identical_pixels_rejected(self): self.assertFalse(gate(0,6,0,True))
    def test_no_per_part_calibration_rejected(self): self.assertFalse(gate(0,6,100,False))
    def test_strict_failure_rejected(self): self.assertFalse(gate(1,6,100,True))
    def test_all_evidence_required(self): self.assertTrue(gate(0,6,100,True))
if __name__=='__main__': unittest.main()
