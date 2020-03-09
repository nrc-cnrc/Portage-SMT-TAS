#!/usr/bin/env python

from __future__ import print_function

import unittest
import os
import numpy as np
import numpy.random as rng
import subprocess
from mock import patch
from mock import MagicMock
from mock import Mock
from mock import mock_open
from mock import call
from nnjm_data_iterator import openShuffleNoTempFile
from nnjm_data_iterator import InfiniteIterator
from nnjm_data_iterator import DataIterator
from nnjm_data_iterator import HardElideDataIterator
from nnjm_data_iterator import SoftElideDataIterator


class TestOpenShuffleNoTempFile(unittest.TestCase):
   def __init__(self, *args, **kwargs):
      super(TestOpenShuffleNoTempFile, self).__init__(*args, **kwargs)
      self.data_filename = os.path.join('tests', 'sample.data')


   def test_01_default_usage(self):
      with open(self.data_filename, 'r') as f:
         data = f.readlines()
      with openShuffleNoTempFile(self.data_filename, 'r') as f:
         shuffled_data = f.readlines()

      self.assertEqual(len(data), len(shuffled_data), 'There should the same number of samples.')

      data = np.asarray(data)
      shuffled_data = np.asarray(shuffled_data)

      self.assertTrue(np.all(data.shape == shuffled_data.shape), 'The data should retain its shape.')
      self.assertFalse(np.all(data == shuffled_data), 'The samples are not supposed to be in the same order.')
      self.assertTrue(np.all(data.sort() == shuffled_data.sort()), 'We should have exactly the same samples.')


   @patch('nnjm_data_iterator.Popen', autospec=True)
   def test_02_terminate_should_be_called(self, mocked_popen):
      with open(self.data_filename, 'r') as g:
         mocked_popen.return_value.stdout = g
         with openShuffleNoTempFile(self.data_filename, 'r') as f:
            _ = f.readlines()
      #print(mocked_popen.mock_calls)
      #print(mocked_popen.return_value.terminate.call_count)
      self.assertTrue(mocked_popen.return_value.terminate.call_count == 1, 'A call to terminate should have happened.')
      mocked_popen.return_value.terminate.assert_called_once_with()
      expected_calls = [ call('gzip -cqdf {} | shuf 2> /dev/null'.format(self.data_filename), shell=True, stdout=-1),
             call().terminate() ]
      # From mock==1.0.1 to mock==2.0.0 the assert_has_calls has changed
      # behavior and we can no longer use it for our purpose.
      # See: https://github.com/testing-cabal/mock/issues/353
      #from pudb import set_trace; set_trace()
      #mocked_popen.assert_has_calls('gzip -cqdf {} | shuf 2> /dev/null'.format(self.data_filename), shell=True, stdout=-1)
      for e, c in zip(expected_calls, mocked_popen.mock_calls):
         self.assertEqual(e, c)



class TestInfiniteIterator(unittest.TestCase):
   def __init__(self, *args, **kwargs):
      super(TestInfiniteIterator, self).__init__(*args, **kwargs)
      self.data_filename = os.path.join('tests', 'sample.data')


   def test_01_default_usage(self):
      iteratable = InfiniteIterator(self.data_filename)
      # The file has 100 samples, thus using 150 will force a reopen.
      for _ in range(150):
         sample = next(iteratable)
         self.assertEqual(len(sample.split()), 17, 'Invalid format')


   @patch('__builtin__.open', spec=open)
   def test_02_file_reopened(self, my_mock_open):
      def setupMock():
         data_file_1 = MagicMock(name='samples', spec=file)
         data_file_1.__enter__.return_value = data_file_1
         data_file_1.__iter__.return_value = iter(['a', 'b'])

         data_file_2 = MagicMock(name='samples', spec=file)
         data_file_2.__enter__.return_value = data_file_2
         data_file_2.__iter__.return_value = iter(['a', 'b'])

         my_mock_open.side_effect = (data_file_1, data_file_2)
         return data_file_1, data_file_2

      d1, d2 = setupMock()
      iteratable = InfiniteIterator(self.data_filename, opener=my_mock_open)
      # The file has 2 samples, thus using 3 will force a reopen.
      for _ in range(3):
         sample = next(iteratable)

      self.assertEqual(my_mock_open.call_count, 2, 'We should have opened the file twice.')
      expected_calls = [ call('tests/sample.data', 'r') ] * 2
      my_mock_open.assert_has_calls(expected_calls)

      #print(d1.mock_calls)
      #print(d2.mock_calls)   # We are not closing the file?!



