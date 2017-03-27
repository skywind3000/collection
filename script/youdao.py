#!/usr/bin/env python2
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
# python3 compatible 
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
	xrange = range
	int2byte = lambda x: bytes([x])
	byte2int = lambda b: b
	def raw_input (x):
		sys.stdout.write(x)
		sys.stdout.flush()
		return sys.stdin.readline()
else:
	int2byte = lambda x: chr(x)
	byte2int = lambda x: ord(x)


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
		if self._content[:5] != b'\xff\xff\xff\xff\x01':
			raise IOException('Youdao local dictionary format error')
		self._index_size = struct.unpack('I', self._content[5:9])[0]
		self._index_end = self._index_size + 5
		self._read_index()
		convert = {}
		convert['A'] = u'\xe6'
		convert['B'] = u'\u0251'
		convert['E'] = u'\u04d9'
		convert['G'] = u'\u0292'
		convert['F'] = u'\u0283'
		convert['O'] = u'\u0254'
		convert['N'] = u'\u014b'
		convert['R'] = u'\u028c'
		convert['T'] = u'\xf0'
		convert['Z'] = u'\u03b8'
		self._convert = convert
	
	# 读取索引
	def _read_index (self):
		self._index_array = []
		self._index_dict = {}
		self._index_cache = {}
		pos = 9
		content = self._content
		if sys.version_info[0] >= 3:
			ord = lambda x: x
		while pos < self._index_end:
			size = 255 - byte2int(content[pos])
			text = bytearray(content[pos + 1:pos + 1 + size])
			for i in xrange(len(text)):
				text[i] = 255 - text[i]
			text = bytes(text).decode('gbk')
			pos += 1 + size + 4
			locate = content[pos - 4:pos]
			c1 = 255 - byte2int(locate[0])
			c2 = 255 - byte2int(locate[1])
			c3 = 255 - byte2int(locate[2])
			c4 = 255 - byte2int(locate[3])
			locate = (c4 << 24) | (c3 << 16) | (c2 << 8) | c1
			self._index_array.append((text, locate))
			self._index_dict[text] = (text, locate)
			self._index_dict[text.lower()] = (text, locate)
		self._index_array.sort(key = lambda x: x[0].lower())
		self._index_count = len(self._index_array)
		return self._index_count
	
	# 查询单词
	def lookup (self, word):
		if isinstance(word, int):
			return self._index_array[word][0]
		word = word.strip('\r\n\t ')
		info = self._index_dict.get(word, None)
		if info is None:
			info = self._index_dict.get(word.lower(), None)
			if info is None:
				return None
		word = info[0]
		locate = info[1]
		data = self._index_cache.get(word, None)
		if data == None:
			pos = locate + self._index_end
			content = self._content
			c1 = byte2int(content[pos + 0])
			c2 = byte2int(content[pos + 1])
			size = c1 + (c2 << 8)
			c1 = 255 - byte2int(content[pos + 2])
			p1 = bytearray(content[pos + 3:pos + 3 + c1])
			pos += 3 + c1
			c2 = 255 - byte2int(content[pos])
			p2 = bytearray(content[pos + 1:pos + 1 + c2])
			pos += 1 + c2
			for i in xrange(len(p1)):
				p1[i] = 255 - p1[i]
			for i in xrange(len(p2)):
				p2[i] = 255 - p2[i]
			p1 = bytes(p1).decode('gbk', 'ignore')
			p2 = bytes(p2).decode('gbk', 'ignore')
			self._index_cache[word] = (p1, p2)
			data = self._index_cache[word]
		return (word, data[0], data[1])
	
	# 快速查询
	def __getitem__ (self, key):
		return self.lookup(key)
	
	# 是否包含
	def __contains__ (self, key):
		if key in self._index_dict:
			return True
		return self._index_dict.__contains__(key.lower())
	
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
		word = word.lower()
		bottom = self._index_count - 1
		middle = top
		index = self._index_array
		while top < bottom:
			middle = (top + bottom) >> 1
			if top == middle or bottom == middle:
				break
			text = index[middle][0].lower()
			if word == text:
				break
			elif word < text:
				bottom = middle
			elif word > text:
				top = middle
		while index[middle][0].lower() < word:
			middle += 1
			if middle >= self._index_count:
				break
		likely = [ tx[0] for tx in index[middle:middle + count] ]
		return likely

	# 转换国际音标：数据库编码为 GBK，这里将代号表示的音标转成标准格式
	def convert_phonetic (self, phonetic):
		convert = self._convert
		for k in convert:
			phonetic = phonetic.replace(k, convert[k])
		return phonetic
	

