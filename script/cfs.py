#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
# 
# cfs.py - Distributed FileSystem Based on MongoDB 
#
# 初始化服务
#
#======================================================================
import sys
import time
import hashlib
import binascii
import datetime
import random
import base64
import cPickle
import Queue

import pymongo
import bson

try:
	import memcache
except:
	pass


#----------------------------------------------------------------------
# MIME
#----------------------------------------------------------------------
MIMETYPES = [
	('image/jpeg', 'jpg,jpeg'),
	('image/png', 'png'),
	('image/gif', 'gif'),
	('image/tiff', 'tiff'),
	('image/webp', 'webp'),
	('audio/x-aac', 'aac'),
	('audio/mpeg', 'mp3'),
	('audio/ogg', 'ogg'),
	('audio/flac', 'flac'),
	('audio/opus', 'opus'),
	('audio/vorbis', 'vorbis'),
	('audio/amr', 'amr'),
	('audio/wav', 'wav'),
	('audio/mid', 'mid'),
	('video/mp4', 'mp4'),
	('video/mpeg', 'mpg,mpeg'),
	('video/x-flv', 'flv'),
	('video/webm', 'webm'),
	('video/x-ms-wmv', 'wmv'),
	('video/real', 'rm'),
	('video/rmvb', 'rmvb'),
	('video/x-motion-jpeg', 'mjpg'),
	('text/plain', 'txt,md,c,h,cpp,cc,hpp,py,pyw,erl,java'),
	('text/json', 'json'),
	('text/html', 'html,htm'),
	('text/css', 'css'),
	('text/csv', 'csv'),
	('text/rtf', 'rtf'),
	('text/xml', 'xml'),
	('application/pdf', 'pdf'),
	('application/zip', 'zip'),
	('application/gzip', 'gzip'),
	('application/x-7z-compressed', '7z'),
	('application/x-shockwave-flash', 'swf'),
	('application/javascript', 'js'),
	('application/excel', 'xls,xlt,xlsx,xltx'),
	('application/msword', 'doc,dot,docx'),
	('application/powerpoint', 'ppt,pptx,pps'),
	('application/octet-stream', 'dat'),
]

MIMECHECK = {}

for mimetype, exts in MIMETYPES:
	for ext in exts.strip('\r\n\t ').split(','):
		MIMECHECK['.' + ext.strip('\r\n\t ')] = mimetype

def guessmime(fn):
	import os
	fn = fn.strip('\r\n\t ')
	pp = fn.rfind('.')
	if pp < 0:
		return 'application/octet-stream'
	ext = fn[pp:].strip('\r\n\t ').lower()
	return MIMECHECK.get(ext, 'application/octet-stream')

random_chars = 'AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456'
def random_str(randomlength = 8):
	str = ''
	length = len(random_chars) - 1
	rand = random.Random().randint
	for i in range(randomlength):
		str += random_chars[rand(0, length)]
	return str