class TestDataIterator(unittest.TestCase):
   def __init__(self, *args, **kwargs):
      super(TestDataIterator, self).__init__(*args, **kwargs)
      self.data_filename = os.path.join('tests', 'sample.data')


   def test_01_default_usage(self):
      # The total isn't 100 samples since the first sample is used to validate the format.
      expected_block_sizes = (30, 30, 30, 9)
      with open(self.data_filename, 'r') as f:
         iteratable = DataIterator(f, block_size = 30)
         for i, (X, y) in enumerate(iteratable):
            self.assertEqual(X.shape[0], y.shape[0], 'There should be as many samples as labels.')
            self.assertEqual(X.shape[0], expected_block_sizes[i], 'Invalid block size {}'.format(i))
         self.assertEqual(i+1, len(expected_block_sizes), 'Not the expected block count')


   def test_02_changing_block_size(self):
      with open(self.data_filename, 'r') as f:
         iteratable = DataIterator(f, block_size = 30)

         X, y = next(iteratable)
         self.assertEqual(X.shape[0], y.shape[0], 'There should be as many samples as labels.')
         self.assertEqual(X.shape[0], 30, 'Invalid block size {}'.format(0))

         iteratable.block_size = 20
         X, y = next(iteratable)
         self.assertEqual(X.shape[0], y.shape[0], 'There should be as many samples as labels.')
         self.assertEqual(X.shape[0], 20, 'Invalid block size {}'.format(1))


   def test_03_block_size_too_big(self):
      with open(self.data_filename, 'r') as f:
         iteratable = DataIterator(f, block_size = 130)
         X, y = next(iteratable)
         self.assertEqual(X.shape[0], 100-1, 'The block size should be the maximum number of available samples.')

         # Trying to read again should raise a StopIteration since DataIterator is an iterator.
         with self.assertRaises(StopIteration) as cm:
            next(iteratable)


   def test_04_with_infinite_shuffle(self):
      """
      This is our use case scenario that we are mainly interested in.
      """
      iteratable = DataIterator(InfiniteIterator(self.data_filename, opener=openShuffleNoTempFile),
                                block_size = 33)
      samples = []
      # Asking for 6 blocks means that we will trigger a reopen since there 3 blocks per file.
      for i in range(6):
         samples.append(next(iteratable))

      # The data should be shuffle each time we go through the file thus no block should be the same.
      for i in range(3):
         self.assertFalse(np.all(samples[i][0] == samples[3+i][0]))

      # Technically, the first 3 sample blocks should contain the same samples
      # as the last three blocks but since we are losing a sample to validate
      # the format, I'm not quite sure how to test the equality.



