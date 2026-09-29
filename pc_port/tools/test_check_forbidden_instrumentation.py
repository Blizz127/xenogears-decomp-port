#!/usr/bin/env python3
"""Unit fixtures for pc_port/tools/check_forbidden_instrumentation.py.

Run from the repo root:
    python3 pc_port/tools/test_check_forbidden_instrumentation.py
"""

import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import check_forbidden_instrumentation as checker  # noqa: E402

FIXTURES = os.path.join(HERE, "forbidden_fixtures")


def load(name):
    with open(os.path.join(FIXTURES, name), "r") as f:
        return f.read()


class CheckerFixtures(unittest.TestCase):
    def run_fixture(self, name):
        code, report = checker.check_log(load(name))
        return code, "\n".join(report)

    def test_valid_zero_passes(self):
        code, report = self.run_fixture("valid_zero.txt")
        self.assertEqual(code, 0, report)
        self.assertIn("RESULT PASS", report)

    def test_valid_hit_passes_when_optional(self):
        code, report = self.run_fixture("valid_hit.txt")
        self.assertEqual(code, 0, report)

    def test_not_instrumented_required_fails(self):
        code, report = self.run_fixture("not_instrumented.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("ctrl_unregistered", report)

    def test_registration_error_required_fails(self):
        code, report = self.run_fixture("registration_error.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("ctrl_deleted", report)

    def test_missing_required_label_fails(self):
        code, report = self.run_fixture("missing_required.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("cdsync_world", report)

    def test_duplicate_label_fails(self):
        code, report = self.run_fixture("duplicate_label.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("duplicate label", report)

    def test_legacy_numeric_dict_rejected(self):
        code, report = self.run_fixture("legacy_numeric_dict.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("legacy_numeric_dictionary", report)

    def test_registered_with_undefined_hits_rejected(self):
        code, report = self.run_fixture("undefined_hits_registered.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("undefined hits", report)

    def test_negative_hits_rejected(self):
        code, report = self.run_fixture("negative_hits.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("negative hit count", report)

    def test_unknown_status_rejected(self):
        code, report = self.run_fixture("unknown_status.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("unknown status", report)

    def test_status_count_contradiction_rejected(self):
        code, report = self.run_fixture("contradiction_zero_with_hits.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("ZERO_VERIFIED but hits", report)

    def test_missing_block_rejected(self):
        code, report = self.run_fixture("no_block.txt")
        self.assertEqual(code, 1, report)
        self.assertIn("no *_FORBIDDEN_SUMMARY_BEGIN block", report)

    def test_required_override(self):
        # valid_zero fixture requires 712d0,74e58; overriding with a label
        # that is not in the summary must fail.
        code, report = checker.check_log(load("valid_zero.txt"),
                                         required_override=["no_such_label"])
        report = "\n".join(report)
        self.assertEqual(code, 1, report)
        self.assertIn("no_such_label", report)


if __name__ == "__main__":
    unittest.main(verbosity=2)
