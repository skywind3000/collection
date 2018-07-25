#! /usr/bin/env python
# -*- coding: utf-8 -*-
#  vim: set ts=4 sw=4 tw=0 noet :
#======================================================================
#
# accounts.py - 
#
# Created by skywind on 2017/03/16
# Last change: 2017/03/16 11:58:41
#
#======================================================================
from __future__ import print_function
import sys
import time
import os
import datetime
import sqlite3

try:
	import json
except:
	import simplejson as json

MySQLdb = None
pymongo = None


#----------------------------------------------------------------------
# python3 compatible
#----------------------------------------------------------------------
if sys.version_info[0] >= 3:
	unicode = str
	long = int
	xrange = range


#----------------------------------------------------------------------
# AccountLocal
#----------------------------------------------------------------------
class AccountLocal (object):

	def __init__ (self, filename):
		self.__dbname = os.path.abspath(filename)
		self.__conn = None
		self.__open()
		self.mode = 0

	def __open (self):
		sql = '''
		CREATE TABLE IF NOT EXISTS "account" (
		    "uid" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,
		    "urs" VARCHAR(88) NOT NULL UNIQUE,
		    "cid" INTEGER DEFAULT (0),
		    "name" VARCHAR(32) NOT NULL DEFAULT(''),			
		    "pass" VARCHAR(64) NOT NULL DEFAULT(''),
		    "gender" INTEGER DEFAULT (0),
			"credit" REAL DEFAULT (0),
		    "gold" REAL DEFAULT (0),
		    "level" INTEGER DEFAULT (0),
			"exp" INTEGER DEFAULT (0),
			"birthday" DATE,
		    "icon" INTEGER DEFAULT (0),
		    "mail" VARCHAR(88),
		    "mobile" VARCHAR(32), 
			"sign" VARCHAR(32),			
		    "photo" VARCHAR(256),
			"intro" VARCHAR(256),
			"misc" TEXT,
			"src" VARCHAR(16),
			"ip" VARCHAR(70),
		    "RegDate" DATETIME,
		    "LastLoginDate" DATETIME,
		    "LoginTimes" INTEGER DEFAULT (0),
			"CreditConsumed" REAL DEFAULT (0),
		    "GoldConsumed" REAL DEFAULT (0)
		);
		CREATE UNIQUE INDEX IF NOT EXISTS "account_1" ON account (uid);
		CREATE UNIQUE INDEX IF NOT EXISTS "account_2" ON account (urs, pass);
		CREATE INDEX IF NOT EXISTS "account_3" ON account (cid);
		'''

		c = sqlite3.connect(self.__dbname, isolation_level = 'IMMEDIATE')
		c.isolation_level = 'IMMEDIATE'

		self.__conn = c

		sql = '\n'.join([ n.strip('\t') for n in sql.split('\n') ])
		sql = sql.strip('\n')

		# self.__conn.execute('drop table if exists account;')
		self.__conn.executescript(sql)
		self.__conn.commit()
	
		fields = ( 'uid', 'urs', 'cid', 'name', 'pass', 'gender', 'credit',
			'gold', 'level', 'exp', 'birthday', 'icon', 'mail', 'mobile',
			'sign', 'photo', 'intro', 'misc', 'src', 'ip', 'RegDate',
			'LastLoginDate', 'LoginTimes', 'CreditConsumed', 
			'GoldConsumed' )

		self.__names = {}
		self.__items = []

		for i, v in enumerate(fields):
			self.__names[v] = i
			self.__items.append((v, i))

		self.__items = tuple(self.__items)

		x = ('cid', 'name', 'pass', 'gender', 'icon', 'mail', 'mobile', 
			'photo', 'misc', 'level', 'exp', 'birthday', 'sign',
			'intro', 'src')
		self.__enable = {}
		for n in x:
			self.__enable[n] = self.__names[n]

		return True

	# 将数据库记录转化为字典
	def __record2obj (self, record):
		if record is None:
			return None
		user = {}
		for k, i in self.__items:
			v = record[i]
			if k == 'misc':
				if v:
					try:
						user[k] = json.loads(v)
					except:
						user[k] = None
				else:
					user[k] = None
			elif k != 'pass':
				user[k] = v
		return user

	# 关闭数据库连接
	def close (self):
		if self.__conn:
			self.__conn.close()
		self.__conn = None

	# 关闭数据库连接
	def __del__ (self):
		self.close()

	# 登录，输入用户名和密码，返回用户数据，密码为 None的话强制登录
	def login (self, urs, passwd, ip = None):
		sql1 = 'select * from account where urs = ? and pass = ?;'
		sql2 = 'select * from account where urs = ?;'
		record = None
		c = self.__conn.cursor()
		try:
			if passwd is not None:
				c.execute(sql1, (urs, passwd))
			else:
				c.execute(sql2, (urs,))
			record = c.fetchone()
			if record is None:
				c.close()
				return None
		except sqlite3.IntegrityError:
			c.close()
			return None
		c.close()
		sql = "update account set "
		sql += "LastLoginDate = datetime('now', 'localtime'),"
		sql += "LoginTimes = LoginTimes + 1,"
		sql += "ip = ?"
		if self.mode == 0:
			try:
				self.__conn.execute(sql + ' where urs = ?;', (ip, urs))
				self.__conn.commit()
			except sqlite3.IntegrityError:
				self.__conn.rollback()
				return None
		return self.__record2obj(record)

	# 查询用户信息：
	#   以 urs读取信息：urs != None, uid == None
	#   以 uid读取信息：urs == None, uid != None
	#   验证 urs/uid匹配：urs != None, uid != None
	# 成功返回用户记录，失败返回 None
	def query (self, urs = None, uid = None):
		if urs is None and uid is None:
			return None
		c = self.__conn.cursor()
		record = None
		try:
			if urs is not None and uid is None:
				c.execute('select * from account where urs = ?;', (urs,))
			elif urs is None and uid is not None:
				c.execute('select * from account where uid = ?;', (uid,))
			else:
				x = 'select * from account where urs = ? and uid = ?;'
				c.execute(x, (urs, uid))
			record = c.fetchone()
		except:
			c.close()
			return None
		return self.__record2obj(record)

	# 用户注册，返回记录
	def register (self, urs, passwd, name, gender = 0, src = None):
		sql = 'INSERT INTO account(urs, pass, name, gender, src, RegDate) '
		sql += "VALUES(?, ?, ?, ?, ?, datetime('now', 'localtime'));"
		try:
			self.__conn.execute(sql, (urs, passwd, name, gender, src))
			self.__conn.commit()
		except sqlite3.IntegrityError:
			self.__conn.rollback()
			return None
		return self.query(urs)
	
	# 用户更新资料, changes是一个字典格式和 query返回相同，允许设置字段有：
	# cid, name, pass, gender, icon, mail, mobile, photo, misc, level, 
	# score, intro, sign, birthday
	def update (self, uid, changes):
		names, values = [], []
		for k in changes:
			if k not in self.__enable:
				continue
			v = changes[k]
			if k == 'misc':
				if v is not None:
					v = json.dumps(v, ensure_ascii = False)
			names.append(k)
			values.append(v)
		if not values:
			return False
		sql = 'UPDATE account SET ' + ', '.join(['%s=?'%n for n in names])
		sql += ' WHERE uid=%d;'%uid
		try:
			self.__conn.execute(sql, tuple(values))
			self.__conn.commit()
		except sqlite3.IntegrityError:
			self.__conn.rollback()
			return False
		return True

	# 更新或者验证密码
	# old == None, passwd != None -> 重置密码
	# old != None, passwd == None -> 验证密码
	# old != None, passwd != None -> 修改密码
	def passwd (self, uid, old, passwd = None):
		if old is None and passwd is None:
			return False
		if old is not None:
			c = self.__conn.cursor()
			if isinstance(uid, int) or isinstance(uid, long):
				sql = 'SELECT * FROM account WHERE uid=? and pass=?;'
			else:
				sql = 'SELECT * FROM account WHERE urs=? and pass=?;'
			try:
				c.execute(sql, (uid, old))
				record = c.fetchone()
				if record is None:
					c.close()
					return False
			except sqlite3.IntegrityError:
				c.close()
				return False
		if passwd is not None and passwd != old:
			if isinstance(uid, int) or isinstance(uid, long):
				sql = 'UPDATE account SET pass=? WHERE uid=?;'
			else:
				sql = 'UPDATE account SET pass=? WHERE urs=?;'
			try:
				self.__conn.execute(sql, (passwd, uid))
				self.__conn.commit()
			except sqlite3.IntegrityError:
				self.__conn.rollback()
				return False
		return True

	# 支付钱，kind为 'credit'或 'gold'，money是需要支付的钱数
	# 返回 (结果, 还有多少钱, 错误原因) 
	# 结果=0支付成功，结果=1用户不存在，结果=2钱不够，结果=3未知错误
	def payment (self, uid, kind, money):
		kind = kind.lower()
		if kind not in ('credit', 'gold'):
			return (-1, 0, 'money kind error %s'%kind)
		if kind == 'credit':
			x1, x2 = ('credit', 'CreditConsumed')
		else:
			x1, x2 = ('gold', 'GoldConsumed')
		sql = 'UPDATE account SET %s=%s-?, %s=%s+? WHERE uid=? and %s>=?;'
		sql = sql%(x1, x1, x2, x2, x1)
		changes = self.__conn.total_changes
		try:
			self.__conn.execute(sql, (money, money, uid, money))
			self.__conn.commit()
		except sqlite3.IntegrityError:
			self.__conn.rollback()
		changed = self.__conn.total_changes - changes
		data = self.query(None, uid)
		if data is None:
			return (1, 0, 'bad uid %d'%uid)
		if changed == 0:
			if data[x1] < money:
				return (2, data[x1], 'not enough %s'%x1)
			return (3, data[x1], 'unknow payment error')
		return (0, data[x1], 'ok')
	
	# 存钱，kind为 'credit'或 'gold'，money是需要增加的钱数
	def deposit (self, uid, kind, money):
		kind = kind.lower()
		if kind not in ('credit', 'gold'):
			return (-1, 0, 'money kind error %s'%kind)
		sql = 'UPDATE account SET %s=%s+? WHERE uid=?'%(kind, kind)
		changes = self.__conn.total_changes
		try:
			self.__conn.execute(sql, (money, uid))
			self.__conn.commit()
		except:
			self.__conn.rollback()
		changed = self.__conn.total_changes - changes
		data = self.query(None, uid)
		if data is None:
			return (1, 0, 'bad uid %d'%uid)
		if changed == 0:
			return (2, data[kind], 'unknow deposit error')
		return (0, data[kind], 'ok')

	# 向数据库插入随机记录，用于测试
	def population (self, count = 100):
		x = 'INSERT INTO account(urs, name, pass, gender, RegDate) '
		y = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
		succeed = 0
		for i in xrange(count):
			z = "VALUES('10%d@qq.com', 'name%d', '****', '%d', '%s');"%( 
				i + 1, i + 1, i % 3, y)
			sql = x + z
			try:
				self.__conn.execute(sql)
				succeed += 1
			except:
				pass
		self.__conn.commit()
		return succeed



