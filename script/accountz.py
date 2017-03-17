#! /usr/bin/env python
# -*- coding: utf-8 -*-
#======================================================================
#
# accounts.py - 
#
# Created by skywind on 2017/03/16
# Last change: 2017/03/16 11:58:41
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
		    "name" VARCHAR(32),			
		    "pass" VARCHAR(64) DEFAULT(''),
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

		return True

	# 将数据库记录转化为字典
	def __record2obj (self, record):
		if record == None:
			return None
		user = {}
		for k, i in self.__items:
			v = record[i]
			if k == 'misc':
				if v is not None:
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
			if passwd != None:
				c.execute(sql1, (urs, passwd))
			else:
				c.execute(sql2, (urs,))
			record = c.fetchone()
			if record == None:
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
		if urs == None and uid == None:
			return None
		c = self.__conn.cursor()
		record = None
		try:
			if urs != None and uid == None:
				c.execute('select * from account where urs = ?;', (urs,))
			elif urs == None and uid != None:
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
			if not k in self.__enable:
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
				if record == None:
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
		if not kind in ('credit', 'gold'):
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
		if data == None:
			return (1, 0, 'bad uid %d'%uid)
		if changed == 0:
			if data[x1] < money:
				return (2, data[x1], 'not enough %s'%x1)
			return (3, data[x1], 'unknow payment error')
		return (0, data[x1], 'ok')
	
	# 存钱，kind为 'credit'或 'gold'，money是需要增加的钱数
	def deposit (self, uid, kind, money):
		kind = kind.lower()
		if not kind in ('credit', 'gold'):
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
		if data == None:
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
			z = "VALUES('10%d@qq.com', 'name%d', '****', '%d', '%s');"%( \
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
# testing
#----------------------------------------------------------------------
if __name__ == '__main__':
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
		return 0
	def test2():
		return 0
	test1()



