#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# fiction_mangg.py - fiction download from mangg.com
#
# Created by skywind on 2016/11/29
# Last change: 2016/11/29 17:33:26
#
#======================================================================
import shell
import re



#----------------------------------------------------------------------
# FictionMangg
#----------------------------------------------------------------------
class FictionMangg (object):

	def __init__ (self, url):
		self._url = url
		if 1:
			self._content = shell.request_safe(url).decode('utf-8', 'ignore')
			self._content = self._content.encode('gbk', 'ignore')
			self._content = self._content.decode('gbk', 'ignore')
		else:
			self._content = open('mangg.txt', 'r').read().decode('gbk')
		content = self._content
		p = re.compile(r'<dd><a href="([^\.]*).html">([^<]*)</a>')
		result = [[0, x[0], x[1]] for x in p.findall(content)]
		for m in result:
			m[0] = int(re.findall('/id\d*/(\d*)', m[1])[0])
		result.sort()
		self._index = [(m[1] + '.html', m[2]) for m in result]
		intro = ''
		p1 = content.find('<div id="intro">')
		p2 = content.find('</div>', p1)
		if p1 >= 0 and p2 >= 0:
			intro = content[p1:p2]
			intro = shell.html2text(intro) + '\n\n'
		self._intro = intro

	def __len__ (self):
		return len(self._index)

	def __getitem__ (self, n):
		return self._index[n][1]

	def read_chapter (self, n):
		url = 'http://www.mangg.com/' + self._index[n][0]
		text = shell.request_safe(url).decode('utf-8', 'ignore')
		text = text.encode('gbk', 'ignore').decode('gbk', 'ignore')
		return text

	def chapter (self, n):
		content = self.read_chapter(n)
		p = re.compile(r'<div id="content">(.*)</div>')
		result = p.findall(content)
		html = result[0]
		return shell.html2text(html)

	def download (self, filename):
		part = []
		size = len(self._index)
		for i in xrange(size):
			text = self._index[i][1] + '\n\n'
			print '[%d/%d] %s'%(i + 1, size, self._index[i][1])
			text+= self.chapter(i) + '\n\n'
			part.append(text)
		text = '\n'.join(part)
		self._whole = self._intro + text
		open(filename, 'w').write(self._whole.encode('utf-8'))
		print 'saved:', filename
		return 0



#----------------------------------------------------------------------
# simple download interface
#----------------------------------------------------------------------
def download (url, filename):
	mangg = FictionMangg(url)
	mangg.download(filename)
	return 0


#----------------------------------------------------------------------
# main program
#----------------------------------------------------------------------
if __name__ == '__main__':
	url = 'http://www.mangg.com/id31957/'

	def test1():
		mangg = FictionMangg(url)
		for m, n in mangg._index:
			print m, n
		#print mangg.chapter(0)
		return 0

	def test2():
		download(url, 'e:/fiction2.txt')
		return 0

	test2()


