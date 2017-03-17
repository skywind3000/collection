#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# stardict.py - 
#
# Created by skywind on 2017/03/13
# Last change: 2017/03/13 16:17:34
#
#======================================================================
import sys
import time
import os
import sqlite3

try:
	import json
except:
	import simplejson as json

MySQLdb = None


#----------------------------------------------------------------------
# python3 compatible
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
	unicode = str
	long = int


#----------------------------------------------------------------------
# StarDict 
#----------------------------------------------------------------------
class StarDict (object):

	def __init__ (self, filename, verbose = False):
		self.__dbname = os.path.abspath(filename)
		self.__conn = None
		self.__verbose = verbose
		self.__open()

	# 初始化并创建必要的表格和索引
	def __open (self):
		sql = '''
		CREATE TABLE IF NOT EXISTS "stardict" (
			"id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,
			"word" VARCHAR(64) NOT NULL UNIQUE,
			"phonetic" VARCHAR(64),
			"definition" TEXT,
			"translation" TEXT,
			"pos" VARCHAR(16),
			"collins" INTEGER DEFAULT(0),
			"oxford" INTEGER DEFAULT(0),
			"tag" VARCHAR(64),
			"bnc" INTEGER DEFAULT(NULL),
			"frq" INTEGER DEFAULT(NULL),
			"tense" TEXT,
			"plural" VARCHAR(64),
			"detail" TEXT,
			"audio" VARCHAR(128),
			"audio_uk" VARCHAR(128),
			"audio_us" VARCHAR(128)
		);
		CREATE UNIQUE INDEX IF NOT EXISTS "stardict_1" ON stardict (id);
		CREATE UNIQUE INDEX IF NOT EXISTS "stardict_2" ON stardict (word);
		CREATE INDEX IF NOT EXISTS "stardict_c1" ON stardict (collins);
		CREATE INDEX IF NOT EXISTS "stardict_c2" ON stardict (oxford);
		CREATE INDEX IF NOT EXISTS "stardict_c3" ON stardict (tag);
		'''

		self.__conn = sqlite3.connect(self.__dbname, isolation_level = "IMMEDIATE")
		self.__conn.isolation_level = "IMMEDIATE"

		sql = '\n'.join([ n.strip('\t') for n in sql.split('\n') ])
		sql = sql.strip('\n')

		self.__conn.executescript(sql)
		self.__conn.commit()

		self.__fields = [ 'id', 'word', 'phonetic', 'definition', 
			'translation', 'pos', 'collins', 'oxford', 'tag', 'bnc', 'frq', 
			'tense', 'plural', 'detail', 'audio', 'audio_uk', 'audio_us' ]
		fields = self.__fields
		self.__fields = [ (fields[i], i) for i in range(len(fields)) ]
		self.__names = { }
		for k, v in self.__fields:
			self.__names[k] = v
		self.__enable = self.__fields[2:]
		return True

	# 数据库记录转化为字典
	def __record2obj (self, record):
		if record is None:
			return None
		word = {}
		for k, v in self.__fields:
			word[k] = record[v]
		if word['detail'] is not None:
			word['detail'] = json.loads(word['detail'])
		return word

	# 关闭数据库
	def close (self):
		if self.__conn:
			self.__conn.close()
		self.__conn = None
	
	def __del__ (self):
		self.close()

	# 输出日志
	def out (self, text):
		if self.__verbose:
			print(text)
		return True

	# 查询单词
	def query (self, key):
		c = self.__conn.cursor()
		record = None
		if isinstance(key, int) or isinstance(key, long):
			c.execute('select * from stardict where id = ?;', (key,))
		elif isinstance(key, str) or isinstance(key, unicode):
			c.execute('select * from stardict where word = ?;', (key,))
		else:
			return None
		record = c.fetchone()
		return self.__record2obj(record)

	# 查询单词匹配
	def match (self, word, limit = 10):
		c = self.__conn.cursor()
		sql = 'select id, word from stardict where word like ? '
		sql += 'order by word limit ?;'
		c.execute(sql, (word + '%', limit))
		records = c.fetchall()
		result = []
		for record in records:
			result.append(tuple(record))
		return result

	# 批量查询
	def query_batch (self, keys):
		sql = 'select * from stardict where '
		if keys is None:
			return None
		if not keys:
			return []
		querys = []
		for key in keys:
			if isinstance(key, int) or isinstance(key, long):
				querys.append('id = ?')
			elif key is not None:
				querys.append('word = ?')
		sql = sql + ' or '.join(querys) + ';'
		query_word = {}
		query_id = {}
		c = self.__conn.cursor()
		c.execute(sql, tuple(keys))
		for row in c:
			obj = self.__record2obj(row)
			query_word[obj['word']] = obj
			query_id[obj['id']] = obj
		results = []
		for key in keys:
			if isinstance(key, int) or isinstance(key, long):
				results.append(query_id.get(key, None))
			elif key is not None:
				results.append(query_word.get(key, None))
			else:
				results.append(None)
		return tuple(results)

	# 取得单词总数
	def count (self):
		c = self.__conn.cursor()
		c.execute('select count(*) from stardict;')
		record = c.fetchone()
		return record[0]

	# 注册新单词
	def register (self, word, items, commit = True):
		sql = 'INSERT INTO stardict(word) VALUES(?);';
		try:
			self.__conn.execute(sql, (word,))
		except sqlite3.IntegrityError as e:
			self.out(str(e))
			return False
		except sqlite3.Error as e:
			self.out(str(e))
			return False
		self.update(word, items, commit)
		return True

	# 删除单词
	def remove (self, key, commit = True):
		if isinstance(key, int) or isinstance(key, long):
			sql = 'DELETE FROM stardict WHERE id=?;'
		else:
			sql = 'DELETE FROM stardict WHERE name=?;'
		try:
			self.__conn.execute(sql, (key,))
			if commit:
				self.__conn.commit()
		except sqlite3.IntegrityError:
			return False
		return True

	# 清空数据库
	def delete_all (self, reset_id = False):
		sql1 = 'DELETE FROM stardict;'
		sql2 = "UPDATE sqlite_sequence SET seq = 0 WHERE name = 'stardict';"
		try:
			self.__conn.execute(sql1)
			if reset_id:
				self.__conn.execute(sql2)
			self.__conn.commit()
		except sqlite3.IntegrityError as e:
			self.out(str(e))
			return False
		except sqlite3.Error as e:
			self.out(str(e))
			return False
		return True

	# 更新单词数据
	def update (self, key, items, commit = True):
		names = []
		values = []
		for name, id in self.__enable:
			if name in items:
				names.append(name)
				value = items[name]
				if name == 'detail':
					if value is not None:
						value = json.dumps(value, ensure_ascii = False)
				values.append(value)
		if len(names) == 0:
			if commit:
				try:
					self.__conn.commit()
				except sqlite3.IntegrityError:
					return False
			return False
		sql = 'UPDATE stardict SET ' + ', '.join(['%s=?'%n for n in names])
		if isinstance(key, str) or isinstance(key, unicode):
			sql += ' WHERE word=?;'
		else:
			sql += ' WHERE id=?;'
		try:
			self.__conn.execute(sql, tuple(values + [key]))
			if commit:
				self.__conn.commit()
		except sqlite3.IntegrityError:
			return False
		return True

	# 提交变更
	def commit (self):
		try:
			self.__conn.commit()
		except sqlite3.IntegrityError:
			self.__conn.rollback()
			return False
		return True