#----------------------------------------------------------------------
# CFS
#----------------------------------------------------------------------
class CFS (object):

	def __init__ (self, url, ncollections = 4):
		self.config = self.__url_parse(url)
		self.__client = None
		self.__db = None
		self.__bad = False
		N = ncollections
		self.__num = N < 1 and 1 or (N > 256 and 256 or N)
		mask = -1
		for i in xrange(32):
			if (1 << i) == self.__num:
				mask = (1 << i) - 1
		self.__mask = mask
	
	# 解析 mongo url: mongodb://user:pass@abc.com/database?key=val
	def __url_parse (self, url):
		url = url.strip('\r\n\t ')
		chk = 'mongodb://'
		if url[:len(chk)] != chk:
			raise ValueError('bad protocol: %s'%url)
		config = {}
		config['url'] = url
		URL = url
		url = url[len(chk):]
		p1 = url.find('/')
		if p1 >= 0:
			dbname = url[p1 + 1:]
			p1 = dbname.find('?')
			if p1 >= 0:
				dbname = dbname[:p1]
			if not dbname:
				dbname = 'test'
		else:
			dbname = 'test'
		config['db'] = dbname
		return config
	
	# 返回一个 db对象，没有就创建
	def db (self):
		if self.__db != None:
			return self.__db
		if self.__client == None:
			if self.__bad:
				return None
			url = self.config['url']
			self.__client = pymongo.MongoClient(url)
		if self.__client is None:
			self.__bad = True
			return None
		db = self.__client[self.config['db']]
		if not db:
			return None
		self.__db = db
		return self.__db

	# 关闭数据库和客户端连接
	def close (self):
		if self.__db:
			self.__db = None
		if self.__client:
			try:
				self.__client.close()
			except:
				pass
			self.__client = None
		return True
	
	# 初始化数据库：建立集合和索引
	def init (self):
		db = self.db()
		auth = db['cfs.auth']
		auth.ensure_index("user", unique = True, background = True)
		admin = self.__client.admin
		for i in xrange(self.__num):
			coll = 'cfs.%03d'%i
			cfs = db[coll]
			cfs.ensure_index('name', unique = True, background = True)
			cfs.ensure_index([('expire', pymongo.ASCENDING)], background = True)
		return True
	
	# 根据名字求 hash值
	def hash (self, name):
		hh = binascii.crc32(name)
		tt = binascii.crc32(name[::-1])
		if hh < 0:
			hh = 0x100000000 + hh
		if tt < 0:
			tt = 0x100000000 + tt
		if self.__mask < 0:
			id = int(hh % self.__num)
		else:
			id = int(hh & self.__mask)
		if tt <= 0x7fffffff:
			tt = int(tt)
		return hh, id, tt
	
	# 写入文件
	def write (self, user, name, content, mime = None, md5 = None, expire = None):
		if not type(content) in (type(''), type(u'')):
			content = content.read()
		if md5 is None:
			md5 = hashlib.md5(content).hexdigest()
		if mime is None:
			mime = guessmime(name)
		hh, id, tt = self.hash(name)
		db = self.db()
		cc = db['cfs.%03d'%id]
		update = { 
			'name': name,
			'user': user,
			'content': bson.binary.Binary(content),
			'size': len(content),
			'mime': mime,
			'date': datetime.datetime.now(),
			'md5': md5,
			'shard': tt,
		}
		if expire != None:
			update['expire'] = expire
		cc.update({'name':name, 'shard':tt}, update, True)
		return True
	
	# 读取文件
	def read (self, name):
		hh, id, tt = self.hash(name)
		db = self.db()
		cc = db['cfs.%03d'%id]
		rr = cc.find_one({'name':name, 'shard':tt})
		if rr is None:
			return None
		result = {
			'name': rr.get('name', None),
			'user': str(rr.get('user', None)),
			'size': rr.get('size', 0),
			'mime': str(rr.get('mime', None)),
			'date': rr.get('date', None),
			'md5': str(rr.get('md5', '')),
			'shard': rr.get('shard', ''),
		}
		expire = rr.get('expire', None)
		if expire != None:
			if type(expire) == type(u''):
				expire = str(expire)
			result['expire'] = expire
		x = rr.get('content', None)
		if x != None:
			x = str(x)
		result['content'] = x
		return result
	
	# 删除文件
	def remove (self, name):
		hh, id, tt = self.hash(name)
		db = self.db()
		cc = db['cfs.%03d'%id]
		xx = cc.remove({'name':name, 'shard': tt})
		return xx['n']

	# 文件名标准化
	def normalize (self, name):
		name = name.strip('\r\n\t ')
		if name[:1] == '/':
			name = name[1:]
		return name

	# 添加用户
	def user_set (self, user, token):
		db = self.db()
		auth = db['cfs.auth']
		return auth.update({'user':user}, {'user':user, 'token':token}, True)
	
	# 删除用户
	def user_del (self, user):
		db = self.db()
		auth = db['cfs.auth']
		return auth.remove({'user':user})
	
	# 验证用户: user(用户名), name(希望写入的文件名), ts(时间戳)
	# verify - md5sum(user + token + name + ts)
	def user_auth (self, user, name, ts, verify, tolerant = 600):
		check = user + '/'
		if name[:len(check)] != check:
			return False
		if tolerant < 10:
			tolerant = 10
		now = long(time.time())
		ts = long(ts)
		if abs(now - ts) > tolerant:
			return False
		db = self.db()
		rr = db['cfs.auth'].find_one({'user':user})
		if rr == None:
			return False
		token = str(rr.get('token', ''))
		user = str(user)
		name = str(name)
		token = str(token)
		sign = hashlib.md5(user + token + name + str(ts)).hexdigest()
		if sign != verify:
			return False
		return True
	
	# 生成签名
	def user_sign (self, user, token, name, ts = None):
		if ts is None:
			ts = time.time()
		ts = long(ts)
		user = str(user)
		name = str(name)
		token = str(token)
		return hashlib.md5(user + token + name + str(ts)).hexdigest()
	
	# 生成密码
	def user_token (self):
		return random_str(32)
	
	# 列举用户
	def user_list (self):
		db = self.db()
		rr = db['cfs.auth']
		cc = []
		for obj in rr.find():
			if 'user' in obj and 'token' in obj:
				cc.append((obj['user'], obj['token']))
		return cc
	
	# 取得用户
	def user_read (self, user):
		db = self.db()
		rr = db['cfs.auth']
		cc = rr.find_one({'user':user})
		if cc is None:
			return None
		if 'user' in cc and 'token' in cc:
			return cc['token']
		return None


