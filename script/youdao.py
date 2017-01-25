#!/usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# youdao.py - youdao dictionary database reader
#
# NOTE: 有道词典数据库读取程序
#
#======================================================================
import sys, time, struct
import os


#----------------------------------------------------------------------
# YoudaoMini - 有道词典本地库 dicten.db, dictcn.db 读取文件 - 4.4
#----------------------------------------------------------------------
class YoudaoMini (object):

	# 传入原始字典文件
	def __init__ (self, fp):
		if type(fp) in (type(''), type(u'')):
			self._content = open(fp, 'rb').read()
		else:
			self._content = fp.read()
		if self._content[:5] != '\xff\xff\xff\xff\x01':
			raise IOException('Youdao local dictionary format error')
		self._index_size = struct.unpack('I', self._content[5:9])[0]
		self._index_end = self._index_size + 5
		self._read_index()
	
	# 读取索引
	def _read_index (self):
		self._index_array = []
		self._index_dict = {}
		self._index_cache = {}
		pos = 9
		content = self._content
		while pos < self._index_end:
			size = 255 - ord(content[pos])
			text = content[pos + 1:pos + 1 + size]
			text = ''.join([ chr(255 - ord(ch)) for ch in text ])
			pos += 1 + size + 4
			locate = content[pos - 4:pos]
			c1 = 255 - ord(locate[0])
			c2 = 255 - ord(locate[1])
			c3 = 255 - ord(locate[2])
			c4 = 255 - ord(locate[3])
			locate = (c4 << 24) | (c3 << 16) | (c2 << 8) | c1
			self._index_array.append((text, locate))
			self._index_dict[text] = locate
		self._index_array.sort()
		self._index_count = len(self._index_array)
		return self._index_count
	
	# 查询单词
	def lookup (self, word):
		if type(word) in (int, long):
			return self._index_array[word][0]
		if type(word) == type(u''):
			word = word.encode('gbk')
		word = word.strip('\r\n\t ')
		locate = self._index_dict.get(word, None)
		if locate == None:
			return None
		data = self._index_cache.get(word, None)
		if data == None:
			pos = locate + self._index_end
			content = self._content
			c1 = ord(content[pos + 0])
			c2 = ord(content[pos + 1])
			size = c1 + (c2 << 8)
			c1 = 255 - ord(content[pos + 2])
			p1 = content[pos + 3:pos + 3 + c1]
			pos += 3 + c1
			c2 = 255 - ord(content[pos])
			p2 = content[pos + 1:pos + 1 + c2]
			pos += 1 + c2
			p1 = ''.join([ chr(255 - ord(ch)) for ch in p1 ])
			p2 = ''.join([ chr(255 - ord(ch)) for ch in p2 ])
			self._index_cache[word] = (p1, p2)
			data = self._index_cache[word]
		return data
	
	# 快速查询
	def __getitem__ (self, key):
		return self.lookup(key)
	
	# 是否包含
	def __contains__ (self, key):
		return self._index_dict.__contains__(key)
	
	# 取得长度
	def __len__ (self):
		return self._index_count
	
	# 读取数据
	def get (self, key, default = None):
		value = self.lookup(key)
		return value and value or default
	
	# 二分相似搜索，给一个单词，搜索出前缀类似的一批单词列表
	def match (self, word, count = 5):
		if self._index_count <= 0:
			return []
		top = 0
		bottom = self._index_count - 1
		middle = top
		index = self._index_array
		while top < bottom:
			middle = (top + bottom) >> 1
			if top == middle or bottom == middle:
				break
			text = index[middle][0]
			if word == text:
				break
			elif word < text:
				bottom = middle
			elif word > text:
				top = middle
		while index[middle][0] < word:
			middle += 1
			if middle >= self._index_count:
				break
		likely = [ tx[0] for tx in index[middle:middle + count] ]
		return likely