class TestHardElideDataIterator(unittest.TestCase):
   def __init__(self, *args, **kwargs):
      super(TestHardElideDataIterator, self).__init__(*args, **kwargs)
      self.data = [
               '6 104 11 193 388 453 210 10 3 3 3 / 16 252 14 / 333',
               '206 7 205 339 11 6 207 380 269 309 10 / 329 23 133 / 37',
               '339 11 6 207 380 269 309 10 3 3 3 / 133 37 12 / 334',
               '5 12 396 7 285 380 269 5 174 269 5 / 156 324 16 / 226',
               '2 2 2 2 6 308 436 275 296 294 483 / 2 2 15 / 364'
            ]


   def testNoElide(self):
      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      X_expected = [
            [206,   7, 205, 339,  11,   6, 207, 380, 269, 309,  10, 329,  23, 133],
            [339,  11,   6, 207, 380, 269, 309,  10,   3,   3,   3, 133,  37,  12],
            [  5,  12, 396,   7, 285, 380, 269,   5, 174, 269,   5, 156, 324,  16],
            [  2,   2,   2,   2,   6, 308, 436, 275, 296, 294, 483,   2,   2,  15]]
      y_expected = [ 37, 334, 226, 364 ]

      iteratable = HardElideDataIterator(iteratable, thist_size = 3, thist_elide_size = 0)
      X, y = iteratable.next()
      self.assertTrue(np.all(X == X_expected))
      self.assertTrue(np.all(y == y_expected))


   def testMaxElide(self):
      X_expected = [
            [206,   7, 205, 339,  11,   6, 207, 380, 269, 309,  10, 0, 0, 0],
            [339,  11,   6, 207, 380, 269, 309,  10,   3,   3,   3, 0, 0, 0],
            [  5,  12, 396,   7, 285, 380, 269,   5, 174, 269,   5, 0, 0, 0],
            [  2,   2,   2,   2,   6, 308, 436, 275, 296, 294, 483, 0, 0, 0]]
      y_expected = [ 37, 334, 226, 364 ]

      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      iteratable = HardElideDataIterator(iteratable, thist_size = 3, thist_elide_size = 3)
      X, y = iteratable.next()
      self.assertTrue(np.all(X == X_expected))
      self.assertTrue(np.all(y == y_expected))


   def testTwoElide(self):
      X_expected = [
            [206,   7, 205, 339,  11,   6, 207, 380, 269, 309,  10, 0, 0, 133],
            [339,  11,   6, 207, 380, 269, 309,  10,   3,   3,   3, 0, 0,  12],
            [  5,  12, 396,   7, 285, 380, 269,   5, 174, 269,   5, 0, 0,  16],
            [  2,   2,   2,   2,   6, 308, 436, 275, 296, 294, 483, 0, 0,  15]]
      y_expected = [ 37, 334, 226, 364 ]

      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      iteratable = HardElideDataIterator(iteratable, thist_size = 3, thist_elide_size = 2)
      X, y = iteratable.next()
      self.assertTrue(np.all(X == X_expected))
      self.assertTrue(np.all(y == y_expected))


   def testOneElide(self):
      X_expected = [
            [206,   7, 205, 339,  11,   6, 207, 380, 269, 309,  10, 0,  23, 133],
            [339,  11,   6, 207, 380, 269, 309,  10,   3,   3,   3, 0,  37,  12],
            [  5,  12, 396,   7, 285, 380, 269,   5, 174, 269,   5, 0, 324,  16],
            [  2,   2,   2,   2,   6, 308, 436, 275, 296, 294, 483, 0,   2,  15]]
      y_expected = [ 37, 334, 226, 364 ]

      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      iteratable = HardElideDataIterator(iteratable, thist_size = 3, thist_elide_size = 1)
      X, y = iteratable.next()
      self.assertTrue(np.all(X == X_expected))
      self.assertTrue(np.all(y == y_expected))


   def testTooMuchElide(self):
      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      with self.assertRaises(Exception) as context:
         iteratable = HardElideDataIterator(iteratable, thist_size = 3, thist_elide_size = 4)
      self.assertTrue('Error: You cannot elide more token than you have.' in context.exception)


   def testWrong_thist_size(self):
      iteratable = DataIterator(
            iter(self.data),
            block_size = 2,
            swin_size = 11,
            thist_size = 3)
      with self.assertRaises(Exception) as context:
         iteratable = HardElideDataIterator(iteratable, thist_size = 4, thist_elide_size = 0)
      self.assertTrue('Error: Incompatible thist_sizes.' in context.exception)



class TestHardElideDataIterator(unittest.TestCase):
   def __init__(self, *args, **kwargs):
      super(TestHardElideDataIterator, self).__init__(*args, **kwargs)
      self.data = [
               '6 104 11 193 388 453 210 10 3 3 3 / 16 252 14 / 333',
               '206 7 205 339 11 6 207 380 269 309 10 / 329 23 133 / 37',
               '339 11 6 207 380 269 309 10 3 3 3 / 133 37 12 / 334',
               '5 12 396 7 285 380 269 5 174 269 5 / 156 324 16 / 226',
               '2 2 2 2 6 308 436 275 296 294 483 / 2 2 15 / 364'
            ]


   def testTooMuchElide(self):
      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      with self.assertRaises(Exception) as context:
         iteratable = SoftElideDataIterator(iteratable, thist_size = 3, max_elide = 4, elide_prob = 1.0)
      self.assertTrue('You cannot elide more of the history than you have.' in context.exception)


   def testTooMuchElide(self):
      rng.seed(1382)
      X_expected = [
            [206,  7, 205, 339,  11,   6, 207, 380, 269, 309,  10,   0,   0, 133],
            [339, 11,   6, 207, 380, 269, 309,  10,   3,   3,   3,   0,  37, 12],
            [  5, 12, 396,   7, 285, 380, 269,   5, 174, 269,   5, 156, 324, 16],
            [  2,  2,   2,   2,   6, 308, 436, 275, 296, 294, 483,   2,   2, 15]]
      y_expected = [ 37, 334, 226, 364 ]

      iteratable = DataIterator(
            iter(self.data),
            block_size = 4,
            swin_size = 11,
            thist_size = 3)
      iteratable = SoftElideDataIterator(iteratable, thist_size = 3, max_elide = 2, elide_prob = 0.5)
      X, y = iteratable.next()
      self.assertTrue(np.all(X == X_expected))
      self.assertTrue(np.all(y == y_expected))










if __name__ == '__main__':
   unittest.main()
