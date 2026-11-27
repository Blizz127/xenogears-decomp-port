"""Exercise public source guard behavior with an independently written synthetic fixture."""
import hashlib
import unittest
from unittest.mock import patch

import public_digest_translation as guard


class TranslationTests(unittest.TestCase):
    def setUp(self):
        tokens = [b"if", b"(", b"flag", b")", b"{", b"return", b"14", b";", b"}", b"return", b"3", b";"]
        digest = hashlib.sha256(b"\0".join(tokens)).hexdigest()
        self.label = "known translated source body: synthetic test"
        self.addCleanup(patch.stopall)
        patch.object(guard, "_CATALOG", {digest: [len(tokens), "synthetic test"]}).start()
        patch.object(guard, "_SIZES", {len(tokens)}).start()

    def test_renamed_and_reformatted(self):
        text = (b'int other(int flag) {\r\n    #line 9 "harmless/path.h"\n'
                b'if /* tag */(flag) { return 14; } return \\\r\n3; }')
        self.assertEqual(guard.check_translation("innocent.data", text), [self.label])

    def test_text_wrappers_and_macro_continuation(self):
        body = b"{ if(flag){return 14;} return 3; }"
        for candidate in (b"/* int saved(int flag)" + body + b" */",
                          b" #define OLD(flag) \\\n" + body):
            self.assertEqual(guard.check_translation("any", candidate), [self.label])

    def test_unrelated_and_strings(self):
        body = b"{if(flag){return 14;}return 3;}"
        for source in (b"int f(){return 3;}", b"void x(){return;}",
                       b"int f(int flag)" + body.replace(b"14", b"15"),
                       b'char *example="' + body + b'";', b"\0" + body):
            self.assertEqual(guard.check_translation("x.c", source), [])


if __name__ == "__main__":
    unittest.main()