#----------------------------------------------------------------------
# WordFrequency - 读取词频数据库
#----------------------------------------------------------------------
class WordFrequency (object):

	def __init__ (self, fp, limit = 50000):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'r')
		import csv
		reader = csv.reader(fp)
		count = 0
		words = {}
		index = []
		wordpure = []
		for row in reader:
			if not row:
				continue
			word = row[0].strip('\r\n\t ')
			test = word.replace(' ', '').replace('\'', '')
			if not test.isalpha():
				continue
			words[word] = count
			index.append((word, count))
			wordpure.append(word)
			count += 1
			if limit > 0:
				if count >= limit:
					break
		self._words = words
		self._index = index
		self._count = len(index)
		self._wordpure = wordpure

	def __len__ (self):
		return self._count
	
	def __getitem__ (self, key):
		if type(key) in (type(''), type(u'')):
			return self._words[key]
		return self._index[key][0]
	
	def get (self, key, default = None):
		if type(key) in (type(''), type(u'')):
			return self._words.get(key, default)
		if key < 0 or key >= self._count:
			return default
		return self._index[key]
	
	def __iter__ (self):
		return self._wordpure.__iter__()

	def __contains__ (self, key):
		return self._words.__contains__(key)


#----------------------------------------------------------------------
# WordHistory - 读取历史数据
#----------------------------------------------------------------------
class WordHistory (object):

	def __init__ (self, fp):
		needclose = False
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'r')
			needclose = True
		import csv
		reader = csv.reader(fp)
		history = []
		words = {}
		remember = []
		forget = []
		for row in reader:
			if not row: continue
			if len(row) < 3: continue
			word = row[0].strip('\r\n\t ')
			try:
				flag = int(row[1])
			except:
				continue
			ts = row[2].strip('\r\n\t ')
			history.append((word, flag, ts))
			words[word] = flag
		if needclose:
			fp.close()
		for word, _, _ in history:
			flag = words[word]
			if flag == 1:
				remember.append(word)
			else:
				forget.append(word)
		self._history = history
		self._words = words
		self._count = len(words)
		self._remember = remember
		self._forget = forget
	
	def __contains__ (self, key):
		return self._words.__contains__(key)
	
	def __len__ (self):
		return self._count
	
	def __iter__ (self):
		return self._words.__iter__()
	
	def __getitem__ (self, key):
		return self._words[key]
	
	def get (self, key, default = None):
		return self._words.get(key, default)

	def forget (self):
		return [ n for n in self._forget ]
	
	def remember (self):
		return [ n for n in self._remember ]
	
	def check (self, word):
		flag = self._words.get(word, 0)
		return flag and True or False



#----------------------------------------------------------------------
# DictLMS - 查询文曲星的 LMS 字典
#----------------------------------------------------------------------
class DictLMS (object):

	def __init__ (self, fp):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'rb')
		self._content = fp.read()
		self._read_index()
	
	def _read_index (self):
		content = self._content
		unpack = struct.unpack
		self._position_dict = struct.unpack('<I', content[10:14])[0]
		self._position_look = struct.unpack('<I', content[14:18])[0]
		count = 0
		index = []
		position = self._position_look
		last = -1
		length = len(content)
		while 1:
			start = unpack('<I', content[position:position + 4])[0]
			position += 4
			if len(index) > 0:
				index[-1][2] = start - index[-1][1]
			if position >= length:
				break
			mode = ord(content[position])
			position += 1
			if mode != 0x40:
				break
			text = ''
			while 1:
				ch = content[position]
				position += 1
				if ch == '#':
					break
				text += ch
			text = text.decode('gbk', 'ignore')
			index.append([text, start, -1])
		self._index = []
		self._lookup = {}
		for word, start, size in index:
			self._index.append((word, start, size))
			self._lookup[word] = (start, size)
		self._index.sort()
		self._index_count = len(self._index)
		return 0

	def lookup (self, word):
		if type(word) == type(''):
			word = word.decode('gbk', 'ignore')
		desc = self._lookup.get(word, None)
		if desc == None:
			return None
		content = self._content[desc[0]:desc[0] + desc[1]]
		if ord(content[0]) != 0xa1 or ord(content[1]) != 0xef:
			return None
		content = content[2:]
		return content.decode('gbk', 'ignore')

	# 二分相似搜索，给一个单词，搜索出前缀类似的一批单词列表
	def match (self, word, count = 5):
		if self._index_count <= 0:
			return []
		top = 0
		bottom = self._index_count - 1
		middle = top
		index = self._index
		while top < bottom:
			middle = (top + bottom) >> 1
			if top == middle or bottom == middle:
				break
			text = index[middle][0]
			if word == text:
				break
			elif word < text:
				bottom = middle
			elif word > text:
				top = middle
		while index[middle][0] < word:
			middle += 1
			if middle >= self._index_count:
				break
		likely = [ tx[0] for tx in index[middle:middle + count] ]
		return likely



