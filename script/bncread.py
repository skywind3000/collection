#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# bncread.py - 
#
# Created by skywind on 2017/03/27
# Last change: 2017/03/27 12:26:48
#
#======================================================================
import sys
import os
import time
import csv

sys.path.append('e:/lab/web/common')
import ascmini



#----------------------------------------------------------------------
# WordStem
#----------------------------------------------------------------------
class WordStem (object):
	def __init__ (self, stem):
		self.stem = stem
		self.count = 0
		self.words = {}
	def add (self, c5, word, n = 1):
		if c5 and word:
			term = (c5, word)
			if not term in self.words:
				self.words[term] = n
			else:
				self.words[term] += n
			self.count += 1
		return True
	def dump (self):
		output = []
		for term in self.words:
			c5, word = term
			output.append((c5, word, self.words[term]))
		output.sort(key = lambda x: x[2], reverse = True)
		return output


#----------------------------------------------------------------------
# 
#----------------------------------------------------------------------
class WordStatistic (object):
	def __init__ (self):
		self.words = {}
	
	def load (self, filename):
		rows = ascmini.csv_load(filename)
		if not rows:
			return False
		words = self.words
		for row in rows:
			if not row:
				continue
			if len(row) < 4:
				continue
			stem = row[0].lower()
			c5 = row[1]
			word = row[3]
			if not word:
				continue
			if not stem in words:
				ws = WordStem(stem)
				words[stem] = ws
			else:
				ws = words[stem]
			ws.add(c5, word.lower())
		rows = None
		return True

	def save (self, filename):
		words = [ self.words[s] for s in self.words ]
		words.sort(key = lambda x: x.count, reverse = True)
		output = []
		for stem in words:
			row = [stem.stem, stem.count]
			for record in stem.dump():
				output.append(row + list(record))
		ascmini.csv_save(output, filename)
		return True

	def __len__ (self):
		return len(self.words)


#----------------------------------------------------------------------
# 
#----------------------------------------------------------------------
class BncReader (object):

	def __init__ (self):
		pass

	def read_bnc_text (self, filename):
		# import xml.etree.ElementTree as ET
		import lxml.etree
		root = lxml.etree.parse(filename)
		output = []
		for p in root.iterfind('.//w'):
			hw = p.attrib.get('hw', '')
			c5 = p.attrib.get('c5', '')
			pos = p.attrib.get('pos', '')
			word = p.text and p.text.strip() or ''
			output.append((hw, c5, pos, word))
		return output

	def search_xmls (self, name):
		import fnmatch
		matches = []
		for root, dirnames, filenames in os.walk(name):
			for filename in fnmatch.filter(filenames, '*.xml'):
				matches.append(os.path.join(root, filename))
		return matches

	def dump_bnc (self):
		lines = 0
		index = 0
		for cwd in os.listdir('.'):
			if len(cwd) != 1 or (not cwd.isupper()):
				continue
			xmls = self.search_xmls(cwd)
			writer = None
			count = 0
			fp = None
			for xmlname in xmls:
				rows = self.read_bnc_text(xmlname)
				for row in rows:
					if writer is None:
						index += 1
						name = 'output/bnc-%s-%d.csv'%(cwd.lower(), index)
						fp = open(name, 'w')
						writer = csv.writer(fp)
						print 'swith to', name
					writer.writerow([ n.encode('utf-8') for n in row ])
					lines += 1
					if lines % 2000000 == 0:
						if fp: fp.close()
						fp = None
						writer = None
				count += 1
				print '%s: %d/%d'%(name, count, len(xmls))
		return True

	def statistic (self):
		ws = WordStatistic()
		for filename in ascmini.posix.find_files('words-bnc', '*.csv'):
			print filename, len(ws)
			ws.load(filename)
		ws.save('output.txt')


#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------
if __name__ == '__main__':
	def test1():
		bnc = BncReader()
		# bnc.dump_bnc()
		return 0
	def test2():
		bnc = BncReader()
		bnc.statistic()
		return 0
	test2()