#----------------------------------------------------------------------
# startup MySQLdb
#----------------------------------------------------------------------
def mysql_startup():
	global MySQLdb
	if MySQLdb is not None:
		return True
	try:
		import MySQLdb as _mysql
		MySQLdb = _mysql
	except ImportError:
		return False
	return True


#----------------------------------------------------------------------
# DictMysql
#----------------------------------------------------------------------
class DictMySQL (object):

	def __init__ (self, **argv):
		self.__argv = {}
		self.__uri = {}
		for k, v in argv.items():
			self.__argv[k] = v
			if not k in ('engine', 'init', 'db', 'verbose'):
				self.__uri[k] = v
		self.__uri['connect_timeout'] = self.__uri.get('connect_timeout', 10)
		self.__conn = None
		self.__verbose = argv.get('verbose', False)
		if not 'db' in argv:
			raise KeyError('not find db name')
		self.__open()
	
	def __open (self):
		mysql_startup()
		if MySQLdb is None:
			raise ImportError('No module named MySQLdb')

		self.__fields = [ 'id', 'word', 'phonetic', 'definition', 
			'translation', 'pos', 'collins', 'oxford', 'tag', 'bnc', 'frq', 
			'tense', 'plural', 'detail', 'audio', 'audio_uk', 'audio_us' ]
		fields = self.__fields
		self.__fields = [ (fields[i], i) for i in range(len(fields)) ]
		self.__names = { }
		for k, v in self.__fields:
			self.__names[k] = v
		self.__enable = self.__fields[2:]

		init = self.__argv.get('init', False)
		self.__db = self.__argv.get('db', 'stardict')
		if not init:
			uri = {}
			for k, v in self.__uri.items():
				uri[k] = v
			uri['db'] = self.__db
			self.__conn = MySQLdb.connect(**uri)
		else:
			self.__conn = MySQLdb.connect(**self.__uri)
			return self.init()
		return True

	# 输出日志
	def out (self, text):
		if self.__verbose:
			print(text)
		return True

	# 初始化数据库与表格
	def init (self):
		database = self.__argv.get('db', 'stardict')
		self.out('create database: %s'%database)
		self.__conn.query("SET sql_notes = 0;")
		self.__conn.query('CREATE DATABASE IF NOT EXISTS %s;'%database)
		self.__conn.query('USE %s;'%database)
		# self.__conn.query('drop table if exists stardict')
		sql = '''
			CREATE TABLE IF NOT EXISTS `%s`.`stardict` (
			`id` INT PRIMARY KEY NOT NULL AUTO_INCREMENT,
			`word` VARCHAR(64) NOT NULL UNIQUE KEY,
			`phonetic` VARCHAR(64),
			`definition` TEXT,
			`translation` TEXT,
			`pos` VARCHAR(16),
			`collins` SMALLINT DEFAULT 0,
			`oxford` SMALLINT DEFAULT 0,
			`tag` VARCHAR(64),
			`bnc` INT DEFAULT NULL,
			`frq` INT DEFAULT NULL,
			`tense` TEXT,
			`plural` VARCHAR(64),
			`detail` TEXT,
			`audio` VARCHAR(128),
			`audio_uk` VARCHAR(128),
			`audio_us` VARCHAR(128),
			KEY(`collins`),
			KEY(`oxford`),
			KEY(`tag`)
			)
			'''%(database)
		sql = '\n'.join([ n.strip('\t') for n in sql.split('\n') ])
		sql = sql.strip('\n')
		sql+= ' ENGINE=MyISAM DEFAULT CHARSET=utf8;'
		self.__conn.query(sql)
		self.__conn.commit()
		return True

	# 数据库记录转化为字典
	def __record2obj (self, record):
		if record is None:
			return None
		word = {}
		for k, v in self.__fields:
			word[k] = record[v]
		if word['detail'] is not None:
			word['detail'] = json.loads(word['detail'])
		return word

	# 关闭数据库
	def close (self):
		if self.__conn:
			self.__conn.close()
		self.__conn = None
	
	def __del__ (self):
		self.close()

	# 查询单词
	def query (self, key):
		record = None
		if isinstance(key, int) or isinstance(key, long):
			sql = 'select * from stardict where id = %s;'
		elif isinstance(key, str) or isinstance(key, unicode):
			sql = 'select * from stardict where word = %s;'
		else:
			return None
		with self.__conn as c:
			c.execute(sql, (key,))
			record = c.fetchone()
		return self.__record2obj(record)

	# 查询单词匹配
	def match (self, word, limit = 10):
		c = self.__conn.cursor()
		sql = 'select id, word from stardict where word like %s '
		sql += 'order by word limit %s;'
		c.execute(sql, (word + '%', limit))
		records = c.fetchall()
		result = []
		for record in records:
			result.append(tuple(record))
		return result

	# 批量查询
	def query_batch (self, keys):
		sql = 'select * from stardict where '
		if keys is None:
			return None
		if not keys:
			return []
		querys = []
		for key in keys:
			if isinstance(key, int) or isinstance(key, long):
				querys.append('id = %s')
			elif key is not None:
				querys.append('word = %s')
		sql = sql + ' or '.join(querys) + ';'
		query_word = {}
		query_id = {}
		with self.__conn as c:
			c.execute(sql, tuple(keys))
			for row in c:
				obj = self.__record2obj(row)
				query_word[obj['word']] = obj
				query_id[obj['id']] = obj
		results = []
		for key in keys:
			if isinstance(key, int) or isinstance(key, long):
				results.append(query_id.get(key, None))
			elif key is not None:
				results.append(query_word.get(key, None))
			else:
				results.append(None)
		return tuple(results)

	# 注册新单词
	def register (self, word, items, commit = True):
		sql = 'INSERT INTO stardict(word) VALUES(%s);';
		try:
			with self.__conn as c:
				c.execute(sql, (word,))
		except MySQLdb.Error as e:
			self.out(str(e))
			return False
		self.update(word, items, commit)
		return True

	# 删除单词
	def remove (self, key, commit = True):
		if isinstance(key, int) or isinstance(key, long):
			sql = 'DELETE FROM stardict WHERE id=%s;'
		else:
			sql = 'DELETE FROM stardict WHERE name=%s;'
		try:
			with self.__conn as c:
				c.execute(sql, (key,))
		except MySQLdb.Error as e:
			self.out(str(e))
			return False
		return True

	# 清空数据库
	def delete_all (self, reset_id = False):
		sql1 = 'DELETE FROM stardict;'
		try:
			self.__cursor.execute(sql1)
			self.__conn.commit()
		except MySQLdb.Error as e:
			self.out(str(e))
			return False
		return True

	# 更新单词数据
	def update (self, key, items, commit = True):
		names = []
		values = []
		for name, id in self.__enable:
			if name in items:
				names.append(name)
				value = items[name]
				if name == 'detail':
					if value is not None:
						value = json.dumps(value, ensure_ascii = False)
				values.append(value)
		if len(names) == 0:
			if commit:
				try:
					self.__conn.commit()
				except MySQLdb.Error as e:
					self.out(str(e))
					return False
			return False
		sql = 'UPDATE stardict SET ' + ', '.join(['%s=%%s'%n for n in names])
		if isinstance(key, str) or isinstance(key, unicode):
			sql += ' WHERE word=%s;'
		else:
			sql += ' WHERE id=%s;'
		try:
			with self.__conn as c:
				c.execute(sql, tuple(values + [key]))
		except MySQLdb.Error as e:
			self.out(str(e))
			return False
		return True