#----------------------------------------------------------------------
# StarDict 的词典读取，只支持 2.4.2 纯文本格式
#----------------------------------------------------------------------
class StarDict (object):

	def __init__ (self, idxfile, dictfile):
		if type(idxfile) in (type(''), type(u'')):
			self._idx = open(idxfile, 'rb').read()
		else:
			self._idx = idxfile.read()
		if type(dictfile) in (type(''), type(u'')):
			self._dict = open(dictfile, 'rb').read()
		else:
			self._dict = dictfile.read()
		self._read_index()

	# 读取索引
	def _read_index (self):
		self._index_array = []
		self._index_dict = {}
		self._index_cache = {}
		pos = 0
		content = self._idx
		length = len(content)
		unpack = struct.unpack
		while pos < length:
			p1 = content.find(b'\x00', pos)
			if p1 < 0:
				break
			word = content[pos:p1].decode('utf-8', 'ignore')
			position, size = unpack('>II', content[p1+1:p1+9])
			pos = p1 + 9
			self._index_array.append((word, position, size))
			self._index_dict[word] = (position, size)
		self._index_array.sort()
		self._index_count = len(self._index_array)
		return self._index_count

	# 查询单词
	def lookup (self, word):
		if isinstance(word, int):
			return self._index_array[word][0]
		word = word.strip('\r\n\t ')
		locate = self._index_dict.get(word, None)
		if locate is None:
			return None
		data = self._index_cache.get(word, None)
		if data == None:
			pos, size = locate
			text = self._dict[pos:pos + size].decode('utf-8', 'ignore')
			self._index_cache[word] = text
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
			if not word in words:
				words[word] = count
			if not word.lower() in words:
				words[word.lower()] = count
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
			if key in self._words:
				return self._words[key]
			key = key.lower()
			if key in self._words:
				return self._words[key]
		return self._index[key][0]
	
	def get (self, key, default = None):
		if type(key) in (type(''), type(u'')):
			if key in self._words:
				return self._words.get(key, default)
			key = key.lower()
			if key in self._words:
				return self._words.get(key, default)
		if key < 0 or key >= self._count:
			return default
		return self._index[key]
	
	def __iter__ (self):
		return self._wordpure.__iter__()

	def __contains__ (self, key):
		if key in self._words:
			return True
		return self._words.__contains__(key.lower())


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
			mode = byte2int(content[position])
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

	def get (self, word, default = None):
		r = self.lookup(word)
		if r is not None:
			return r
		return default

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

	def __len__ (self):
		return self._index_count

	def __contains__ (self, word):
		return self.lookup(word) is not None

	def __getitem__ (self, word):
		return self.lookup(word)



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
			print('checksum error in wtb')
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
# 读取 Excel 词表
#----------------------------------------------------------------------
def excel_col2num(col):
	num = 0
	for ch in col:
		nn = (ord(ch.lower()) - 97 + 1)
		num = num * 26 + nn
	return num

def excel_num2col(num):
	div = num
	string = ''
	temp = 0
	while div > 0:
		module = (div - 1) % 26
		string = chr(65 + module) + string
		div = int((div - module) / 26)
	return string

