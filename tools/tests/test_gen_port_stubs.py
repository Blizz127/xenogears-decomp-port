"""Exercise the real generator with real ELF symbol tables, without game builds."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
GENERATOR = ROOT / 'tools/scripts/gen_port_stubs.py'

class StubClassificationTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = pathlib.Path(self.temp.name)
        for name, source in [('menu', 'int menu_helper(void) {return 1;}'),
                             ('main', 'unsigned char retail_data[40]; int retail_helper(void) {return 2;}')]:
            p = self.base / (name + '.c')
            p.write_text(source)
            subprocess.run(['gcc', '-c', str(p), '-o', str(p.with_suffix('.o'))], check=True)
        self.out = self.base / 'stubs.c'
        self.out.write_text('preserve previous manifest\n')

    def run_generator(self, names, owners):
        args = [sys.executable, str(GENERATOR), '--out', str(self.out)]
        for owner in owners:
            args += ['--elf', str(self.base / (owner + '.o'))]
        return subprocess.run(args, input='\n'.join(names), text=True, capture_output=True)

    def test_complete_owners_preserve_function_and_data_types(self):
        result = self.run_generator(['menu_helper', 'retail_helper', 'retail_data',
                                     'xeno_port_stub', 'xeno_port_is_generated_stub'], ['menu', 'main'])
        self.assertEqual(result.returncode, 0, result.stderr)
        source = self.out.read_text()
        self.assertIn('unsigned char retail_data[80]', source)
        self.assertIn('long retail_helper(void)', source)
        self.assertNotIn('long retail_data(void)', source)
        self.assertNotIn('long xeno_port_stub(void)', source)

    def test_partial_owners_reject_missing_data_without_overwriting(self):
        result = self.run_generator(['menu_helper', 'retail_data'], ['menu'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('retail_data', result.stderr)
        self.assertEqual(self.out.read_text(), 'preserve previous manifest\n')

    def test_unknown_function_name_is_not_authority(self):
        result = self.run_generator(['func_801FFFF0'], ['menu', 'main'])
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.out.read_text(), 'preserve previous manifest\n')

    def test_unrelated_missing_overlay_does_not_block_known_symbols(self):
        result = self.run_generator(['menu_helper'], ['menu'])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_build_driver_stops_on_partial_elf_classification(self):
        # Execute the production selection/classification block without the
        # expensive preceding game compilation. A stale manifest must survive,
        # but the driver must not proceed to its compile/link stage.
        (self.base / 'build/out').mkdir(parents=True)
        (self.base / 'tools/scripts').mkdir(parents=True)
        (self.base / 'tools/scripts/gen_port_stubs.py').symlink_to(GENERATOR)
        (self.base / 'build/out/menu.elf').symlink_to(self.base / 'menu.o')
        (self.base / 'undef.txt').write_text('menu_helper\nretail_data\n')
        source = (ROOT / 'pc_port/build_port.sh').read_text()
        start = source.index('if [ -s "$OUT/undef.txt" ]; then')
        end = source.index('    nm -g --defined-only', start)
        block = source[start:end] + '\nfi\necho LINK_STAGE_REACHED\n'
        result = subprocess.run(['bash', '-c', 'set -uo pipefail; OUT=.; ' + block],
                                cwd=self.base, text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn('LINK_STAGE_REACHED', result.stdout)
        self.assertIn('stub classification failed', result.stdout)
        self.assertEqual(self.out.read_text(), 'preserve previous manifest\n')

if __name__ == '__main__':
    unittest.main()
