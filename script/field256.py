#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# field256.py - 
#
# Created by skywind on 2018/05/14
# Last Modified: 2018/05/14 23:48:17
#
#======================================================================
from __future__ import print_function
import sys
import os


#----------------------------------------------------------------------
# GF(2^8)
#----------------------------------------------------------------------
class GF256 (object):

	def __init__ (self):
		self.table = [0] * 256
		self.inverse = [0] * 256
		self.arc_table = [0] * 256
		self._init_table()
		self._init_arc()
		self._init_inverse()

	def _init_table (self):
		self.table[0] = 1
		for i in range(1, 255):
			self.table[i] = (self.table[i - 1] << 1) ^ self.table[i - 1]
			if (self.table[i] & 0x100) != 0:
				self.table[i] ^= 0x11B
		return 0

	def _init_arc (self):
		for i in range(255):
			self.arc_table[self.table[i]] = i
		return 0

	def _init_inverse (self):
		for i in range(1, 256):
			k = self.arc_table[i]
			k = (255 - k) % 255
			self.inverse[i] = self.table[k]
		return 0

	def add (self, x, y):
		return x ^ y

	def sub (self, x, y):
		return x ^ y

	def mul (self, x, y):
		if x == 0 or y == 0:
			return 0
		return self.table[(self.arc_table[x] + self.arc_table[y]) % 255]

	def div (self, x, y):
		return self.mul(x, self.inverse[y])

	def test (self):
		for a in range(256):
			for b in range(256):
				k = self.mul(a, b)
				assert (k >= 0 and k < 256)
				# print(a, b, k)
				if a > 0 and b > 0:
					assert self.div(k, a) == b
					assert self.div(k, b) == a
		print('pass')
		return 0


#----------------------------------------------------------------------
# test GF(2^8)
#----------------------------------------------------------------------
if __name__ == '__main__':
	gf = GF256()
	gf.test()