#----------------------------------------------------------------------
# testing
#----------------------------------------------------------------------
if __name__ == '__main__':
	db = os.path.join(os.path.dirname(__file__), 'stardict.db')
	my = {'host':'xnode3.ddns.net', 'user':'skywind', 'passwd':'678900', 'db':'skywind_t9'}
	def test1():
		t = time.time()
		sd = StarDict(db)
		print(time.time() - t)
		sd.delete_all(True)
		print(sd.register('kiss2', {'definition':'kiss me'}, False))
		print(sd.register('kiss here', {'definition':'kiss me'}, False))
		print(sd.register('kiss', {'definition':'kiss me'}, False))
		print(sd.register('kiss', {'definition':'kiss me'}, False))
		print(sd.register('suck', {'definition':'suck me'}, False))
		sd.commit()
		print('')
		print(sd.count())
		print(sd.query('kiss'))
		print(sd.query(2))
		print(sd.match('kis'))
		mysql_startup()
		print('mysql: %s'%MySQLdb)
		print('')
		print(sd.query_batch(['kiss', 2]))
		return 0
	def test2():
		t = time.time()
		dm = DictMySQL(init = True, **my)
		print(time.time() - t)
		# dm.delete_all(True)
		print(dm.register('kiss2', {'definition':'kiss me'}, False))
		print(dm.register('kiss here', {'definition':'kiss me'}, False))
		print(dm.register('kiss', {'definition':'kiss me'}, False))
		print(dm.register('kiss', {'definition':'kiss me'}, False))
		print(dm.register('suck', {'definition':'suck me'}, False))
		print(dm.query('kiss'))
		print(dm.match('kis'))
		print('')
		print(dm.query_batch(['kiss', 2, 9]))
		return 0
	test2()