#----------------------------------------------------------------------
# FastCFS - Intergrate CFS and memcached
#----------------------------------------------------------------------
class FastCFS (object):

	def __init__ (self, content):
		self.__mongos = Queue.Queue()
		self.__caches = Queue.Queue()
		self.__config = {}
		self.__users = {}
		self.__config_parser(content)

	def __config_parser (self, content):
		self.close()
		self.__config = {}
		self.__users = {}
		if content[:3] == '\xef\xbb\xbf':
			content = content[3:]
		content = '\n'.join([ n.strip('\t ') for n in content.split('\n') ])
		import cStringIO
		sio = cStringIO.StringIO(content)
		import ConfigParser
		cp = ConfigParser.ConfigParser()
		cp.readfp(sio)
		config = {}
		for sect in cp.sections():
			for key, val in cp.items(sect):
				lowsect, lowkey = sect.lower(), key.lower()
				config.setdefault(lowsect, {})[lowkey] = val.strip('\t ')
		if not 'cfs' in config:
			raise ValueError('can not find CFS section in configure')
		mongos = config['cfs'].get('mongos', None)
		if mongos is None:
			raise ValueError('can not find mongos url in configure')
		self.__config['mongos'] = []
		for n in mongos.split(','):
			self.__config['mongos'].append(n.strip('\r\n\t '))
		self.__config['caches'] = []
		caches = config['cfs'].get('caches', None)
		if caches:
			for n in caches.split(','):
				self.__config['caches'].append(n.strip('\r\n\t '))
		self.__config['nc'] = 4
		nc = config['cfs'].get('nc', '-1')
		try:
			nc = int(nc)
			if nc > 0:
				self.__config['nc'] = nc
		except:
			pass
		self.__config['mongos_index'] = 0
		self.__config['caches_index'] = 0
		self.__config['root'] = config['cfs'].get('root', '').strip('\t ')
		self.__config['nocache'] = False
		self.__config['auth'] = False
		auth = config['cfs'].get('auth', 'False').lower().strip('\r\n\t ')
		if auth in ('1', 'yes', 'true', 'y', 'on', 'enable'):
			self.__config['auth'] = True
		self.__config['error'] = 600
		error = config.get('error', '600')
		try:
			error = int(error)
			if error > 1:
				self.__config['error'] = error
		except:
			pass
		self.__config['maxsize'] = 8 * 1024 * 1024
		self.__configure = config
		return 0

	# 读取配置
	def config (self, sect, name, default = ''):
		if not sect in self.__configure:
			return default
		if not name in self.__configure[sect]:
			return default
		text = self.__configure.get(sect, {}).get(name, str(default))
		# 如果配置是字符串直接返回
		if type(default) in (type(''), type(u'')):
			return text
		# 如果是 boolean 则判断
		if default in (True, False):
			text = text.lower().strip('\r\n\t ')
			if text in ('true', '1', 'yes', 'on', 'enable', 'y'):
				return True
			return False
		# 如果配置是数字则计算 MB, KB等
		text = text.strip('\r\n\t ')
		multiply = 1
		text = text.strip('\r\n\t ')
		postfix1 = text[-1:].lower()
		postfix2 = text[-2:].lower()
		if postfix1 == 'k':
			multiply = 1024
			text = text[:-1]
		elif postfix1 == 'm': 
			multiply = 1024 * 1024
			text = text[:-1]
		elif postfix2 == 'kb':
			multiply = 1024
			text = text[:-2]
		elif postfix2 == 'mb':
			multiply = 1024 * 1024
			text = text[:-2]
		try: text = int(text.strip('\r\n\t '), 0)
		except: text = default
		if multiply > 1: 
			text *= multiply
		return text

	# 关闭所有
	def close (self):
		while True:
			try:
				cfs = self.__mongos.get_nowait()
				try:
					cfs.close()
				except:
					pass
			except:
				break
		while True:
			try:
				cache = self.__caches.get_nowait()
				try:
					cache.disconnect_all()
				except:
					pass
			except:
				break
		return True

	# 取得一个 CFS对象
	def __cfs_get (self):
		try:
			cfs = self.__mongos.get_nowait()
			return cfs
		except:
			pass
		index = self.__config['mongos_index'] % len(self.__config['mongos'])
		self.__config['mongos_index'] += 1
		url = self.__config['mongos'][index]
		ncollections = self.__config['nc']
		cfs = CFS(url, ncollections)
		return cfs
	
	# 释放一个 CFS对象 
	def __cfs_release (self, cfs):
		if cfs != None:
			self.__mongos.put(cfs)
	
	# 取得一个 memcached客户端
	def __cache_get (self):
		caches = self.__config['caches']
		if not caches:
			return None
		if self.__config['nocache']:
			return None
		try:
			cache = self.__caches.get_nowait()
			return cache
		except:
			pass
		try:
			import memcache
		except:
			return None
		cache = memcache.Client(self.__config['caches'])
		return cache
	
	# 释放一个 memcached客户端
	def __cache_release (self, cache):
		if cache != None:
			self.__caches.put(cache)

	# 初始化数据库
	def init (self):
		cfs = self.__cfs_get()
		cfs.init()
		self.__cfs_release(cfs)
	
	# 用户设置
	def user_set (self, user, token):
		cfs = self.__cfs_get()
		hr = cfs.user_set(user, token)
		self.__cfs_release(cfs)
		return hr
	
	# 用户删除
	def user_del (self, user):
		cfs = self.__cfs_get()
		hr = cfs.user_del(user)
		self.__cfs_release(cfs)
		return hr
	
	# 用户列表
	def user_list (self):
		cfs = self.__cfs_get()
		hr = cfs.user_list()
		self.__cfs_release(cfs)
		return hr
	
	# 用户密码生成
	def user_token (self):
		return random_str(32)

	# 生成签名
	def user_sign (self, user, token, name, ts = None):
		if ts is None:
			ts = time.time()
		ts = long(ts)
		user = str(user)
		name = str(name)
		token = str(token)
		hh = hashlib.md5(user + token + name + str(ts)).hexdigest()
		return hh.lower()

	# 验证用户是否有权限写某文件
	def __auth (self, cfs, user, name, ts, verify):
		if not self.__config['auth']:
			return True
		error = self.__config['error']
		now = long(time.time())
		ts = long(ts)
		if user == 'root':
			token = self.__config['root']
			if not token:
				return False
			if abs(ts - now) > error:
				return False
			sign = self.user_sign(user, token, name, ts)
			if verify != sign:
				return False
			return True
		token = self.__users.get(user, None)
		if token == None:
			token = cfs.user_read(user)
		if token == None:
			return False
		self.__users[user] = token
		if token == '':
			return True
		if abs(ts - now) > error:
			return False
		sign = self.user_sign(user, token, name, ts)
		if verify != sign:
			return False
		return True
	
	# 验证用户是否有权限写某文件
	def auth (self, user, name, ts, verify):
		if not self.__config['auth']:
			return True
		if user in self.users or user == 'root':
			return self.__auth(None, user, name, ts, verify)
		cfs = self.__cfs_get()
		if cfs == None:
			return False
		hr = self.__auth(cfs, user, name, ts, verify)
		self.__cfs_release(cfs)
		return hr
	
	# 检查文件名
	def check_name (self, name):
		if not name:
			return False
		if len(name) > 240:
			return False
		if '\t' in name:
			return False
		if ' ' in name:
			return False
		if '\n' in name:
			return False
		if '\r' in name:
			return False
		if ':' in name:
			return False
		return True

	# 写入文件，成功返回0，失败返回字符串原因
	def write (self, user, name, content, mime = None, md5 = None, ts = 1, verify = '', expire = None):
		if not self.check_name(name):
			return 'Invalid file name'
		if content != None:
			if len(content) > self.__config['maxsize']:
				return 'file size exceed limit'
		access = False
		cfs = None
		if not self.__config['auth'] and user == None:
			user = 'root'
		if user == None:
			return 'None user name'
		if user != 'root' and name[:len(user)] != user:
			return 'Authorized Failed'
		if not self.__config['auth']:
			access = True
			if user in (None, ''):
				user = 'root'
		elif user in self.__users or user == 'root':
			access = self.__auth(None, user, name, ts, verify)
		else:
			cfs = self.__cfs_get()
			if cfs == None:
				return 'Error connect to mongos'
			access = self.__auth(cfs, user, name, ts, verify)
		if not access:
			if cfs != None:
				self.__cfs_release(cfs)
			return 'Authorized Failed'
		if not mime:
			mime = guessmime(name)
		if not md5:
			if content != None:
				md5 = hashlib.md5(content).hexdigest()
		if cfs == None:
			cfs = self.__cfs_get()
			if not cfs:
				return 'Error connect to mongos'
		if content != None:
			cfs.write(user, name, content, mime, md5, expire)
		else:
			cfs.remove(name)
		self.__cfs_release(cfs)
		key = 'FILE:' + name
		cache = self.__cache_get()
		if cache != None:
			cache.delete(key)
			self.__cache_release(cache)
		return 0
	
	# 读出文件
	def read (self, name):
		if not self.check_name(name):
			return 'Invalid file name: %s'%repr(name)
		cache = self.__cache_get()
		if cache:
			obj = cache.get('FILE:' + name)
			if obj != None:
				obj = cPickle.loads(obj)
				if obj:
					obj['cache'] = 1
				self.__cache_release(cache)
				return obj
		self.__cache_release(cache)
		cfs = self.__cfs_get()
		obj = cfs.read(name)
		self.__cfs_release(cfs)
		if obj == None:
			return None
		obj['cache'] = 0
		cache = self.__cache_get()
		if cache:
			text = cPickle.dumps(obj, 2)
			cache.set('FILE:' + name, text)
			self.__cache_release(cache)
		return obj
	
	# 删除文件
	def remove (self, user, name, ts, verify):
		return self.write(user, name, None, None, None, ts, verify)
	
	# 强制删除
	def force_remove (self, name):
		cfs = self.__cfs_get()
		cfs.remove(name)
		self.__cfs_release(cfs)

	# 是否允许缓存
	def cache (self, on = None):
		hr = not self.__config['nocache']
		if on is None:
			return hr
		if on:
			self.__config['nocache'] = False
		else:
			self.__config['nocache'] = True
		return hr