#----------------------------------------------------------------------
# ReadWtb - 读取文曲星 wtb 词库 (单词，音标，解释，例句, 类型）
#----------------------------------------------------------------------
class ReadWtb (object):

	def __init__ (self, fp):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'rb')
		self._content = fp.read()
		if self._content[:3] != '\x47\x47\x56':
			raise IOException('WTB local wordlist format error')
		self._read_index()

	def _read_index (self):
		content = self._content
		self._count = ord(content[14]) + ord(content[15]) * 256
		self._position_start = struct.unpack('<i', content[32:36])[0]
		self._position_endup = struct.unpack('<i', content[36:40])[0]
		caption = content[40:128]
		pos = caption.find('\x00')
		if pos >= 0: caption = caption[:pos]
		self._name = caption.decode('gbk', 'ignore')
		self._words = []
		self._lookup = {}
		pos = self._position_start
		for index in xrange(self._count):
			head = content[pos:pos + 6]	
			mode = ord(head[0]) + ord(head[1]) * 256
			pos += 6
			t1 = content[pos:pos + ord(head[2])].decode('gbk', 'ignore')
			pos += ord(head[2])
			t2 = content[pos:pos + ord(head[3])].decode('gbk', 'ignore')
			pos += ord(head[3])
			t3 = content[pos:pos + ord(head[4])].decode('gbk', 'ignore')
			pos += ord(head[4])
			t4 = content[pos:pos + ord(head[5])].decode('gbk', 'ignore')
			pos += ord(head[5])
			self._words.append((t1, t2, t3, t4, mode))
			self._lookup[t1] = self._words[-1]
		if self._position_endup != pos:
			print 'checksum error in wtb'
			return -1
		return 0

	def __len__ (self):
		return self._count

	def __contains__ (self, key):
		return self._lookup.__contains__(key)

	def __getitem__ (self, key):
		if type(key) in (type(''), type(u'')):
			return self._lookup[key]
		return self._words[key]

	def __iter__ (self):
		return self._words.__iter__()

	def get (self, key, default = None):
		if type(key) in (type(''), type(u'')):
			return self._lookup.get(key, default)
		if key < 0 or key >= self._count:
			return default
		return self._words[key]

	def name (self):
		return self._name



#----------------------------------------------------------------------
# ReadGvd - 读取文曲星 Gvd 字典
#----------------------------------------------------------------------
class ReadGvd (object):

	def __init__ (self, fp):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'rb')
		self._content = fp.read()
		if self._content[:8] != 'DictData':
			raise IOException('GVD local dictionary format error')
		self._read_index()
	
	def _read_index (self):
		content = self._content
		fetch = lambda d: ord(d[0]) + ord(d[1]) * 256 + ord(d[2]) * 65536
		self._count = ord(content[64]) + ord(content[65]) * 256
		self._position_start = fetch(content[72:75]) + 64
		self._words = []
		self._lookup = {}
		pos = self._position_start
		for index in xrange(self._count):
			size = ord(content[pos])
			next = fetch(content[pos + 1:pos + 4]) + 64
			word = content[pos + 4:pos + size]
			pos += size
			p1 = content.find('\x00', next, next + 512)
			text = ''
			if p1 >= next:
				text = content[next:p1]
			self._words.append((word, text))
			self._lookup[word] = self._words[-1]
		return 0

	def __len__ (self):
		return self._count

	def __contains__ (self, key):
		return self._lookup.__contains__(key)

	def __getitem__ (self, key):
		if type(key) in (type(''), type(u'')):
			return self._lookup[key]
		return self._words[key]

	def __iter__ (self):
		return self._words.__iter__()

	def get (self, key, default = None):
		if type(key) in (type(''), type(u'')):
			return self._lookup.get(key, default)
		if key < 0 or key >= self._count:
			return default
		return self._words[key]

	def name (self):
		return self._name