#----------------------------------------------------------------------
# initialize MySQLdb 
#----------------------------------------------------------------------
def mysql_init():
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
# AccountMySQL
#----------------------------------------------------------------------
class AccountMySQL (object):

	def __init__ (self, **argv):
		self.__argv = {}
		self.__uri = {}
		for k, v in argv.items():
			self.__argv[k] = v
			if k not in ('init', 'db', 'verbose'):
				self.__uri[k] = v
		self.__uri['connect_timeout'] = argv.get('connect_timeout', 10)
		self.__conn = None
		self.__verbose = argv.get('verbose', False)
		if 'db' not in argv:
			raise KeyError('not find db name')
		self.__open()
		self.mode = 0
	
	def __open (self):
		mysql_init()
		if MySQLdb is None:
			raise ImportError('No module named MySQLdb')

		fields = ( 'uid', 'urs', 'cid', 'name', 'pass', 'gender', 'credit',
			'gold', 'level', 'exp', 'birthday', 'icon', 'mail', 'mobile',
			'sign', 'photo', 'intro', 'misc', 'src', 'ip', 'RegDate',
			'LastLoginDate', 'LoginTimes', 'CreditConsumed', 
			'GoldConsumed' )

		self.__names = {}
		self.__items = []

		for i in range(len(fields)):
			self.__names[fields[i]] = i
			self.__items.append((fields[i], i))

		self.__items = tuple(self.__items)

		x = ('cid', 'name', 'pass', 'gender', 'icon', 'mail', 'mobile', 
			'photo', 'misc', 'level', 'exp', 'birthday', 'sign',
			'intro', 'src')
		self.__enable = {}
		for n in x:
			self.__enable[n] = self.__names[n]

		init = self.__argv.get('init', False)
		self.__db = self.__argv.get('db', 'account')
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
		database = self.__argv.get('db', 'account')
		self.out('create database: %s'%database)
		self.__conn.query("SET sql_notes = 0;")
		self.__conn.query('CREATE DATABASE IF NOT EXISTS %s;'%database)
		self.__conn.query('USE %s;'%database)
		# self.__conn.query('drop table if exists account;')
		self.__conn.commit()
		sql = '''
			CREATE TABLE IF NOT EXISTS `%s`.`account` (
		    `uid` INT PRIMARY KEY NOT NULL AUTO_INCREMENT,
		    `urs` VARCHAR(88) NOT NULL UNIQUE KEY,
		    `cid` INT DEFAULT 0,
		    `name` VARCHAR(32) NOT NULL DEFAULT '',			
		    `pass` VARCHAR(64) NOT NULL DEFAULT '',
		    `gender` SMALLINT DEFAULT 0,
			`credit` REAL DEFAULT 0,
		    `gold` REAL DEFAULT 0,
		    `level` INT DEFAULT 0,
			`exp` INT DEFAULT 0,
			`birthday` DATE,
		    `icon` INT DEFAULT 0,
		    `mail` VARCHAR(88),
		    `mobile` VARCHAR(32), 
			`sign` VARCHAR(32),			
		    `photo` VARCHAR(256),
			`intro` VARCHAR(256),
			`misc` TEXT,
			`src` VARCHAR(16),
			`ip` VARCHAR(70),
		    `RegDate` DATETIME,
		    `LastLoginDate` DATETIME,
		    `LoginTimes` INT DEFAULT 0,
			`CreditConsumed` REAL DEFAULT 0,
		    `GoldConsumed` REAL DEFAULT 0,
			KEY(`cid`),
			KEY(`name`),
			KEY(`src`)
			)
		'''%(database)
		sql = '\n'.join([ n.strip('\t') for n in sql.split('\n') ])
		sql = sql.strip('\n')
		sql += ' ENGINE=InnoDB DEFAULT CHARSET=utf8;'
		self.__conn.query(sql)
		self.__conn.commit()
		return True

	# 将数据库记录转化为字典
	def __record2obj (self, record):
		if record is None:
			return None
		user = {}
		for k, i in self.__items:
			v = record[i]
			if k == 'misc':
				if v:
					try:
						user[k] = json.loads(v)
					except:
						user[k] = None
				else:
					user[k] = None
			elif k != 'pass':
				user[k] = v
		return user

	# 关闭数据库连接
	def close (self):
		if self.__conn:
			self.__conn.close()
		self.__conn = None

	# 关闭数据库连接
	def __del__ (self):
		self.close()

	# 登录，输入用户名和密码，返回用户数据，密码为 None的话强制登录
	def login (self, urs, passwd, ip = None):
		sql1 = 'select * from account where urs = %s and pass = %s;'
		sql2 = 'select * from account where urs = %s;'
		record = None
		try:
			with self.__conn as c:
				if passwd is not None:
					c.execute(sql1, (urs, passwd))
				else:
					c.execute(sql2, (urs,))
				record = c.fetchone()
			if record is None:
				return None
		except MySQLdb.Error:
			return None
		sql = "update account set "
		sql += "LastLoginDate = Now(),"
		sql += "LoginTimes = LoginTimes + 1,"
		sql += "ip = %s where urs = %s;"
		if self.mode == 0:
			try:
				with self.__conn as c:
					c.execute(sql, (ip, urs))
			except MySQLdb.Error:
				return None
		return self.__record2obj(record)

	# 查询用户信息：
	#   以 urs读取信息：urs != None, uid == None
	#   以 uid读取信息：urs == None, uid != None
	#   验证 urs/uid匹配：urs != None, uid != None
	# 成功返回用户记录，失败返回 None
	def query (self, urs = None, uid = None):
		if urs is None and uid is None:
			return None
		record = None
		try:
			with self.__conn as c:
				if urs is not None and uid is None:
					c.execute('select * from account where urs = %s', (urs,))
				elif urs is None and uid is not None:
					c.execute('select * from account where uid = %s', (uid,))
				else:
					x = 'select * from account where urs = %s and uid = %s'
					c.execute(x, (urs, uid))
				record = c.fetchone()
		except:
			return None
		return self.__record2obj(record)

	# 用户注册，返回记录
	def register (self, urs, passwd, name, gender = 0, src = None):
		sql = 'INSERT INTO account(urs, pass, name, gender, src, RegDate) '
		sql += "VALUES(%s, %s, %s, %s, %s, Now());"
		try:
			with self.__conn as c:
				c.execute(sql, (urs, passwd, name, gender, src))
		except MySQLdb.Error:
			return None
		return self.query(urs)
	
	# 用户更新资料, changes是一个字典格式和 query返回相同，允许设置字段有：
	# cid, name, pass, gender, icon, mail, mobile, photo, misc, level, 
	# score, intro, sign, birthday
	def update (self, uid, changes):
		names, values = [], []
		for k in changes:
			if k not in self.__enable:
				continue
			v = changes[k]
			if k == 'misc':
				if v is not None:
					v = json.dumps(v, ensure_ascii = False)
			names.append(k)
			values.append(v)
		if not values:
			return False
		sql = 'UPDATE account SET ' + ', '.join(['%s=%%s'%n for n in names])
		sql += ' WHERE uid=%d;'%uid
		try:
			with self.__conn as c:
				c.execute(sql, tuple(values))
		except MySQLdb.Error:
			return False
		return True

	# 更新或者验证密码
	# old == None, passwd != None -> 重置密码
	# old != None, passwd == None -> 验证密码
	# old != None, passwd != None -> 修改密码
	def passwd (self, uid, old, passwd = None):
		if old is None and passwd is None:
			return False
		if old is not None:
			if isinstance(uid, int) or isinstance(uid, long):
				sql = 'SELECT * FROM account WHERE uid=%s and pass=%s;'
			else:
				sql = 'SELECT * FROM account WHERE urs=%s and pass=%s;'
			try:
				with self.__conn as c:
					c.execute(sql, (uid, old))
					record = c.fetchone()
					if record is None:
						return False
			except MySQLdb.Error:
				return False
		if passwd is not None and passwd != old:
			if isinstance(uid, int) or isinstance(uid, long):
				sql = 'UPDATE account SET pass=%s WHERE uid=%s;'
			else:
				sql = 'UPDATE account SET pass=%s WHERE urs=%s;'
			try:
				with self.__conn as c:
					c.execute(sql, (passwd, uid))
			except MySQLdb.Error:
				return False
		return True

	# 支付钱，kind为 'credit'或 'gold'，money是需要支付的钱数
	# 返回 (结果, 还有多少钱, 错误原因) 
	# 结果=0支付成功，结果=1用户不存在，结果=2钱不够，结果=3未知错误
	def payment (self, uid, kind, money):
		kind = kind.lower()
		if kind not in ('credit', 'gold'):
			return (-1, 0, 'money kind error %s'%kind)
		if kind == 'credit':
			x1, x2 = ('credit', 'CreditConsumed')
		else:
			x1, x2 = ('gold', 'GoldConsumed')
		sql = 'UPDATE account SET %s=%s-?, %s=%s+? WHERE uid=? and %s>=?;'
		sql = (sql%(x1, x1, x2, x2, x1)).replace('?', '%s')
		changed = 0
		try:
			with self.__conn as c:
				changed = c.execute(sql, (money, money, uid, money))
		except MySQLdb.Error:
			pass
		data = self.query(None, uid)
		if data is None:
			return (1, 0, 'bad uid %d'%uid)
		if changed == 0:
			if data[x1] < money:
				return (2, data[x1], 'not enough %s'%x1)
			return (3, data[x1], 'unknow payment error')
		return (0, data[x1], 'ok')
	
	# 存钱，kind为 'credit'或 'gold'，money是需要增加的钱数
	def deposit (self, uid, kind, money):
		kind = kind.lower()
		if kind not in ('credit', 'gold'):
			return (-1, 0, 'money kind error %s'%kind)
		sql = 'UPDATE account SET %s=%s+? WHERE uid=?'%(kind, kind)
		sql = sql.replace('?', '%s')
		changed = 0
		try:
			with self.__conn as c:
				changed = c.execute(sql, (money, uid))
		except MySQLdb.Error:
			pass
		data = self.query(None, uid)
		if data is None:
			return (1, 0, 'bad uid %d'%uid)
		if changed == 0:
			return (2, data[kind], 'unknow deposit error')
		return (0, data[kind], 'ok')

	# 向数据库插入随机记录，用于测试
	def population (self, count = 100):
		x = 'INSERT INTO account(urs, name, pass, gender, RegDate) '
		y = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
		succeed = 0
		with self.__conn as c:
			for i in xrange(count):
				z = "VALUES('10%d@qq.com', 'name%d', '****', '%d', '%s');"
				z = z%(i + 1, i + 1, i % 3, y)
				sql = x + z
				try:
					c.execute(sql)
					succeed += 1
				except:
					pass
		self.__conn.commit()
		return succeed



