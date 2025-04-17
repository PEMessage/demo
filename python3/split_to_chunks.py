#!/usr/bin/env python3

import unittest
import random
from typing import List, Any


def split_to_chunks(max_chunk_size, indexable):
    chunks = [indexable[i:i+max_chunk_size] for i in range(0, len(indexable), max_chunk_size)]
    return chunks



class TestSplitToChunks(unittest.TestCase):
    def test_empty_input(self):
        self.assertEqual(split_to_chunks(3, []), [])

    def test_single_chunk_exact_size(self):
        self.assertEqual(split_to_chunks(3, [1, 2, 3]), [[1, 2, 3]])

    def test_single_chunk_smaller_than_max(self):
        self.assertEqual(split_to_chunks(5, [1, 2, 3]), [[1, 2, 3]])

    def test_multiple_chunks_exact_sizes(self):
        self.assertEqual(split_to_chunks(2, [1, 2, 3, 4]), [[1, 2], [3, 4]])

    def test_multiple_chunks_with_remainder(self):
        self.assertEqual(split_to_chunks(2, [1, 2, 3, 4, 5]), [[1, 2], [3, 4], [5]])

    def test_chunk_size_larger_than_input(self):
        self.assertEqual(split_to_chunks(10, [1, 2, 3]), [[1, 2, 3]])

    def test_chunk_size_of_one(self):
        self.assertEqual(split_to_chunks(1, ['a', 'b', 'c']), [['a'], ['b'], ['c']])

    def test_string_input(self):
        self.assertEqual(split_to_chunks(3, "abcdefg"), ["abc", "def", "g"])

    def test_tuple_input(self):
        self.assertEqual(split_to_chunks(2, (1, 2, 3, 4)), [(1, 2), (3, 4)])

    def test_random_binary_data_with_random_chunk_sizes(self):
        for _ in range(100):  # Run 100 random tests
            # Generate random binary data (0-255 bytes)
            original_data = [ 
                    random.randint(0,16).to_bytes(1, byteorder='little')
                    for i in range(random.randint(0, 1024))
                    ]
            original_data = b''.join(original_data)

            # Generate random chunk size (1 to data_size + 10 to test various cases)
            chunk_size = random.randint(1, 1024)

            # Split into chunks
            chunks = split_to_chunks(chunk_size, original_data)

            # Rejoin the chunks
            rejoined_data = b''.join(chunks)

            # Verify the rejoined data matches original
            self.assertEqual(rejoined_data, original_data)

if __name__ == '__main__':
    unittest.main()
