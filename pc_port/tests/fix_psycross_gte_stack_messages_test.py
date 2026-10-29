#!/usr/bin/env python3
"""Source-transform checks use invented diagnostics, never SDK text."""

import importlib.util
from pathlib import Path
import tempfile
import subprocess
import sys
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "tools/fix_psycross_gte_stack_messages.py"
SPEC = importlib.util.spec_from_file_location("fix_gte_messages", SCRIPT)
FIX = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FIX)


def fixture(malformed=False):
    parts = ["// unrelated prefix\n"]
    for name, end in FIX.SUCCESS_END.items():
        prefix = f"void {name}()\n{{\n\tif (test_condition)\n\t{{\n" + end
        diagnostic = '\t\tprintf("fixture diagnostic\\n");\n'
        if malformed:
            suffix = ("#if 0 // _xeno_gte_stack_messages: the SDK text is not shipped\n"
                      "\telse\n#endif\n" + diagnostic + "\t{\n" + diagnostic + "\t}\n")
        else:
            suffix = "\telse\n\t{\n" + diagnostic + "\t}\n"
        parts.append(prefix + suffix + "}\n")
    return "".join(parts) + "// unrelated suffix\n"


class StackMessagesTransform(unittest.TestCase):
    def test_pristine_and_previous_patch(self):
        for old in (False, True):
            with self.subTest(malformed=old):
                before = fixture(old)
                after, count = FIX.repair(before)
                self.assertEqual(count, 2)
                self.assertNotIn("fixture diagnostic", after)
                self.assertNotIn("#if", after)
                for name, end in FIX.SUCCESS_END.items():
                    prefix = before.split(f"void {name}()\n{{\n")[1].split(end)[0] + end
                    self.assertIn(prefix, after)
                    self.assertIn(f'fputs("{FIX.MESSAGE[name]}\\n", stdout);', after)
                self.assertTrue(after.startswith("// unrelated prefix\n"))
                self.assertTrue(after.endswith("// unrelated suffix\n"))
                self.assertEqual(FIX.repair(after), (after, 0))

    def test_unknown_or_duplicate_rejected(self):
        for bad in (
            fixture().replace("matrixLevel++;", "matrixLevel += 2;"),
            fixture().replace("printf(", "other_print(", 1),
            fixture().replace("void PopMatrix()", "void OtherMatrix()"),
            fixture() + fixture(),
        ):
            with self.subTest(source=bad):
                with self.assertRaises(ValueError):
                    FIX.repair(bad)

    def test_cli_does_not_partially_repair_on_later_error(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "LIBGTE.C"
            initial = fixture(True).replace("void PopMatrix()", "void OtherMatrix()")
            path.write_text(initial)
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(path)], capture_output=True, text=True
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(path.read_text(), initial)

    def test_cli_preserves_vendor_comment_encoding(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "LIBGTE.C"
            initial = b"// legacy vendor comment \x82\xff\n" + fixture(True).encode()
            path.write_bytes(initial)
            subprocess.run([sys.executable, str(SCRIPT), str(path)],
                           check=True, capture_output=True)
            fixed = path.read_bytes()
            self.assertTrue(fixed.startswith(b"// legacy vendor comment \x82\xff\n"))
            subprocess.run([sys.executable, str(SCRIPT), str(path)],
                           check=True, capture_output=True)
            self.assertEqual(path.read_bytes(), fixed)


if __name__ == "__main__":
    unittest.main()
