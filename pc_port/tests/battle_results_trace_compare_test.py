"""Reject false parity from unstable references, omitted tail bytes or truncation."""
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('trace_compare', Path(__file__).with_name('battle_results_trace_compare.py'))
trace = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trace)


class ComparisonTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=os.environ['RESULTS_TRACE_TEST_OUT'])
        self.addCleanup(self.temp.cleanup)
        self.dirs = [Path(self.temp.name) / name for name in ('a', 'b', 'native')]
        for directory in self.dirs:
            directory.mkdir()
            (directory / 'snapshots.log').write_text(
                'FORMAT 1 synthetic\n'
                'S 1 1 entry 0 ra=80071698 sp=801ffff0 status=OK\n'
                'S 1 1 return 0 ra=80071698 sp=801ffff0 status=OK\n')
            for phase in ('entry', 'return'):
                (directory / f'b1-r1-{phase}-0.bin').write_bytes(bytes(trace.SNAPSHOT_SIZE))

    def test_stable_reference_and_native(self):
        self.assertEqual(trace.compare(*self.dirs), 2)

    def test_native_transition_tail_difference(self):
        path = self.dirs[2] / 'b1-r1-return-0.bin'
        data = bytearray(path.read_bytes())
        data[0x2358] = 1
        path.write_bytes(data)
        with self.assertRaisesRegex(ValueError, 'native snapshots'):
            trace.compare(*self.dirs)

    def test_unstable_reference(self):
        path = self.dirs[1] / 'b1-r1-entry-0.bin'
        data = bytearray(path.read_bytes())
        data[0] = 1
        path.write_bytes(data)
        with self.assertRaisesRegex(ValueError, 'reference runs differ'):
            trace.compare(*self.dirs)

    def test_truncated_or_unreturned(self):
        path = self.dirs[2] / 'b1-r1-return-0.bin'
        path.write_bytes(bytes(trace.SNAPSHOT_SIZE - 1))
        with self.assertRaisesRegex(ValueError, 'truncated'):
            trace.compare(*self.dirs)
        path.write_bytes(bytes(trace.SNAPSHOT_SIZE))
        log = self.dirs[2] / 'snapshots.log'
        log.write_text(log.read_text().split('S 1 1 return')[0])
        with self.assertRaisesRegex(ValueError, 'complete RESULTS'):
            trace.compare(*self.dirs)


if __name__ == '__main__':
    unittest.main()
