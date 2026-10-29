"""The lowered runtime must not swallow unrelated host exceptions."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'parser/stdlibs/target'))
import ispa_std


class PrimitiveTests(unittest.TestCase):
    def test_nested_failure_and_success(self):
        def inner():
            ispa_std.fail(10, 'no match')

        def outer():
            self.assertEqual(ispa_std.attempt(inner), 10)
            self.assertEqual(ispa_std.failure_message(), 'no match')
            ispa_std.fail(11, 'external parser failed')

        self.assertEqual(ispa_std.attempt(outer), 11)
        self.assertEqual(ispa_std.failure_message(), 'external parser failed')
        self.assertEqual(ispa_std.attempt(lambda: None), 0)
        self.assertEqual(ispa_std.failure_message(), 'external parser failed')

    def test_host_exception_propagates(self):
        error = ValueError('host error')

        def host():
            raise error

        with self.assertRaises(ValueError) as caught:
            ispa_std.attempt(host)
        self.assertIs(caught.exception, error)


if __name__ == '__main__':
    unittest.main()
