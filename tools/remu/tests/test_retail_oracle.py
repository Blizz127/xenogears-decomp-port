"""Verify directory promotion/demotion with real retail bytes and remu."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]


class OracleLocationTests(unittest.TestCase):
    def test_generated_locations_and_invalid_oracles(self):
        relative = pathlib.Path("main73/func_800AF400.s")
        source = next(ROOT / "asm/battle" / folder / relative
                      for folder in ("matchings", "nonmatchings")
                      if (ROOT / "asm/battle" / folder / relative).exists())
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            driver = root / "driver.c"
            driver.write_text('''#include "retail_oracle.h"
int main(int argc, char **argv) {
    remu_t *m = remu_create();
    if (!m || argc != 2) return 2;
    uint32_t entry = remu_load_oracle(m, argv[1]);
    remu_destroy(m);
    return entry == 0x800AF400u ? 0 : 1;
}
''')
            binary = root / "oracle"
            subprocess.run([os.environ.get("REMU_CC", "clang"), "-std=gnu17",
                            "-I" + str(ROOT / "tools/remu/tests"),
                            "-I" + str(ROOT / "tools/remu"), str(driver),
                            str(ROOT / "tools/remu/remu.c"), "-o", str(binary)],
                           check=True)
            paths = [root / "asm/battle" / folder / relative
                     for folder in ("matchings", "nonmatchings")]
            for path in paths:
                path.parent.mkdir(parents=True)
            for available in paths:
                shutil.copyfile(source, available)
                for requested in paths:
                    with self.subTest(available=available, requested=requested):
                        self.assertEqual(subprocess.run(
                            [str(binary), str(requested)]).returncode, 0)
                available.unlink()
            self.assertEqual(subprocess.run([str(binary), str(paths[0])]).returncode, 1)
            # An invalid existing file must not silently use the alternate.
            paths[0].write_text("invalid oracle\n")
            shutil.copyfile(source, paths[1])
            self.assertEqual(subprocess.run([str(binary), str(paths[0])]).returncode, 1)


if __name__ == "__main__":
    unittest.main()
