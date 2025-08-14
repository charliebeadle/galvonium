import unittest
from serialio.parser import is_eoc, accumulate_dump_lines, parse_dump_text


class TestParser(unittest.TestCase):
    def test_is_eoc(self):
        self.assertTrue(is_eoc("EOC"))
        self.assertTrue(is_eoc("eoc\n"))
        self.assertFalse(is_eoc("EOC!"))
        self.assertFalse(is_eoc("OK"))

    def test_accumulate_and_parse_dump(self):
        lines = [
            "0: 128,64 255",
            "1: 130,66 240",
            "EOC",
            "TRAILING IGNORED",
        ]
        text = accumulate_dump_lines(lines)
        self.assertIn("0: 128,64 255", text)
        self.assertIn("1: 130,66 240", text)

        # This calls your BufferData.from_dump_response
        buf = parse_dump_text(text)
        # Basic sanity: last used should be index 1
        self.assertGreaterEqual(buf.get_last_used_index(), 1)


if __name__ == "__main__":
    unittest.main()