#----------------------------------------------------------------------
# signature
#----------------------------------------------------------------------
def sign_make(user, token, name, ts = None):
	if ts == None:
		ts = time.time()
	ts = long(ts)
	user = str(user)
	token = str(token)
	name = str(name)
	verify = hashlib.md5(user + token + name + str(ts)).hexdigest()
	text = '%s:%s:%s'%(user, ts, verify.lower())
	return base64.standard_b64encode(text)

def sign_extract(signature):
	if not signature:
		return None, None, None
	try:
		text = base64.standard_b64decode(signature)
	except:
		return None, None, None
	part = text.split(':')
	if len(part) != 3:
		return None, None, None
	user = part[0].strip('\r\n\t ')
	try:
		ts = long(part[1])
	except:
		return None, None, None
	verify = part[2].strip('\r\n\t ').lower()
	return user, ts, verify

# 检查文件名
def check_name (name):
	if not name:
		return False
	if len(name) > 240:
		return False
	if '\t' in name:
		return False
	if ' ' in name:
		return False
	if '\n' in name:
		return False
	if '\r' in name:
		return False
	if ':' in name:
		return False
	if '#' in name:
		return False
	return True


#----------------------------------------------------------------------
# CFS
#----------------------------------------------------------------------
if __name__ == '__main__':

	def test1():
		cfs = CFS('mongodb://localhost')
		cfs.init()
		cfs.write('linwei', 'test/1.jpg', '1234')
		t = time.time()
		for i in xrange(1000):
			cfs.write('linwei', 'test/1.jpg', '1234567')
		t = time.time() - t
		cfs.write('linwei', 'test/2.mp4', '3334')
		print 'time', t
		t = time.time()
		for i in xrange(100):
			cfs.read('test/1.jpg')
		t = time.time() - t
		print 'time', t
		print cfs.read('test/1.jpg')
		#cfs.remove('test/1.jpg')
		print cfs.read('test/2.mp4')
		#(self, owner, name, content, mime = None, md5 = None):
	
	def test2():
		c = memcache.Client(['127.0.0.1:11211'])
		t = time.time()
		c.set('test/2.mp4', 'asdfa')
		for i in xrange(10000):
			c.set('test/2.mp4', '12341324')
		t = time.time() - t
		print c.get('test/2.mp4')
		print 'time', t
		return 0

	def test3():
		cfs = CFS('mongodb://skywind:skywind@192.168.0.22/skywind')
		cfs.db()
		cfs.init()
		c = memcache.Client(['127.0.0.1:11211'])
		c.set('abcddf12834=*/.\\', 'asdf\x00\x01\r\n asdf')
		print repr(c.get('abcddf12834=*/.\\'))
		print cfs.user_token()
		cfs.user_set('skywind', cfs.user_token())
		cfs.user_set('coco', cfs.user_token())
		for row in cfs.user_list():
			print row
		return 0
	
	ini = '''
	[cfs]
	mongos2 = mongodb://skywind:skywind@192.168.0.22/skywind,mongodb://skywind:skywind@192.168.0.22/skywind
	mongos = mongodb://127.0.0.1/skywind,mongodb://127.0.0.1/skywind
	caches = 127.0.0.1:11211,127.0.0.1:11211
	root = 123456
	auth = true
	'''
	
	def test4():
		fs = FastCFS(ini)
		ts = long(time.time())
		verify = fs.user_sign('skywind', '3nPDI3cBFnF3tmUdoaNaqUrj4sRuRxzE', 'skywind/t2.jpg', ts)
		fs.cache(False)
		print fs.write('skywind', 'skywind/t2.jpg', '1234324', None, None, ts, verify, '2015')
		t = time.time()
		for i in xrange(100):
			fs.read('skywind/t2.jpg')
		t = time.time() - t
		print 'time', t
		#fs.remove('skywind', 'skywind/t1.jpg', ts, verify)
		print fs.read('skywind/t2.jpg')
		print fs.config('cfs', 'auth', False)
		return 0

	test4()