class ExcelReader (object):

	def __init__ (self, filename):
		import xlrd
		self._workbook = xlrd.open_workbook(filename)
		self._col2num = {}
		self._num2col = {}
		self._sheets = {}
	
	def col2num (self, col):
		num = self._col2num.get(col, None)
		if num is None:
			num = excel_col2num(col)
			self._col2num[col] = num
		return num
	
	def num2col (self, num):
		col = self._num2col.get(num, None)
		if col is None:
			col = excel_num2col(num)
			self._num2col[num] = col
		return col

	def read_sheet (self, name):
		if not name in self._workbook.sheet_names():
			return None
		sheet = self._workbook.sheet_by_name(name)
		if sheet is None:
			return None
		data = []
		for r in xrange(sheet.nrows):
			row = sheet.row_values(r)
			data.append(row)
		sheet = None
		return data

	def sheet (self, name):
		if name in self._sheets:
			return self._sheets[name]
		data = self.read_sheet(name)
		self._sheets[name] = data
		return data


#----------------------------------------------------------------------
# csv load / save
#----------------------------------------------------------------------
def csv_load (filename, encoding = None):
	content = None
	text = None
	try:
		content = open(filename, 'rb').read()
	except:
		return None
	if content is None:
		return None
	if content[:3] == b'\xef\xbb\xbf':
		text = content[3:].decode('utf-8')
	elif encoding is not None:
		text = content.decode(encoding, 'ignore')
	else:
		codec = sys.getdefaultencoding()
		text = None
		for name in [codec, 'utf-8', 'gbk', 'ascii', 'latin1']:
			try:
				text = content.decode(name)
				break
			except:
				pass
		if text is None:
			text = content.decode('utf-8', 'ignore')
	if not text:
		return None
	import csv
	if sys.version_info[0] < 3:
		import cStringIO
		sio = cStringIO.StringIO(text.encode('utf-8', 'ignore'))
	else:
		import io
		sio = io.StringIO(text)
	reader = csv.reader(sio)
	output = []
	if sys.version_info[0] < 3:
		for row in reader:
			output.append([ n.decode('utf-8', 'ignore') for n in row ])
	else:
		for row in reader:
			output.append(row)
	return output


def csv_save (rows, filename, encoding = 'utf-8'):
	import csv
	ispy2 = (sys.version_info[0] < 3)
	if not encoding:
		encoding = 'utf-8'
	if sys.version_info[0] < 3:
		fp = open(filename, 'wb')
		writer = csv.writer(fp)
	else:
		fp = open(filename, 'w', encoding = encoding)
		writer = csv.writer(fp)
	for row in rows:
		newrow = []
		for n in row:
			if isinstance(n, int) or isinstance(n, long):
				n = str(n)
			elif isinstance(n, float):
				n = str(n)
			elif not isinstance(n, bytes):
				if (n is not None) and ispy2:
					n = n.encode(encoding, 'ignore')
			newrow.append(n)
		writer.writerow(newrow)
	fp.close()
	return True


