import unittest
from unittest.mock import Mock, patch, MagicMock

from serialio.parser import is_eoc, accumulate_dump_lines, parse_dump_text


class TestParser(unittest.TestCase):
    """Test the parser functions"""

    def test_is_eoc_basic(self):
        """Test basic EOC detection"""
        self.assertTrue(is_eoc("EOC"))
        self.assertTrue(is_eoc("eoc"))
        self.assertTrue(is_eoc("Eoc"))
        self.assertTrue(is_eoc("eOc"))

    def test_is_eoc_with_whitespace(self):
        """Test EOC detection with various whitespace"""
        self.assertTrue(is_eoc(" EOC"))
        self.assertTrue(is_eoc("EOC "))
        self.assertTrue(is_eoc(" EOC "))
        self.assertTrue(is_eoc("\tEOC\n"))
        self.assertTrue(is_eoc("\nEOC\r"))
        self.assertTrue(is_eoc("  EOC  "))

    def test_is_eoc_with_newlines(self):
        """Test EOC detection with newlines"""
        self.assertTrue(is_eoc("EOC\n"))
        self.assertTrue(is_eoc("EOC\r"))
        self.assertTrue(is_eoc("EOC\r\n"))
        self.assertTrue(is_eoc("\nEOC"))
        self.assertTrue(is_eoc("\rEOC"))
        self.assertTrue(is_eoc("\r\nEOC"))

    def test_is_eoc_false_cases(self):
        """Test cases that should NOT be detected as EOC"""
        self.assertFalse(is_eoc("EOC!"))
        self.assertFalse(is_eoc("EOC."))
        self.assertFalse(is_eoc("EOCx"))
        self.assertFalse(is_eoc("xEOC"))
        self.assertFalse(is_eoc("OK"))
        self.assertFalse(is_eoc(""))
        self.assertFalse(is_eoc("END"))
        self.assertFalse(is_eoc("FINISH"))

        # These cases ARE detected as EOC by the actual implementation
        # because strip() removes whitespace and upper() converts to uppercase
        self.assertTrue(is_eoc("EOC "))  # strip() removes trailing space
        self.assertTrue(is_eoc(" EOC"))  # strip() removes leading space
        self.assertTrue(is_eoc("eoc"))  # upper() converts to "EOC"

    def test_is_eoc_case_insensitive(self):
        """Test that EOC detection is case insensitive"""
        self.assertTrue(is_eoc("eoc"))
        self.assertTrue(is_eoc("Eoc"))
        self.assertTrue(is_eoc("eOc"))
        self.assertTrue(is_eoc("EOc"))
        self.assertTrue(is_eoc("eOC"))
        self.assertTrue(is_eoc("EoC"))
        self.assertTrue(is_eoc("eoc"))

    def test_is_eoc_edge_cases(self):
        """Test EOC detection edge cases"""
        # Very long strings
        long_eoc = "EOC" + " " * 1000
        self.assertTrue(is_eoc(long_eoc))

        # Strings with whitespace that get stripped
        self.assertTrue(is_eoc("EOC\t\n\r"))

        # Strings with control characters - strip() only removes whitespace, not control chars
        # So "EOC\x00\x01\x02" becomes "EOC\x00\x01\x02" after strip, which is not "EOC"
        self.assertFalse(is_eoc("EOC\x00\x01\x02"))

        # Unicode strings
        self.assertTrue(is_eoc("EOC"))
        self.assertTrue(is_eoc("EOC\u00a0"))  # non-breaking space gets stripped

    def test_accumulate_dump_lines_basic(self):
        """Test basic line accumulation"""
        lines = [
            "0: 128,64 255",
            "1: 130,66 240",
            "EOC",
            "TRAILING IGNORED",
        ]
        text = accumulate_dump_lines(lines)

        self.assertIn("0: 128,64 255", text)
        self.assertIn("1: 130,66 240", text)
        self.assertNotIn("EOC", text)
        self.assertNotIn("TRAILING IGNORED", text)

    def test_accumulate_dump_lines_empty(self):
        """Test line accumulation with empty input"""
        # Empty list
        text = accumulate_dump_lines([])
        self.assertEqual(text, "")

        # List with only EOC
        text = accumulate_dump_lines(["EOC"])
        self.assertEqual(text, "")

        # List with only whitespace (these are not EOC, so they get included)
        text = accumulate_dump_lines([" ", "\t", "\n"])
        # The actual implementation joins with \n, so we get " \n\t\n\n"
        self.assertEqual(text, " \n\t\n\n")

    def test_accumulate_dump_lines_no_eoc(self):
        """Test line accumulation when no EOC is found"""
        lines = [
            "0: 128,64 255",
            "1: 130,66 240",
            "2: 132,68 225",
        ]
        text = accumulate_dump_lines(lines)

        self.assertIn("0: 128,64 255", text)
        self.assertIn("1: 130,66 240", text)
        self.assertIn("2: 132,68 225", text)
        self.assertEqual(text, "0: 128,64 255\n1: 130,66 240\n2: 132,68 225")

    def test_accumulate_dump_lines_eoc_first(self):
        """Test line accumulation when EOC is the first line"""
        lines = ["EOC", "0: 128,64 255", "1: 130,66 240"]
        text = accumulate_dump_lines(lines)

        self.assertEqual(text, "")
        self.assertNotIn("0: 128,64 255", text)
        self.assertNotIn("1: 130,66 240", text)

    def test_accumulate_dump_lines_eoc_middle(self):
        """Test line accumulation when EOC is in the middle"""
        lines = [
            "0: 128,64 255",
            "EOC",
            "1: 130,66 240",
            "2: 132,68 225",
        ]
        text = accumulate_dump_lines(lines)

        self.assertIn("0: 128,64 255", text)
        self.assertNotIn("EOC", text)
        self.assertNotIn("1: 130,66 240", text)
        self.assertNotIn("2: 132,68 225", text)
        self.assertEqual(text, "0: 128,64 255")

    def test_accumulate_dump_lines_eoc_last(self):
        """Test line accumulation when EOC is the last line"""
        lines = [
            "0: 128,64 255",
            "1: 130,66 240",
            "2: 132,68 225",
            "EOC",
        ]
        text = accumulate_dump_lines(lines)

        self.assertIn("0: 128,64 255", text)
        self.assertIn("1: 130,66 240", text)
        self.assertIn("2: 132,68 225", text)
        self.assertNotIn("EOC", text)
        self.assertEqual(text, "0: 128,64 255\n1: 130,66 240\n2: 132,68 225")

    def test_accumulate_dump_lines_case_insensitive_eoc(self):
        """Test that EOC detection is case insensitive in accumulation"""
        lines = [
            "0: 128,64 255",
            "eoc",  # lowercase
            "1: 130,66 240",
        ]
        text = accumulate_dump_lines(lines)

        self.assertIn("0: 128,64 255", text)
        self.assertNotIn("eoc", text)
        self.assertNotIn("1: 130,66 240", text)
        self.assertEqual(text, "0: 128,64 255")

    def test_accumulate_dump_lines_whitespace_eoc(self):
        """Test EOC detection with whitespace in accumulation"""
        lines = [
            "0: 128,64 255",
            " EOC ",  # with whitespace
            "1: 130,66 240",
        ]
        text = accumulate_dump_lines(lines)

        self.assertIn("0: 128,64 255", text)
        self.assertNotIn(" EOC ", text)
        self.assertNotIn("1: 130,66 240", text)
        self.assertEqual(text, "0: 128,64 255")

    def test_accumulate_dump_lines_single_line(self):
        """Test line accumulation with single line input"""
        # Single line, no EOC
        text = accumulate_dump_lines(["0: 128,64 255"])
        self.assertEqual(text, "0: 128,64 255")

        # Single line, with EOC
        text = accumulate_dump_lines(["EOC"])
        self.assertEqual(text, "")

        # Single line, EOC with content
        text = accumulate_dump_lines(["EOC content"])
        self.assertEqual(text, "EOC content")

    def test_accumulate_dump_lines_iterable_types(self):
        """Test that accumulate_dump_lines works with different iterable types"""
        # List
        lines_list = ["0: 128,64 255", "1: 130,66 240", "EOC"]
        text_list = accumulate_dump_lines(lines_list)

        # Tuple
        lines_tuple = ("0: 128,64 255", "1: 130,66 240", "EOC")
        text_tuple = accumulate_dump_lines(lines_tuple)

        # Generator
        lines_gen = (line for line in ["0: 128,64 255", "1: 130,66 240", "EOC"])
        text_gen = accumulate_dump_lines(lines_gen)

        # All should produce the same result
        expected = "0: 128,64 255\n1: 130,66 240"
        self.assertEqual(text_list, expected)
        self.assertEqual(text_tuple, expected)
        self.assertEqual(text_gen, expected)

    def test_accumulate_dump_lines_newline_handling(self):
        """Test that newlines are properly handled in accumulation"""
        lines = [
            "0: 128,64 255",
            "1: 130,66 240",
            "EOC",
        ]
        text = accumulate_dump_lines(lines)

        # Should have newlines between lines
        self.assertIn("\n", text)
        self.assertEqual(text.count("\n"), 1)  # One newline between two lines

        # Check the exact format
        expected = "0: 128,64 255\n1: 130,66 240"
        self.assertEqual(text, expected)

    def test_parse_dump_text_mock(self):
        """Test parse_dump_text with mocked BufferData"""
        # Mock the BufferData class
        with patch("serialio.parser.BufferData") as mock_buffer_data:
            mock_instance = Mock()
            mock_buffer_data.from_dump_response.return_value = mock_instance

            # Test with some dump text
            dump_text = "0: 128,64 255\n1: 130,66 240"
            result = parse_dump_text(dump_text)

            # Should call BufferData.from_dump_response with the text
            mock_buffer_data.from_dump_response.assert_called_once_with(dump_text)

            # Should return the mock instance
            self.assertEqual(result, mock_instance)

    def test_parse_dump_text_empty(self):
        """Test parse_dump_text with empty input"""
        with patch("serialio.parser.BufferData") as mock_buffer_data:
            mock_instance = Mock()
            mock_buffer_data.from_dump_response.return_value = mock_instance

            result = parse_dump_text("")

            mock_buffer_data.from_dump_response.assert_called_once_with("")
            self.assertEqual(result, mock_instance)

    def test_parse_dump_text_single_line(self):
        """Test parse_dump_text with single line input"""
        with patch("serialio.parser.BufferData") as mock_buffer_data:
            mock_instance = Mock()
            mock_buffer_data.from_dump_response.return_value = mock_instance

            result = parse_dump_text("0: 128,64 255")

            mock_buffer_data.from_dump_response.assert_called_once_with("0: 128,64 255")
            self.assertEqual(result, mock_instance)

    def test_parse_dump_text_multiline(self):
        """Test parse_dump_text with multiline input"""
        with patch("serialio.parser.BufferData") as mock_buffer_data:
            mock_instance = Mock()
            mock_buffer_data.from_dump_response.return_value = mock_instance

            dump_text = "0: 128,64 255\n1: 130,66 240\n2: 132,68 225"
            result = parse_dump_text(dump_text)

            mock_buffer_data.from_dump_response.assert_called_once_with(dump_text)
            self.assertEqual(result, mock_instance)

    def test_parse_dump_text_with_whitespace(self):
        """Test parse_dump_text with whitespace in input"""
        with patch("serialio.parser.BufferData") as mock_buffer_data:
            mock_instance = Mock()
            mock_buffer_data.from_dump_response.return_value = mock_instance

            dump_text = "  0: 128,64 255  \n  1: 130,66 240  "
            result = parse_dump_text(dump_text)

            mock_buffer_data.from_dump_response.assert_called_once_with(dump_text)
            self.assertEqual(result, mock_instance)

    def test_parse_dump_text_integration(self):
        """Test integration between accumulate_dump_lines and parse_dump_text"""
        with patch("serialio.parser.BufferData") as mock_buffer_data:
            mock_instance = Mock()
            mock_buffer_data.from_dump_response.return_value = mock_instance

            # Simulate a full dump response
            lines = [
                "0: 128,64 255",
                "1: 130,66 240",
                "2: 132,68 225",
                "EOC",
                "IGNORED AFTER EOC",
            ]

            # Accumulate lines
            text = accumulate_dump_lines(lines)

            # Parse the accumulated text
            result = parse_dump_text(text)

            # Verify the flow
            expected_text = "0: 128,64 255\n1: 130,66 240\n2: 132,68 225"
            self.assertEqual(text, expected_text)

            mock_buffer_data.from_dump_response.assert_called_once_with(expected_text)
            self.assertEqual(result, mock_instance)


if __name__ == "__main__":
    unittest.main()
