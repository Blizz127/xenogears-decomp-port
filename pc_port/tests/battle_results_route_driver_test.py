"""Focused process lifecycle/isolation tests; this does not run the game."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location(
    "route_driver", Path(__file__).with_name("battle_results_route_driver.py"))
DRIVER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DRIVER)


class DriverTests(unittest.TestCase):
    def test_private_files_and_complete_log_on_exit(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            checkpoint = root / "input.xgqs"
            checkpoint.write_bytes(b"synthetic driver fixture")
            child = root / "fake-port"
            child.write_text("""#!/usr/bin/env python3
import json,os
from pathlib import Path
keys=['HOME','XDG_CONFIG_HOME','XDG_STATE_HOME','XENO_MEMCARD_DIR',
      'XENO_CONFIG','XENO_QUICKSAVE_PATH']
Path('environment.json').write_text(json.dumps({k:os.environ[k] for k in keys}))
Path(os.environ['XENO_QUICKSAVE_PATH']).write_bytes(b'child wrote private copy')
assert all(k not in os.environ for k in ['DISPLAY','WAYLAND_DISPLAY','XAUTHORITY'])
print('last child line',flush=True)
""")
            child.chmod(0o700)
            out = root / "run"
            with patch.dict(os.environ, {"XENO_ASSISTS_CKPT": str(checkpoint)}):
                port = DRIVER.Port(str(child), str(out), {
                    "HOME": "/invalid/inherited", "DISPLAY": "owner-display",
                    "XENO_CONFIG": "/invalid/inherited-config",
                    "XENO_QUICKSAVE_PATH": str(checkpoint)})
                self.assertIsNotNone(port.wait_for("last child line", 5))
                self.assertEqual(port.stop(), 0)
            self.assertFalse(port.reader.is_alive())
            self.assertEqual(checkpoint.read_bytes(), b"synthetic driver fixture")
            self.assertEqual((out / "checkpoint.xgqs").read_bytes(),
                             b"child wrote private copy")
            for value in json.loads((out / "environment.json").read_text()).values():
                self.assertTrue(Path(value).is_relative_to(out))
            log = (out / "run.log").read_text()
            self.assertIn("last child line", log)
            self.assertTrue(log.rstrip().endswith("[driver] stop rc=0"))

    def test_shutdown_of_running_process_preserves_final_marker(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            checkpoint = root / "input.xgqs"
            checkpoint.write_bytes(b"synthetic driver fixture")
            child = root / "fake-port"
            child.write_text("#!/usr/bin/env python3\nimport time\n"
                             "print('ready',flush=True)\ntime.sleep(30)\n")
            child.chmod(0o700)
            out = root / "run"
            with patch.dict(os.environ, {"XENO_ASSISTS_CKPT": str(checkpoint)}):
                port = DRIVER.Port(str(child), str(out), {})
                self.assertIsNotNone(port.wait_for("ready", 5))
                rc = port.stop()
            self.assertLess(rc, 0)
            self.assertFalse(port.reader.is_alive())
            self.assertIn(f"[driver] stop rc={rc}", (out / "run.log").read_text())

    def test_incomplete_probe_arguments_fail_cleanly(self):
        self.assertEqual(DRIVER.main(["driver", "probe", "binary", "out", "name"]), 2)


if __name__ == "__main__":
    unittest.main()