#----------------------------------------------------------------------
# CsvData
#----------------------------------------------------------------------
class CsvData (object):

	def __init__ (self):
		self._rows = []
	
	def load (self, fp, **kwargs):
		if type(fp) in (type(b''), type(u'')):
			fp = open(fp, 'rb')
		codec = kwargs.get('codec', 'utf-8')
		content = fp.read()
		if type(content) != type(b''):
			content = content.encode(codec, 'ignore')
		import io
		bio = io.BytesIO()
		bio.write(content)
		bio.seek(0)
		import csv
		rows = []
		argv = {}
		for k in kwargs:
			if k != 'codec':
				argv[k] = kwargs[v]
		reader = csv.reader(bio, **argv)
		for row in reader:
			row = [ n.decode(codec, 'ignore') for n in row ]
			rows.append(row)
		self._rows = rows
	
	def save (self, fp, **kwargs):
		codec = kwargs.get('codec', 'utf-8')
		import io
		bio = io.BytesIO()
		import csv
		argv = {}
		for k in kwargs:
			if k != 'codec':
				argv[k] = kwargs[v]
		writer = csv.writer(bio, **argv)
		if sys.version_info[0] >= 3:
			unic = str
		else:
			unic = unicode
		def encode (text):
			if isinstance(text, long) or isinstance(text, int):
				return unic(text)
			if isinstance(text, float):
				return unic(text)
			return text.encode(codec)
		for row in self._rows:
			row = [ encode(n) for n in row ]
			writer.writerow(row)
		needclose = False
		if type(fp) in (type(b''), type(u'')):
			fp = open(fp, 'wb')
			needclose = True
		fp.write(bio.getvalue())
		if needclose:
			fp.close()
		return 0

	def __len__ (self):
		return len(self._rows)

	def __getitem__ (self, index):
		return self._rows.__getitem__(index)

	def __setitem__ (self, index, row):
		self._rows[index] = row

	def __delitem__ (self, index):
		return self._rows.__delitem__(index)

	def __iter__ (self):
		return self._rows.__iter__()

	def __repr__ (self):
		return self._rows.__repr__()

	def next (self):
		return self._rows.next()

	def append (self, row):
		return self._rows.append(row)

	def pop (self):
		return self._rows.pop()

	def extend (self, rows):
		return self._rows.extend(rows)

	def index (self, row):
		return self._rows.index(row)

	def insert (self, index, row):
		return self._rows.insert(index, row)

	def remove (self, row):
		return self._rows.remove(row)

	def __getslice__ (self, **kwargs):
		return self._rows.__getslice__(**kwargs)

	def count (self, row):
		return self._rows.count(row)

	def reverse (self):
		self._rows.reverse()

	def sort (self, **kwargs):
		return self._rows.sort(**kwargs)


#----------------------------------------------------------------------
# WordCount
#----------------------------------------------------------------------
class WordCount (object):

	def __init__ (self):
		self._words = {}
	
	def __len__ (self):
		return len(self._words)

	def __getitem__ (self, key):
		return self._words.__getitem__(key)

	def __contains__ (self, key):
		return (key in self._words)

	def get (self, key, default = None):
		return self._words.get(key, default)

	def reset (self):
		self._words = {}

	def read (self, fp, filter = None, lower = True):
		if type(fp) in (type(''), type(u'')):
			fp = open(fp, 'r')
		count = 0
		words = {}
		for line in fp:
			line = line.strip()
			if not line:
				continue
			for word in line.split():
				word = word.strip()
				if not word:
					continue
				if not word.isalpha():
					continue
				if filter:
					word = filter(word)
				if not word:
					continue
				word = word.strip()
				if not word:
					continue
				if not word.isalpha():
					continue
				if lower and (not word.islower()):
					continue
				self._words[word] = self._words.get(word, 0) + 1
				count += 1
				if count <= 10000:
					words[word] = words.get(word, 0) + 1
		return count, len(words)

	def dump (self):
		items = [ (v, k) for (k, v) in self._words.items() ]
		items.sort(reverse = True)
		return [ (v, k) for (k, v) in items ]


#----------------------------------------------------------------------
# http_request
#----------------------------------------------------------------------
def http_request(url, timeout = 10, data = None, post = False):
	import socket
	if sys.version_info[0] >= 3:
		import urllib
		import urllib.parse
		import urllib.request
		import urllib.error
		if data is not None:
			if isinstance(data, dict):
				data = urllib.parse.urlencode(data)
		if not post:
			if data is None:
				req = urllib.request.Request(url)
			else:
				mark = '?' in url and '&' or '?'
				req = urllib.request.Request(url + mark + data)
		else:
			data = data is not None and data or ''
			if not isinstance(data, bytes):
				data = data.encode('utf-8', 'ignore')
			req = urllib.request.Request(url, data)
		try:
			res = urllib.request.urlopen(req, timeout = timeout)
		except urllib.error.HTTPError as e:
			return e.code, str(e.message)
		except urllib.error.URLError as e:
			return -1, str(e)
		except socket.timeout:
			return -2, 'timeout'
		content = res.read()
	else:
		import urllib2
		import urllib
		if data is not None:
			if isinstance(data, dict):
				part = {}
				for key in data:
					val = data[key]
					if isinstance(key, unicode):
						key = key.encode('utf-8')
					if isinstance(val, unicode):
						val = val.encode('utf-8')
					part[key] = val
				data = urllib.urlencode(part)
			if not isinstance(data, bytes):
				data = data.encode('utf-8', 'ignore')
		if not post:
			if data is None:
				req = urllib2.Request(url)
			else:
				mark = '?' in url and '&' or '?'
				req = urllib2.Request(url + mark + data)
		else:
			req = urllib2.Request(url, data is not None and data or '')
		try:
			res = urllib2.urlopen(req, timeout = timeout)
			content = res.read()
		except urllib2.HTTPError as e:
			return e.code, str(e.message)
		except urllib2.URLError as e:
			return -1, str(e)
		except socket.timeout:
			return -2, 'timeout'
	return 200, content


