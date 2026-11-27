"""Regression checks for C-string views of arbitrary binary or textual inputs."""
import hashlib
from unittest import TestCase, main
from unittest.mock import patch
import zlib

import public_digest_short as guard


class ShortEscapeTests(TestCase):
    def test_bare_hex_escape_preserves_both_bytes(self):
        self.assertEqual(guard._decode(br'\x'), br'\x')
        self.assertEqual(guard._decode(br'\x!\x?'), br'\x!\x?')
        self.assertEqual(guard._decode(br'\x4a\x153'), b'JS')

    def test_bare_escape_keeps_adjacent_positive_and_rejects_mutation(self):
        value = b'Z:/guard-control'
        size = len(value)
        signatures = {size: {zlib.crc32(value): hashlib.sha256(value).hexdigest()}}

        def encoded(data):
            return b'\0"' + br'\x!' + b''.join(b'\\x%02x' % x for x in data) + br'\x?"'

        with patch.object(guard, '_SIGNATURES', signatures):
            findings = guard.check_short('arbitrary.bin', encoded(value))
            self.assertTrue(any('in C string' in item for item in findings))
            self.assertEqual(guard.check_short('arbitrary.bin', encoded(value[:-1]+b'x')), [])


if __name__=='__main__':
    main()