#----------------------------------------------------------------------
# initialize mongodb client
#----------------------------------------------------------------------
def pymongo_init():
	global pymongo
	if pymongo is not None:
		return True
	try:
		import pymongo as _pymongo
		pymongo = _pymongo
	except ImportError:
		return False
	return True


#----------------------------------------------------------------------
# AccountMongo
#----------------------------------------------------------------------
class AccountMongo (object):

	def __init__ (self, url, init = False):
		self.__config = self.__url_parse(url)
		self.__url = url
		self.__client = None
		self.__db = None
		self.__bad = False
		self.__open()
		self.__account = self.__db.account
		self.__seqs = self.__db['user.seqs']
		if init:
			self.init()
		self.__setting()	

	# 解析 mongo url: mongodb://user:pass@abc.com/database?key=val
	def __url_parse (self, url):
		url = url.strip('\r\n\t ')
		chk = 'mongodb://'
		if url[:len(chk)] != chk:
			raise ValueError('bad protocol: %s'%url)
		config = {}
		config['url'] = url
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
	def __open (self):
		if self.__db is not None:
			return self.__db
		pymongo_init()
		if self.__client is None:
			if self.__bad:
				return None
			url = self.__config['url']
			self.__client = pymongo.MongoClient(url)
		if self.__client is None:
			self.__bad = True
			return None
		db = self.__client[self.__config['db']]
		if not db:
			return None
		self.__db = db
		return self.__db

	# 关闭数据库和客户端连接
	def close (self):
		if self.__db:
			self.__db = None
		if self.__account:
			self.__account = None
		if self.__seqs:
			self.__seqs = None
		if self.__client:
			try:
				self.__client.close()
			except:
				pass
			self.__client = None
		return True

	# 删除自身
	def __del__ (self):
		self.close()

	# 初始化字段
	def __setting (self):
		fields = ( 'uid', 'urs', 'cid', 'name', 'pass', 'gender', 'credit',
			'gold', 'level', 'exp', 'birthday', 'icon', 'mail', 'mobile',
			'sign', 'photo', 'intro', 'misc', 'src', 'ip', 'RegDate',
			'LastLoginDate', 'LoginTimes', 'CreditConsumed', 
			'GoldConsumed' )
		self.__names = {}
		self.__items = []

		for i, v in fields:
			self.__names[v] = i
			self.__items.append((v, i))

		self.__items = tuple(self.__items)

		x = ('cid', 'name', 'pass', 'gender', 'icon', 'mail', 'mobile', 
			'photo', 'misc', 'level', 'exp', 'birthday', 'sign',
			'intro', 'src')
		self.__enable = {}
		for n in x:
			self.__enable[n] = self.__names[n]
		return True

	# 字段补充
	def __obj_complete (self, obj):
		newobj = {}
		for k in self.__names:
			if k != 'pass':
				newobj[k] = obj.get(k, None)
		if '_id' in obj:
			newobj['_id'] = obj['_id']
		if newobj['uid'] is None:
			newobj['uid'] = 0
		return newobj

	# 自增量
	def __id_auto_increment (self, name):
		seqs = self.__seqs
		cc = seqs.find_and_modify(
			query = {'_id': name},
			update = {'$inc': {'next': 1}},
			fields = {'next':1},
			new = True,
			upsert = True)
		return cc.get('next', 1)

	# 初始化
	def init (self):
		account = self.__account
		account.ensure_index('uid', unique = True, background = True)
		account.ensure_index('urs', unique = True, background = True)
		account.ensure_index('name', unique = False, background = True)
		return True

	# 登录，输入用户名和密码，返回用户数据，密码为 None的话强制登录
	def login (self, urs, passwd, ip = None):
		account = self.__account
		if passwd is None:
			cc = account.find_one({'urs':urs})
		else:
			cc = account.find_one({'urs':urs, 'pass':passwd})
		if cc is None:
			return None
		account.update_one({'_id': cc['_id'], 'urs': cc['urs']}, 
				{'$set': {'ip':ip, 
					'LastLoginDate':datetime.datetime.now()},
				 '$inc': {'LoginTimes':1}})
		cc = self.__obj_complete(cc)
		del cc['_id']
		return cc

	# 查询用户信息：
	#   以 urs读取信息：urs != None, uid == None
	#   以 uid读取信息：urs == None, uid != None
	#   验证 urs/uid匹配：urs != None, uid != None
	# 成功返回用户记录，失败返回 None
	def query (self, urs = None, uid = None):
		if urs is None and uid is None:
			return None
		account = self.__account
		if urs is not None and uid is None:
			cc = account.find_one({'urs':urs})
		elif urs is None and uid is not None:
			cc = account.find_one({'uid':uid})
		else:
			cc = account.find_one({'uid':uid, 'urs':urs})
		if cc is None:
			return None
		cc = self.__obj_complete(cc)
		if '_id' in cc:
			del cc['_id']
		return cc

	# 注册用户，返回记录
	def register (self, urs, passwd, name, gender = 0, src = None):
		account = self.__account
		cc = account.find_one({'urs': urs})
		if cc is not None:
			return None
		cc = self.__obj_complete({})
		cc['uid'] = self.__id_auto_increment('account')
		cc['urs'] = urs
		cc['name'] = name
		cc['pass'] = passwd
		cc['gender'] = gender
		cc['src'] = src
		cc['LoginTimes'] = 0
		cc['credit'] = 0.0
		cc['gold'] = 0.0
		cc['level'] = 0
		cc['exp'] = 0
		cc['CreditConsumed'] = 0.0
		cc['GoldConsumed'] = 0.0
		cc['RegDate'] = datetime.datetime.now()
		key = {'uid':cc['uid'], 'urs':cc['urs']}
		try:
			account.update(key, cc, upsert = True)
		except pymongo.errors.DuplicateKeyError:
			return None
		return self.query(urs = urs)

	# 用户更新资料, changes是一个字典格式和 query返回相同，允许设置字段有：
	# cid, name, pass, gender, icon, mail, mobile, photo, misc, level, 
	# score, intro, sign, birthday
	def update (self, uid, changes):
		update = {}
		account = self.__account
		for name in self.__enable:
			if name in changes:
				update[name] = changes[name]
		account.update_one({'uid':uid}, {'$set':update}, upsert = False)
		return True

	# 更新或者验证密码
	# old == None, passwd != None -> 重置密码
	# old != None, passwd == None -> 验证密码
	# old != None, passwd != None -> 修改密码
	def passwd (self, uid, old, passwd = None):
		if old is None and passwd is None:
			return False
		account = self.__account
		if old is not None:
			if isinstance(uid, int) or isinstance(uid, long):
				cc = account.find_one({'uid':uid, 'pass':old})
			else:
				cc = account.find_one({'urs':uid, 'pass':old})
			if cc is None:
				return False
		if passwd is not None and passwd != old:
			query = {}
			if isinstance(uid, int) or isinstance(uid, long):
				query['uid'] = uid
			else:
				query['urs'] = uid
			update = {'$set':{'pass':passwd}}
			account.update_one(query, update, upsert = False)
		return True

	# 支付钱，kind为 'credit'或 'gold'，money是需要支付的钱数
	# 返回 (结果, 还有多少钱, 错误原因) 
	# 结果=0支付成功，结果=1用户不存在，结果=2钱不够，结果=3未知错误
	def payment (self, uid, kind, money):
		kind = kind.lower()
		if kind not in ('credit', 'gold'):
			return (-1, 0, 'money kind error %s'%kind)
		query = {'uid':uid}
		inc = {}
		if kind == 'credit':
			query['credit'] = {'$gte':money}
			inc['credit'] = -money
			inc['CreditConsumed'] = money
		else:
			query['gold'] = {'$gte':money}
			inc['gold'] = -money
			inc['GoldConsumed'] = money
		update = {'$inc':inc}
		hh = self.__account.find_and_modify(query, update)
		data = self.query(None, uid)
		if data is None:
			return (1, 0, 'bad uid %d'%uid)
		if hh is None:
			if data[kind] < money:
				return (2, data[kind], 'not enough %s'%kind)
			return (3, data[kind], 'unknow payment error')
		return (0, data[kind], 'ok')

	# 存钱，kind为 'credit'或 'gold'，money是需要增加的钱数
	def deposit (self, uid, kind, money):
		kind = kind.lower()
		if kind not in ('credit', 'gold'):
			return (-1, 0, 'money kind error %s'%kind)
		query = {'uid':uid}
		inc = {}
		if kind == 'credit':
			inc['credit'] = money
		else:
			inc['gold'] = money
		update = {'$inc':inc}
		hh = self.__account.find_and_modify(query, update)
		if hh is None:
			return (1, 0, 'bad uid %d'%uid)
		data = self.query(uid = uid)
		if data is None:
			return (2, 0, 'unknow payment error')
		return (0, data[kind], 'ok')

	# 向数据库插入随机记录，用于测试
	def population (self, count = 100):
		succeed = 0
		for i in xrange(count):
			urs = '10%d@qq.com'%i
			name = 'name%d'%i
			gender = count % 3
			if self.register(urs, '****', name, gender, 'auto'):
				succeed += 1
		return succeed