#----------------------------------------------------------------------
# 有道词典在线 
#----------------------------------------------------------------------
YOUDAO_USER = '11pegasus11'
YOUDAO_PASS = '273646050'

def YoudaoOnline (text):
	req = {}
	req['keyfrom'] = YOUDAO_USER
	req['key'] = YOUDAO_PASS
	req['type'] = 'data'
	req['doctype'] = 'json'
	req['version'] = '1.1'
	req['q'] = text
	url = 'http://fanyi.youdao.com/openapi.do'
	code, data = http_request(url, data = req, post = True)
	if code != 200:
		print('youdao: http %d'%code)
		return None
	data = data.decode('utf-8', 'ignore')
	import json
	try:
		obj = json.loads(data)
	except:
		print('youdao: json error')
		# print(data.encode('gbk', 'ignore'))
		return 0
	obj['error'] = 0
	obj['message'] = 'ok'
	return obj

def QueryYoudao (word):
	data = YoudaoOnline(word)
	if data is None:
		return None
	if data is 0:
		return 0
	if not 'basic' in data:
		return None
	key = word
	phonetic = None
	explain = None
	translation = None
	if 'web' in data:
		for web in data['web']:
			if not 'key' in web:
				continue
			if word == web['key']:
				key = web['key']
				break
		if not key:
			for web in data['web']:
				if not 'key' in web:
					continue
				if word.lower() == web['key'].lower():
					key = web['key']
					break
	basic = data['basic']
	if 'phonetic' in basic:
		phonetic = basic['phonetic']
	elif 'phonetic_uk' in basic:
		phonetic = basic['phonetic_uk']
	elif 'phonetic_us' in basic:
		phonetic = basic['phonetic_us']
	if 'explains' in basic:
		explain = '\n'.join(basic['explains'])
	translation = data.get('translation')
	return key, phonetic, explain, translation
	

#----------------------------------------------------------------------
# 金山词霸在线
#----------------------------------------------------------------------
CIBA_KEY = '' # http://open.iciba.com/?c=api 注册key

def CibaOnline(word):
	url = 'http://dict-co.iciba.com/api/dictionary.php'
	req = {}
	req['w'] = word
	req['type'] = 'json'
	req['key'] = CIBA_KEY
	code, data = http_request(url, data = req, post = False)
	if code != 200:
		return None
	body = None
	import json
	try:
		body = json.loads(data)
	except:
		print('ciba json error')
		return 0
	return body

def QueryCiba(word):
	req = CibaOnline(word)
	if req is None:
		return None
	if req is 0:
		return 0
	name = req.get('word_name', None)
	if not name:
		return None
	symbols = req.get('symbols')
	if not symbols:
		return 0
	if not isinstance(symbols, list):
		return 0
	symbol = symbols[0]
	if symbol.get('ph_en'):
		phonetic = symbol['ph_en']
	elif symbol.get('ph_am'):
		phonetic = symbol['ph_am']
	elif symbol.get('ph_other'):
		phonetic = symbol['ph_other']
	else:
		phonetic = None
	parts = symbol.get('parts', [])
	output = []
	for part in parts:
		text = part['part'] + ' ' + '; '.join(part['means'])
		output.append(text)
	translation = '\n'.join(output)
	return name, phonetic, translation, ''


