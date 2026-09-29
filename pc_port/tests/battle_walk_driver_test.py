"""Exercise the real walker's battle driver without a window or save state."""
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest

ROOT = Path(__file__).resolve().parents[2]
TREE = ast.parse((ROOT / 'scratchpad/blackmoon-route-20260919/cam_walk.py').read_text())
FUNCTION = next(n for n in TREE.body if isinstance(n, ast.FunctionDef)
                and n.name == 'fight_until_done')


class DriverTests(unittest.TestCase):
    def run_driver(self, finish_key=None, active=True, limit=2):
        sent = []
        def tap(key, hold):
            nonlocal active
            self.assertTrue(active, 'input escaped into the field')
            sent.append(key)
            if key == finish_key:
                active = False
        scope = {'battle_active': lambda: active, 'tap': tap,
                 'time': SimpleNamespace(sleep=lambda _: None)}
        exec(compile(ast.Module(body=[FUNCTION], type_ignores=[]), '<driver>', 'exec'), scope)
        result = scope['fight_until_done'](limit)
        return result, sent

    def test_waiting_square_attack_resumes(self):
        result, sent = self.run_driver(finish_key='x')
        self.assertTrue(result)
        self.assertEqual(sent[-1], 'x')

    def test_no_keys_after_any_battle_return(self):
        for key in ('z', 'v', 'x'):
            result, sent = self.run_driver(finish_key=key)
            self.assertTrue(result)
            self.assertEqual(sent[-1], key)

    def test_field_is_untouched(self):
        self.assertEqual(self.run_driver(active=False), (True, []))

    def test_unresolved_battle_is_bounded(self):
        result, sent = self.run_driver(limit=1)
        self.assertFalse(result)
        self.assertLessEqual(len(sent), 8)


if __name__ == '__main__':
    unittest.main()