#----------------------------------------------------------------------
# testing
#----------------------------------------------------------------------
if __name__ == '__main__':
	my = {'host':'xnode3.ddns.net', 'user':'skywind', 'passwd':'678900', 'db':'skywind_t9'}
	def test1():
		# if os.path.exists('accountz.db'):
		# 	os.remove('accountz.db')
		db = AccountLocal('accountz.db')
		print(db.register('skywind@tuohn.com', '1234', 'linwei', 1, 'xx'))
		print(db.population(100))
		print(db.login('skywind@tuohn.com', '1234'))
		print(db.login('skywind@tuohn.com', '1'))
		uid = db.query(urs = 'skywind@tuohn.com')['uid']
		print('uid=%d'%uid)
		db.update(uid, {'level':100})
		print(db.query(urs = 'skywind@tuohn.com'))
		db.deposit(uid, 'credit', 30)
		print(db.payment(uid, 'credit', 100))
		print('')
		db.passwd(uid, None, '5678')
		print(db.passwd(uid, '1234', None))
		print(db.passwd(uid, '5678', None))
		print(db.passwd(uid, '5678', 'abcd'))
		print(db.passwd(uid, 'abcd', None))
		print(db.passwd(uid, None, '1234'))
		return 0
	def test2():
		t = time.time()
		db = AccountMySQL(init = True, **my)
		print(time.time() - t)
		print(db.register('skywind@tuohn.com', '1234', 'linwei', 1, 'xx'))
		print(db.register('skywind@tuohn.com', '1234', 'linwei', 1, 'xx'))
		# print(db.population(100))
		print(db.query(urs = 'skywind@tuohn.com'))
		print('')
		uid = db.query(urs = 'skywind@tuohn.com')['uid']
		print('uid=%d'%uid)
		# print(db.query(uid = uid))
		db.update(uid, {'level':100, 'misc':None})
		print(db.login('skywind@tuohn.com', '1234'))
		print(db.login('skywind@tuohn.com', '1'))
		db.deposit(uid, 'credit', 40)
		print(db.payment(uid, 'credit', 100))
		print('')
		db.passwd(uid, None, '5678')
		print(db.passwd(uid, '1234', None))
		print(db.passwd(uid, '5678', None))
		print(db.passwd(uid, '5678', 'abcd'))
		print(db.passwd(uid, 'abcd', None))
		print(db.passwd(uid, None, '1234'))
		return 0
	def test3():
		url = 'mongodb://xnode3.ddns.net/skywind'
		t = time.time()
		db = AccountMongo(url, True)
		print(time.time() - t)
		print(db.register('skywind@tuohn.com', '1234', 'linwei', 1, 'xx'))
		print(db.register('skywind@tuohn.com', '1234', 'linwei', 1, 'xx'))
		print(db.login('skywind@tuohn.com', '1234'))
		print(db.login('skywind@tuohn.com', '1'))
		print('')
		uid = db.query(urs = 'skywind@tuohn.com')['uid']
		print('uid=%d'%uid)
		db.update(uid, {'level':100, 'misc':'haha'})
		print(db.query(urs = 'skywind@tuohn.com'))
		print('')
		print('---------- money --------')
		db.deposit(uid, 'credit', 40)
		print(db.payment(uid, 'credit', 100))
		print('^^^^^^^^^^^^^^^^^^^^^^^^^')
		print('')
		# db.passwd(uid, None, '5678')
		# print(db.passwd(uid, '1234', None))
		# print(db.passwd(uid, '5678', None))
		# print(db.passwd(uid, '5678', 'abcd'))
		# print(db.passwd(uid, 'abcd', None))
		# print(db.passwd(uid, None, '1234'))
		# print('population: %d'%db.population())
		return 0
	test1()



