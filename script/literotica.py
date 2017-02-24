#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# literotica.py - 
#
# Created by skywind on 2017/02/24
# Last change: 2017/02/24 18:10:44
#
#======================================================================
import sys, time, re
import shell


#----------------------------------------------------------------------
# Literotica
#----------------------------------------------------------------------
class Literotica (object):

	def __init__ (self, url):
		self._url = url
		p1 = url.find('?page=')
		if p1 >= 0:
			self.url = url[:p1]
		self._content = shell.request_safe(self._url)
		self._extract()

	def _extract (self):
		content = self._content
		p1 = content.find('<select name="page">')
		if p1 < 0:
			return -1
		p2 = content.find('</select>', p1)
		if p2 < 0:
			return -1
		count = 0
		self._index = []
		while p1 < p2:
			p1 = content.find('<option value="', p1)
			if p1 < 0 or p1 >= p2:
				break
			p1 = p1 + 15
			pp = content.find('"', p1)
			if p1 < 0:
				break
			text = content[p1:pp].strip()
			p1 = pp
			self._index.append(text)
		p1 = content.find('<div class="b-story-header">')
		self._intro = ''
		if p1 < 0:
			return -2
		p1 = content.find('<h1>', p1)
		if p1 < 0:
			return -2
		p2 = content.find('</h1>', p1)
		if p2 < 0:
			return -2
		title = content[p1 + 4:p2]
		self._intro = shell.html2text(title).strip() + '\n\n\n'
		return 0

	def __len__ (self):
		return len(self._index)

	def read_chapter (self, n):
		url = self._url + '?page=' + self._index[n]
		return shell.request_safe(url)

	def chapter (self, n):
		content = self.read_chapter(n)
		p1 = content.find('<div class="b-story-body-x')
		if p1 < 0:
			print 'suck1'
			return None
		p1 = content.find('>', p1)
		if p1 < 0:
			print 'suck2'
			return None
		p2 = content.find('</div>', p1)
		html = content[p1 + 1:p2]
		return shell.html2text(html)

	def download (self, filename):
		part = []
		size = len(self._index)
		for i in xrange(size):
			print '[%d/%d] %s'%(i + 1, size, self._index[i])
			text = 'PAGE %d\n\n'%(i + 1)
			text += self.chapter(i) + '\n\n'
			part.append(text)
		text = '\n'.join(part)
		self._whole = self._intro + text
		open(filename, 'w').write(self._whole)
		print 'saved:', filename
		return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':
	url = 'https://www.literotica.com/s/words-on-skin'
	def test1():
		lit = Literotica(url)
		print lit.chapter(0)
		return 0
	def test2():
		lit = Literotica(url)
		lit.download('words-on-skin.txt')
		return 0
	test2()