#----------------------------------------------------------------------
# 合并请求单词
#----------------------------------------------------------------------
ENABLE_CIBA = True
ENABLE_YOUDAO = True

def OnlineQuery(word):
	global ENABLE_CIBA
	global ENABLE_YOUDAO
	if not CIBA_KEY:
		ENABLE_CIBA = False
	if not YOUDAO_USER:
		ENABLE_YOUDAO = False
	if (not ENABLE_CIBA) and (not ENABLE_YOUDAO):
		# print('disabled')
		return 0
	if ENABLE_YOUDAO:
		data = QueryYoudao(word)
		if (data is not 0) and (data is not None):
			return data
		if data is 0:
			ENABLE_YOUDAO = False
		# print('fuck youdao: %s'%data)
	if ENABLE_CIBA:
		data = QueryCiba(word)
		if (data is not 0) and (data is not None):
			return data
		if data is 0:
			ENABLE_CIBA = False
		# print('fuck ciba: %s'%data)
	return None
		

#----------------------------------------------------------------------
# 命令行查有道词典本地数据库：dicten.db/dictcn.db
#----------------------------------------------------------------------
def main(args = None):
	if args is None:
		args = sys.argv
	# args = ['', '-d', 'dictcn.db', '-m', '10', 'python']
	args = [ n for n in args ]
	if len(args) < 4:
		print('usage: %s -d DATABASE [--match|-m NUM] WORD'%args[0])
		return -1
	WORD = args.pop()
	DATABASE = None
	MATCH = 0
	index = 1
	while index < len(args):
		if args[index] == '-d':
			if index + 2 > len(args):
				print('not enough arguments')
				return -2
			DATABASE = args[index + 1]
			index += 2
		elif args[index] in ('--match', '-m'):
			if index + 2 > len(args):
				print('not enough arguments')
				return -2
			MATCH = args[index + 1]
			MATCH = int(MATCH)
			index += 2
		else:
			print('unknow argument: %s'%args[index])
			return -2
	if DATABASE is None:
		print('unknow database')
		return -4
	if not os.path.exists(DATABASE):
		print('can not read: %s'%DATABASE)
		return -5
	db = YoudaoMini(DATABASE)
	if MATCH > 0:
		for word in db.match(WORD, MATCH):
			print(word)
	else:
		word = db.get(WORD)
		print(WORD)
		if word is None:
			print('<NOT FIND>')
		else:
			print(word[0])
			if word[1]:
				print(word[1])
			print(word[2])
	return 0


#----------------------------------------------------------------------
# testing case
#----------------------------------------------------------------------
if __name__ == '__main__':

	def test1():
		# 打开有道词典文件
		db = YoudaoMini('dictcn.db')

		# 显示有多少个单词
		print('count %d'%len(db))

		# 查找一个存在的单词
		word = db.get('Python')
		if word != None:
			print('word: %s'%word[0])
			print('phonetic: %s'%word[1])
			print('explain: %s'%word[2])
		else:
			print('not find')
		
		# 查询一批前缀相似的单词
		for word in db.match('pyr', 10):
			print('>', word)

		# 查询第一个单词
		print(db[0])
		print(db[-1])

		raw_input('press enter to quit ....')

	def test2():
		args = ['youdao.py', '-d', 'dictcn.db', 'rural']
		#args = ['youdao.py', '-m', '10', '-d', 'rural']
		main(args)
		return 0

	def test3():
		excel = ExcelReader('../oxford3k.xlsx')
		data = excel.read_sheet('Oxford3K')
		for row in data:
			print(row)
		return 0

	def test4():
		global CIBA_KEY
		import pprint
		pprint.pprint(QueryCiba('english'))
		print(OnlineQuery('english'))
		return 0

	# test4()
	main()