#----------------------------------------------------------------------
# WordBook - 读取教材词库
#----------------------------------------------------------------------
class WordBook (object):

	def __init__ (self, fp):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'r')
		words = []
		lookup = {}
		for line in fp:
			line = line.strip('\r\n\t ')
			if not line: continue
			word = line.lower()
			if word in lookup: continue
			lookup[word] = len(words)
			words.append(word)
		self._words = words
		self._lookup = lookup
		self._count = len(words)
	
	def __contains__ (self, key):
		return self._lookup.__contains__(key)
	
	def __len__ (self):
		return self._count
	
	def __getitem__ (self, key):
		if type(key) in (type(''), type(u'')):
			return self._lookup[key]
		return self._words[key]
	
	def __iter__ (self):
		return self._words.__iter__()
	
	def get (self, key, default = None):
		if type(key) in (type(''), type(u'')):
			return self._lookup.get(key, default)
		if key < 0 or key >= self._count:
			return default
		return self._words[key]
	


#----------------------------------------------------------------------
# MdxDict - use to modify mdx files
#----------------------------------------------------------------------
class MdxDict (object):

	def __init__ (self):
		self._lookup = {}

	def load (self, fp, codec = 'utf-8'):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'rb')
		content = fp.read()
		fp.close()
		content = content.decode(codec, 'ignore')
		word = None
		text = []
		for line in content.split('\n'):
			line = line.rstrip('\r\n')
			if word is None:
				if line == '':
					continue
				else:
					word = line.strip()
			else:
				if line.strip() != '</>':
					text.append(line)
				else:
					self._lookup[word] = '\n'.join(text)
					word = None
					text = []
		return 0
	
	def save (self, fp, codec = 'utf-8'):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'w')
		text = []
		keys = self._lookup.keys()
		keys.sort()
		for word in keys:
			text.append(word)
			for line in self._lookup[word].split('\n'):
				text.append(line)
			text.append('</>')
		content = '\n'.join(text)
		text = []
		content = content.encode(codec, 'ignore')
		fp.write(content)
		fp.close()
		content = None
		return 0

	def __contains__ (self, key):
		return self._lookup.__contains__(key)
	
	def __len__ (self):
		return len(self._lookup)
	
	def __getitem__ (self, key):
		return self._lookup.__getitem__(key)

	def __setitem__ (self, key, val):
		self._lookup[key] = val
	
	def __iter__ (self):
		return self._lookup.__iter__()
	
	def get (self, key, default = None):
		return self._lookup.get(key, default)

	def keys (self):
		return self._lookup.keys()

	def items (self):
		return self._lookup.items()

	def iteritems (self):
		return self._lookup.iteritems()

	def iterkeys (self):
		return self._lookup.iterkeys()

	def itervalues (self):
		return self._lookup.itervalues()



#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':

	# 打开有道词典文件
	db = YoudaoMini('dictcn.db')

	# 显示有多少个单词
	print 'count', len(db)

	# 查找一个存在的单词
	word = db.get('rural')
	if word != None:
		print 'phonetic: ', word[0]
		print 'explain: ', word[1]
	else:
		print 'not find'
	
	# 查询一批前缀相似的单词
	for word in db.match('astd', 10):
		print '>', word

	# 查询第一个单词
	print db[0]
	print db[-1]

	raw_input('press enter to quit ....')





