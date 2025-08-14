import unittest
from serialio.command_builder import cmd_write, cmd_dump, cmd_swap, cmd_clear, cmd_size


class TestCommandBuilder(unittest.TestCase):
    def test_write(self):
        self.assertEqual(cmd_write(0, 1, 2, 3), "WRITE 0 1 2 3 INACTIVE")
        self.assertEqual(
            cmd_write(255, 255, 255, 255), "WRITE 255 255 255 255 INACTIVE"
        )

    def test_dump_clear_swap_size(self):
        self.assertEqual(cmd_dump(), "DUMP INACTIVE")
        self.assertEqual(cmd_clear(), "CLEAR INACTIVE")
        self.assertEqual(cmd_swap(), "SWAP")
        self.assertEqual(cmd_size(5), "SIZE 5 INACTIVE")

    def test_size_bounds(self):
        with self.assertRaises(ValueError):
            cmd_size(0)
        self.assertEqual(cmd_size(1), "SIZE 1 INACTIVE")
        self.assertEqual(cmd_size(256), "SIZE 256 INACTIVE")
        with self.assertRaises(ValueError):
            cmd_size(257)

    def test_write_bounds(self):
        with self.assertRaises(ValueError):
            cmd_write(-1, 0, 0, 0)
        with self.assertRaises(ValueError):
            cmd_write(0, 256, 0, 0)
        with self.assertRaises(ValueError):
            cmd_write(0, 0, 0, 999)


if __name__ == "__main__":
    unittest.main()
