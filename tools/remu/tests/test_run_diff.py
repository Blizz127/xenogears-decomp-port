"""Exercise the real runner with tiny compilable fixtures in an isolated repo."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

RUNNER = pathlib.Path(__file__).resolve().parents[1] / "run_diff.sh"


class RunnerGateTests(unittest.TestCase):
    def run_gate(self, mutator):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            tests = root / "tools/remu/tests"
            tests.mkdir(parents=True)
            shutil.copyfile(RUNNER, root / "tools/remu/run_diff.sh")
            (tests / "remu_shim.h").write_text("")
            (root / "tools/remu/remu.c").write_text("")
            (root / "candidate.c").write_text("int candidate(void) { return 0; }\n")
            (tests / "fixture.c").write_text(
                "int candidate(void); int main(void) { return candidate(); }\n")
            (tests / "mut_fixture.sh").write_text(mutator)
            return subprocess.run(
                ["bash", str(root / "tools/remu/run_diff.sh"), "fixture", "candidate.c"],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                env={**os.environ, "TMPDIR": tmp})

    def test_mutant_rejected(self):
        result = self.run_gate('echo "int candidate(void) { return 1; }" > "$2"\n')
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("MUTANT REJECTED", result.stdout)

    def test_broken_controls_fail(self):
        controls = {
            "mutator": 'echo "int candidate(void) { return 1; }" > "$2"\nexit 9\n',
            "compilation": 'echo "invalid C" > "$2"\n',
            "linking": 'echo "extern int missing(void); int candidate(void) { return missing(); }" > "$2"\n',
            "surviving mutant": 'cp "$1" "$2"\n',
        }
        for name, mutator in controls.items():
            with self.subTest(name=name):
                result = self.run_gate(mutator)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertNotIn("ALL GREEN", result.stdout)
                if name != "surviving mutant":
                    self.assertIn("MUTANT PREPARATION FAILED: " + name, result.stdout)


if __name__ == "__main__":
    unittest.main()
